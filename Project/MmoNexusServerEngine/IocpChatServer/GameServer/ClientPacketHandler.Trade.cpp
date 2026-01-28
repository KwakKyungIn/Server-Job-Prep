#include "pch.h"
#include "ClientPacketHandler.h"
#include "PlayerSession.h"
#include "GameRoom.h"

// 개인 거래 요청 핸들러
// 거래는 두 플레이어의 상태가 동시에 변해야 하므로 동기화가 매우 중요하다
// 따라서 세션 스레드에서 처리하지 않고, 두 플레이어가 속한 'GameRoom' 액터로 작업을 넘긴다
bool ClientPacketHandler::Handle_C_TRADE_REQ(PacketSessionRef& session, Protocol::C_TRADE_REQ& pkt)
{
    PlayerSessionRef ps = static_pointer_cast<PlayerSession>(session);
    if (!ps) return false;

    // 맵 이동 중에는 거래 불가능 (상대방이 사라질 수 있음)
    if (ps->IsMapChanging()) return true;

    const uint64 playerId = ps->GetPlayerId_AnyThread();
    if (playerId == 0) return true;

    const uint64 targetId = pkt.targetplayerid();

    // 1. 플레이어가 속한 룸을 찾고 (PostRoom)
    // 2. 그 룸의 JobQueue에 거래 로직을 넣는다 (gr->Push)
    // 이렇게 하면 Room 스레드 하나에서 순차적으로 실행되므로 Race Condition이 없다
    ps->PostRoom([playerId, targetId](PlayerSessionRef self, RoomActorRef room) mutable
        {
            if (!room) return;
            if (!self) return;
            if (self->IsMapChanging()) return;
            if (room->GetKind() != RoomKind::Game) return;

            auto gr = static_pointer_cast<GameRoom>(room);
            gr->Push([gr, self, playerId, targetId]() mutable
                {
                    // 실제 거래 요청 로직은 GameRoom 안에서 수행
                    gr->HandleTradeReqById(self, playerId, targetId);
                });
        });

    return true;
}

// 거래 초대 응답 (수락/거절) 핸들러
bool ClientPacketHandler::Handle_C_TRADE_INVITE_RESP(PacketSessionRef& session, Protocol::C_TRADE_INVITE_RESP& pkt)
{
    PlayerSessionRef ps = static_pointer_cast<PlayerSession>(session);
    if (!ps) return false;

    if (ps->IsMapChanging()) return true;

    const uint64 playerId = ps->GetPlayerId_AnyThread();
    if (playerId == 0) return true;

    const bool accept = pkt.accept();

    // 응답 처리 역시 룸 액터로 위임해서 상태 머신을 안전하게 돌린다
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

// 거래 물품/골드 등록 핸들러
// 아이템을 올리거나 내릴 때 인벤토리 락 검사 등이 필요함
bool ClientPacketHandler::Handle_C_TRADE_OFFER_SET(PacketSessionRef& session, Protocol::C_TRADE_OFFER_SET& pkt)
{
    PlayerSessionRef ps = static_pointer_cast<PlayerSession>(session);
    if (!ps) return false;

    if (ps->IsMapChanging()) return true;

    const uint64 playerId = ps->GetPlayerId_AnyThread();
    if (playerId == 0) return true;

    // 룸 액터 큐에 작업을 넣어서, 상대방이 동시에 물품을 올리더라도
    // 하나씩 순서대로 처리되게 보장한다
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

// 거래 골드 제안 핸들러
bool ClientPacketHandler::Handle_C_TRADE_GOLD_SET(PacketSessionRef& session, Protocol::C_TRADE_GOLD_SET& pkt)
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
                    gr->HandleTradeGoldSetById(self, playerId, pkt);
                });
        });

    return true;
}

// 거래 준비 완료(Lock) 핸들러
// 양쪽 다 Ready를 박으면 거래 확정 단계로 넘어간다
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

// 거래 최종 확정 핸들러
// 여기서 실제 아이템 소유권 이전과 골드 교환이 일어난다 (트랜잭션)
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

// 거래 취소 핸들러
// 도중에 취소하면 올려둔 아이템 락을 풀고 원래 상태로 되돌린다
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
