#include "pch.h"
#include "GameRoom.h"
#include "Player.h"
#include "PlayerSession.h"
#include "RoomManager.h"

void GameRoom::TransferMapChangeById(PlayerSessionRef session,
    uint64 playerId,
    int32 targetChannelId,
    int32 targetMapId,
    int64 targetInstanceId,
    const Protocol::PositionInfo& spawn)
{
    if (!session || !GRoomManager)
    {
        if (session) session->Post([](PlayerSessionRef s) { s->CancelMapChange(); });
        return;
    }

    auto it = _players.find(playerId);
    if (it == _players.end() || !it->second)
    {
        session->Post([](PlayerSessionRef s) { s->CancelMapChange(); });
        return;
    }

    PlayerRef player = it->second;

    //  목적 채널 확정
    int32 destChannelId = targetChannelId;
    if (destChannelId <= 0)
        destChannelId = player->GetChannelId();

    auto lobby = GRoomManager->GetOrCreateLobby(destChannelId);
    auto newRoom = GRoomManager->GetOrCreateRoom(destChannelId, targetMapId, targetInstanceId);
    if (!lobby || !newRoom)
    {
        session->Post([](PlayerSessionRef s) { s->CancelMapChange(); });
        return;
    }

    // [Trade] mapchange -> force cancel
    const uint64 tradeId = player->ActiveTradeId_ActorOnly();
    if (tradeId != 0)
    {
        CancelTrade_ActorOnly(tradeId, Protocol::TRADE_CANCEL_MAP_CHANGE);
    }

    Leave(session, player);

    player->SetChannelId(destChannelId);     //  핵심
    player->SetMapId(targetMapId);
    player->SetInstanceId(targetInstanceId);
    if (player->GetPosInfo())
        player->GetPosInfo()->CopyFrom(spawn);

    player->SetSession(session);
    player->SetRoom(lobby);

    session->Post([lobby](PlayerSessionRef s) { s->SetCurrentRoom(lobby); });

    const uint64 pid = player->GetPlayerId();

    lobby->Push([lobby, newRoom, session, player, pid]() mutable
        {
            lobby->Adopt(player, true); //  transfer=true 권장

            newRoom->Push([newRoom, lobby, session, player, pid]() mutable
                {
                    newRoom->EnterMapChange(session, player);

                    session->Post([newRoom](PlayerSessionRef s)
                        {
                            s->SetCurrentRoom(newRoom);
                            s->EndMapChange(); //  1회
                        });

                    lobby->Push([lobby, pid]() { lobby->Detach(pid); });
                });
        });
}

// GameRoom.cpp
void GameRoom::SaveReturnLocation_ActorOnly(uint64 playerId)
{
    PlayerRef p = FindPlayer_ActorOnly(playerId);
    if (!p) return;

    auto pos = p->GetPosInfo();
    if (!pos) return;

    //  정책: return map/instance는 "Player가 들고있는 값" 기준
    p->SetReturnLocation(p->GetMapId(), p->GetInstanceId(), *pos);
}

