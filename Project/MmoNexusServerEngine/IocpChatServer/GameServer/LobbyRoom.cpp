// LobbyRoom.cpp
#include "pch.h"
#include "LobbyRoom.h"
#include "Player.h"
#include "PlayerSession.h"
#include "ClientPacketHandler.h"  // 인벤 동기화 보내려면 필요
#include "S2SPacketHandler.h"

std::shared_ptr<LobbyRoom> GLobbyRoom;

void LobbyRoom::EnterGame(PlayerSessionRef ps, uint64 playerId, int32 channelId, int32 mapId, const Protocol::PositionInfo& spawn)
{
    if (!ps) return;
    if (playerId == 0) return;

    // 로비는 채널 단위로 운용할 거면 channelId mismatch 방어
    // (필요 없으면 지워도 됨)
    if (channelId != _channelId)
    {
        // 채널 로비를 따로 두는 설계면 여기서 그냥 return 해도 됨.
        // 지금은 강제 교정해준다.
        channelId = _channelId;
    }

    auto& slot = _players[playerId];

    // 새로 만들거나, 있으면 갱신(재접속/중복 EnterGame)
    if (!slot.player)
    {
        PlayerRef p = MakeShared<Player>();

        Protocol::PlayerInfo info;
        info.set_playerid(playerId);
        info.set_name("Player_" + std::to_string(playerId));
        info.mutable_posinfo()->CopyFrom(spawn);

        p->Init(info);
        p->SetChannelId(channelId);
        p->SetMapId(mapId);
        p->SetInstanceId(0);

        // 약결합(weak) OK
        p->SetSession(ps);

        slot.player = p;
    }
    else
    {
        PlayerRef p = slot.player;
        p->SetChannelId(channelId);
        p->SetMapId(mapId);
        p->SetInstanceId(0);
        p->GetPlayerInfo()->mutable_posinfo()->CopyFrom(spawn);
        p->SetSession(ps);
    }

    // 이번 EnterGame 사이클에서 DB 응답을 다시 받을 거니까 flag reset
    slot.itemsLoaded = false;
    slot.statLoaded = false;

    // Lobby가 소유 + “어떤 Room에도 속함” 보장
    Adopt(slot.player);
}

void LobbyRoom::OnItemsLoaded(uint64 playerId, const Protocol::S2S_RES_ITEMS_LOAD& pkt)
{
    auto it = _players.find(playerId);
    if (it == _players.end()) return;
    if (!it->second.player) return;

    PlayerRef p = it->second.player;

    // 1) 인벤 반영
    p->SetItems(pkt.items());

    // 2) 장착 보너스 포함 스탯 재계산(Stat 먼저 와도, Item 먼저 와도 재계산 계속 해도 됨)
    p->RefreshStats();

    // 3) 클라 동기화(인벤)
    if (auto ps = p->GetSession())
    {
        Protocol::S_ITEM_LIST clientPkt;
        clientPkt.mutable_items()->CopyFrom(pkt.items());
        ps->Send(ClientPacketHandler::MakeSendBuffer(clientPkt));
    }

    it->second.itemsLoaded = true;
}

void LobbyRoom::OnStatLoaded(uint64 playerId, const Protocol::S2S_RES_LOAD_PLAYER_DATA& pkt)
{
    auto it = _players.find(playerId);
    if (it == _players.end()) return;
    if (!it->second.player) return;

    PlayerRef p = it->second.player;

    if (auto st = p->GetStatInfo())
        st->CopyFrom(pkt.statinfo());

    p->RefreshStats();

    it->second.statLoaded = true;
}

bool LobbyRoom::IsReady(uint64 playerId) const
{
    auto it = _players.find(playerId);
    if (it == _players.end()) return false;
    return (it->second.player != nullptr) && it->second.itemsLoaded && it->second.statLoaded;
}

PlayerRef LobbyRoom::DetachIfReady(uint64 playerId)
{
    if (!IsReady(playerId))
        return nullptr;
    return Detach(playerId);
}

// =========================================================
// 기존 API들 (Pending 구조로 맞춰서 수정)
// =========================================================

void LobbyRoom::Adopt(PlayerRef player)
{
    if (!player) return;

    const uint64 pid = player->GetPlayerId();
    if (pid == 0) return;

    // slot 없으면 생성
    auto& slot = _players[pid];
    slot.player = player;

    // ✅ Player는 "항상 어떤 Room"에 속해야 한다.
    // 주의: 이 SetRoom의 타입이 GameRoom(LobbyRoom)까지 받을 수 있어야 함.
    // (너가 말한 Step1에서 Creature::SetRoom 타입 정리하는 걸로 해결)
    player->SetRoom(shared_from_this());
}

PlayerRef LobbyRoom::Detach(uint64 playerId)
{
    auto it = _players.find(playerId);
    if (it == _players.end())
        return nullptr;

    PlayerRef p = it->second.player;
    _players.erase(it);

    // Detach 시점에 SetRoom(nullptr) 할지 여부는 너가 Transfer 파이프라인에서 결정.
    // (다음 Room이 즉시 Adopt한다면 굳이 nullptr 안 둬도 됨)

    return p;
}

PlayerRef LobbyRoom::Find(uint64 playerId) const
{
    auto it = _players.find(playerId);
    if (it == _players.end())
        return nullptr;
    return it->second.player;
}
