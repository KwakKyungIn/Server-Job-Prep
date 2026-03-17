#include "pch.h"
#include "GameRoom.h"
#include "Player.h"
#include "PlayerSession.h"
#include "RoomManager.h"
#include "ClientPacketHandler.h"
#include "ClientPacketHandler.MapChangeUtil.h"
#include "DataManager.h"
#include "ExperimentUtils.h"
#include "GameMetrics.h"
#include "GameRoom.Net.h"
#include "PersistenceService.h"

// 플레이어를 다른 맵이나 채널로 이동시키는 함수
// 비동기 구조라 로비 -> 새 방 순서로 Job을 넘겨가며 처리함
void GameRoom::TransferMapChangeById(PlayerSessionRef session,
    uint64 playerId,
    int32 targetChannelId,
    int32 targetMapId,
    int64 targetInstanceId,
    const Protocol::PositionInfo& spawn)
{
    // 세션이나 매니저 없으면 바로 취소 처리. 이거 없으면 크래시 남
    if (!session || !GRoomManager)
    {
        if (session) session->Post([](PlayerSessionRef s) { s->CancelMapChange(); });
        return;
    }

    // 플레이어가 현재 방에 실제로 존재하는지 확인
    auto it = _players.find(playerId);
    if (it == _players.end() || !it->second)
    {
        session->Post([](PlayerSessionRef s) { s->CancelMapChange(); });
        return;
    }

    PlayerRef player = it->second;

    // 이동할 채널 결정. 0 이하면 현재 채널 유지
    int32 destChannelId = targetChannelId;
    if (destChannelId <= 0)
        destChannelId = player->GetChannelId();

    // 이동할 목적지의 로비랑 방을 미리 확보해둠
    auto lobby = GRoomManager->GetOrCreateLobby(destChannelId);
    auto newRoom = GRoomManager->GetOrCreateRoom(destChannelId, targetMapId, targetInstanceId);
    if (!lobby || !newRoom)
    {
        session->Post([](PlayerSessionRef s) { s->CancelMapChange(); });
        return;
    }

    // 거래 중이면 맵 이동 불가라서 강제로 거래 취소시킴
    const uint64 tradeId = player->ActiveTradeId_ActorOnly();
    if (tradeId != 0)
    {
        CancelTrade_ActorOnly(tradeId, Protocol::TRADE_CANCEL_MAP_CHANGE);
    }

    // 현재 방에서 플레이어를 제거함 (Grid, PlayerMap 등에서 빠짐)
    Leave(session, player);

    // 플레이어 정보 갱신. 이제 소속은 새 방 쪽으로 넘어감
    player->SetChannelId(destChannelId);
    player->SetMapId(targetMapId);
    player->SetInstanceId(targetInstanceId);

    // 스폰 위치 지정되어 있으면 거기 로 설정
    if (player->GetPosInfo())
        player->GetPosInfo()->CopyFrom(spawn);

    // [Respawn] 던전에서 사망 후 월드 복귀하는 경우, 여기서 부활 상태로 전환
    if (player->HasPendingRespawn())
    {
        auto* st = player->GetStatInfo();
        if (st)
        {
            int32 maxHp = st->maxhp();
            if (maxHp <= 0)
                maxHp = 1;
            st->set_hp(maxHp);

            Persistence::PersistenceService::I().UpdatePlayerCore(
                player->GetPlayerId(),
                st->level(),
                st->hp(),
                st->totalexp(),
                true
            );
        }

        if (auto pos = player->GetPosInfo())
        {
            pos->set_state(Protocol::MOVE_IDLE);
            pos->set_actionstate(Protocol::ACTION_IDLE);
        }

        player->ResetMoveStamp_ActorOnly();

        // 클라에 HP 복구를 알려서 부활 상태 UI를 갱신한다
        if (st)
        {
            Protocol::S_CHANGE_HP hpPkt;
            hpPkt.set_objectid(playerId);
            hpPkt.set_attackerid(0);
            hpPkt.set_currenthp(st->hp());
            hpPkt.set_damage(0);
            session->Send(ClientPacketHandler::MakeSendBuffer(hpPkt));
        }

        player->ClearPendingRespawn();
    }

    player->SetSession(session);
    player->SetRoom(lobby); // 일단 로비 소속으로 변경

    // 세션 쪽에도 현재 방이 로비라고 알려줌 (패킷 처리용)
    session->Post([lobby](PlayerSessionRef s) { s->SetCurrentRoom(lobby); });

    const uint64 pid = player->GetPlayerId();

    // 여기서부터 Job Chain 시작
    // 1. 로비 스레드: 플레이어를 로비에 등록 (Adopt)
    lobby->Push([lobby, newRoom, session, player, pid]() mutable
        {
            lobby->Adopt(player, true);

            // 2. 새 방 스레드: 플레이어를 실제 목적지 방으로 입장시킴
            newRoom->Push([newRoom, lobby, session, player, pid]() mutable
                {
                    newRoom->EnterMapChange(session, player);

                    // 3. 세션 스레드: 클라이언트한테 이동 끝났다고 알려줌
                    session->Post([newRoom](PlayerSessionRef s)
                        {
                            s->SetCurrentRoom(newRoom);
                            s->EndMapChange();
                        });

                    // 4. 다시 로비 스레드: 로비 목록에서는 제거 (이제 새 방에 들어갔으니까)
                    lobby->Push([lobby, pid]() { lobby->Detach(pid); });
                });
        });
}

// 마을 귀환용 위치 저장 함수
void GameRoom::SaveReturnLocation_ActorOnly(uint64 playerId)
{
    PlayerRef p = FindPlayer_ActorOnly(playerId);
    if (!p) return;

    auto pos = p->GetPosInfo();
    if (!pos) return;

    // 현재 플레이어가 서 있는 곳을 귀환 위치로 저장함
    p->SetReturnLocation(p->GetMapId(), p->GetInstanceId(), *pos);
}

// [부활 처리] 플레이어가 죽었을 때 요청받아 리스폰 시킨다
void GameRoom::HandleRespawn(PlayerSessionRef session, PlayerRef player)
{
    if (!session || !player) return;

    auto* st = player->GetStatInfo();
    auto* pos = player->GetPosInfo();
    if (!st || !pos) return;

    const bool isDead = (st->hp() <= 0) || (pos->actionstate() == Protocol::ACTION_DEAD);
    if (!isDead)
        return;

    // 던전에서는 원래 복귀해야 할 위치로 리스폰 (맵 이동 처리)
    DataManager* dm = DataManager::Instance();
    const bool isDungeon = IsInstanceRoom() || (dm && dm->IsDungeonMapId(player->GetMapId()));
    if (isDungeon)
    {
        if (!player->HasPendingRespawn())
            player->MarkPendingRespawn(true);

        MapChangeUtil::ForceReturnToWorld(session);
        return;
    }

    // 월드맵이면 현재 맵의 스폰 위치로 리스폰
    Protocol::PositionInfo spawn;
    const MapConfig* cfg = (dm ? dm->GetMapConfig(player->GetMapId()) : nullptr);
    if (cfg)
    {
        spawn.set_x(cfg->spawnX);
        spawn.set_y(cfg->spawnY);
        spawn.set_z(cfg->spawnZ);
    }
    else
    {
        spawn.set_x(50.f);
        spawn.set_y(0.f);
        spawn.set_z(50.f);
    }

    if (ExperimentUtils::ShouldRandomizeRespawnSpawn())
    {
        if (ExperimentUtils::TryRandomizeSpawn(player->GetMapId(), spawn, _map.get()))
        {
            printf(" [Experiment] Random respawn spawn: player=%llu map=%d pos=(%.1f, %.1f, %.1f)\n",
                player->GetPlayerId(), player->GetMapId(), spawn.x(), spawn.y(), spawn.z());
        }
    }

    spawn.set_yaw(0.f);
    spawn.set_state(Protocol::MOVE_IDLE);
    spawn.set_actionstate(Protocol::ACTION_IDLE);

    pos->CopyFrom(spawn);

    int32 maxHp = st->maxhp();
    if (maxHp <= 0)
        maxHp = 1;
    st->set_hp(maxHp);

    // 이동 검증 스탬프 초기화 (텔레포트 후 스피드핵 오탐 방지)
    player->ResetMoveStamp_ActorOnly();

    // Zone 이동 처리
    const int32 oldZoneIndex = player->GetZoneIndex();
    const int32 newZoneIndex = _grid.GetZoneIndex(*pos);
    const int32 totalZones = _grid.GetGridSizeX() * _grid.GetGridSizeY();
    if (newZoneIndex >= 0 && newZoneIndex < totalZones)
    {
        if (oldZoneIndex != newZoneIndex)
        {
            if (oldZoneIndex >= 0 && oldZoneIndex < totalZones)
                _grid.GetZone(oldZoneIndex).players.erase(player);

            _grid.GetZone(newZoneIndex).players.insert(player);
            player->SetZoneIndex(newZoneIndex);
        }
    }

    // AOI 갱신 (텔레포트 처리를 위해 강제 호출)
    if (!ExperimentUtils::IsHotRoomRoomWideBaseline())
        UpdateAOI(session, player, false);

    const uint64 playerId = player->GetPlayerId();

    // 위치/상태 동기화
    SendMoveSync(session, player, *pos, true, true);

    // HP 복구 동기화
    {
        Protocol::S_CHANGE_HP hpPkt;
        hpPkt.set_objectid(playerId);
        hpPkt.set_attackerid(0);
        hpPkt.set_currenthp(st->hp());
        hpPkt.set_damage(0);

        SendBufferRef hpSb = ClientPacketHandler::MakeSendBuffer(hpPkt);
        session->Send(hpSb);

        if (ExperimentUtils::IsHotRoomRoomWideBaseline())
        {
            const int32 recipients = Broadcast(hpSb, playerId);
            GameMetrics::OnBroadcastRecipients(
                GameMetrics::HotRoomBroadcastKind::Hp,
                GameMetrics::HotRoomBroadcastMode::Room,
                static_cast<std::size_t>(recipients));
        }
        else
        {
            auto& vis = player->VisiblePlayers_ActorOnly();
            for (uint64 vid : vis)
            {
                if (vid == playerId) continue;
                SendToPlayer(vid, hpSb);
            }
        }
    }

    // DB/Redis에 즉시 반영
    Persistence::PersistenceService::I().UpdatePlayerCore(
        playerId,
        st->level(),
        st->hp(),
        st->totalexp(),
        true
    );
}

void GameRoom::HandleRespawnById(PlayerSessionRef session, uint64 playerId)
{
    auto it = _players.find(playerId);
    if (it == _players.end())
        return;

    HandleRespawn(session, it->second);
}
