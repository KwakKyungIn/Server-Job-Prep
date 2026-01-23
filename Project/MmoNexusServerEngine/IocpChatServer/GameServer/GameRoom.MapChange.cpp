#include "pch.h"
#include "GameRoom.h"
#include "Player.h"
#include "PlayerSession.h"
#include "RoomManager.h"

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