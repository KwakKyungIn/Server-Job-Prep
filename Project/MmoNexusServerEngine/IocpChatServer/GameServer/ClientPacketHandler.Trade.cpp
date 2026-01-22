#include "pch.h"
#include "ClientPacketHandler.h"
#include "PlayerSession.h"
#include "GameRoom.h"

bool ClientPacketHandler::Handle_C_TRADE_REQ(PacketSessionRef& session, Protocol::C_TRADE_REQ& pkt)
{
    PlayerSessionRef ps = static_pointer_cast<PlayerSession>(session);
    if (!ps) return false;

    // Ignore trade requests during MapChange.
    if (ps->IsMapChanging()) return true;

    const uint64 playerId = ps->GetPlayerId_AnyThread();
    if (playerId == 0) return true;

    const uint64 targetId = pkt.targetplayerid();

    ps->PostRoom([playerId, targetId](PlayerSessionRef self, RoomActorRef room) mutable
        {
            if (!room) return;
            if (!self) return;
            if (self->IsMapChanging()) return;
            if (room->GetKind() != RoomKind::Game) return;

            auto gr = static_pointer_cast<GameRoom>(room);
            gr->Push([gr, self, playerId, targetId]() mutable
                {
                    gr->HandleTradeReqById(self, playerId, targetId);
                });
        });

    return true;
}

bool ClientPacketHandler::Handle_C_TRADE_INVITE_RESP(PacketSessionRef& session, Protocol::C_TRADE_INVITE_RESP& pkt)
{
    PlayerSessionRef ps = static_pointer_cast<PlayerSession>(session);
    if (!ps) return false;

    if (ps->IsMapChanging()) return true;

    const uint64 playerId = ps->GetPlayerId_AnyThread();
    if (playerId == 0) return true;

    const bool accept = pkt.accept();

    ps->PostRoom([playerId, accept](PlayerSessionRef self, RoomActorRef room) mutable
        {
            if (!room) return;
            if (!self) return;
            if (self->IsMapChanging()) return;
            if (room->GetKind() != RoomKind::Game) return;

            auto gr = static_pointer_cast<GameRoom>(room);
            gr->Push([gr, self, playerId, accept]() mutable
                {
                    gr->HandleTradeInviteRespById(self, playerId, accept);
                });
        });

    return true;
}

bool ClientPacketHandler::Handle_C_TRADE_OFFER_SET(PacketSessionRef& session, Protocol::C_TRADE_OFFER_SET& pkt)
{
    PlayerSessionRef ps = static_pointer_cast<PlayerSession>(session);
    if (!ps) return false;

    if (ps->IsMapChanging()) return true;

    const uint64 playerId = ps->GetPlayerId_AnyThread();
    if (playerId == 0) return true;

    ps->PostRoom([playerId, pkt](PlayerSessionRef self, RoomActorRef room) mutable
        {
            if (!room) return;
            if (!self) return;
            if (self->IsMapChanging()) return;
            if (room->GetKind() != RoomKind::Game) return;

            auto gr = static_pointer_cast<GameRoom>(room);
            gr->Push([gr, self, playerId, pkt]() mutable
                {
                    gr->HandleTradeOfferSetById(self, playerId, pkt);
                });
        });

    return true;
}

bool ClientPacketHandler::Handle_C_TRADE_READY(PacketSessionRef& session, Protocol::C_TRADE_READY& pkt)
{
    PlayerSessionRef ps = static_pointer_cast<PlayerSession>(session);
    if (!ps) return false;

    if (ps->IsMapChanging()) return true;

    const uint64 playerId = ps->GetPlayerId_AnyThread();
    if (playerId == 0) return true;

    ps->PostRoom([playerId, pkt](PlayerSessionRef self, RoomActorRef room) mutable
        {
            if (!room) return;
            if (!self) return;
            if (self->IsMapChanging()) return;
            if (room->GetKind() != RoomKind::Game) return;

            auto gr = static_pointer_cast<GameRoom>(room);
            gr->Push([gr, self, playerId, pkt]() mutable
                {
                    gr->HandleTradeReadyById(self, playerId, pkt);
                });
        });

    return true;
}

bool ClientPacketHandler::Handle_C_TRADE_CONFIRM(PacketSessionRef& session, Protocol::C_TRADE_CONFIRM& pkt)
{
    PlayerSessionRef ps = static_pointer_cast<PlayerSession>(session);
    if (!ps) return false;

    if (ps->IsMapChanging()) return true;

    const uint64 playerId = ps->GetPlayerId_AnyThread();
    if (playerId == 0) return true;

    ps->PostRoom([playerId, pkt](PlayerSessionRef self, RoomActorRef room) mutable
        {
            if (!room) return;
            if (!self) return;
            if (self->IsMapChanging()) return;
            if (room->GetKind() != RoomKind::Game) return;

            auto gr = static_pointer_cast<GameRoom>(room);
            gr->Push([gr, self, playerId, pkt]() mutable
                {
                    gr->HandleTradeConfirmById(self, playerId, pkt);
                });
        });

    return true;
}

bool ClientPacketHandler::Handle_C_TRADE_CANCEL(PacketSessionRef& session, Protocol::C_TRADE_CANCEL& pkt)
{
    PlayerSessionRef ps = static_pointer_cast<PlayerSession>(session);
    if (!ps) return false;

    if (ps->IsMapChanging()) return true;

    const uint64 playerId = ps->GetPlayerId_AnyThread();
    if (playerId == 0) return true;

    ps->PostRoom([playerId, pkt](PlayerSessionRef self, RoomActorRef room) mutable
        {
            if (!room) return;
            if (!self) return;
            if (self->IsMapChanging()) return;
            if (room->GetKind() != RoomKind::Game) return;

            auto gr = static_pointer_cast<GameRoom>(room);
            gr->Push([gr, self, playerId, pkt]() mutable
                {
                    gr->HandleTradeCancelById(self, playerId, pkt);
                });
        });

    return true;
}
