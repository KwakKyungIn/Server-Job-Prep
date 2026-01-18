#include "pch.h"
#include "GameRoom.h"
#include "Player.h"
#include "PlayerSession.h"
#include "ClientPacketHandler.h"
#include "GameRoom.Net.h"
#include "PersistenceService.h"
#include "AutoCommitService.h"
#include "GameItemUidGen.h"

namespace
{
    std::atomic<uint64> g_tradeIdGen{ 1 };

    uint64 AllocTradeId()
    {
        return g_tradeIdGen.fetch_add(1);
    }

    Protocol::ItemInfo* FindItemByUid(std::vector<Protocol::ItemInfo>& items, uint64 uid)
    {
        for (auto& it : items)
        {
            if (static_cast<uint64>(it.itemuid()) == uid)
                return &it;
        }
        return nullptr;
    }

    const Protocol::ItemInfo* FindItemByUidConst(const std::vector<Protocol::ItemInfo>& items, uint64 uid)
    {
        for (const auto& it : items)
        {
            if (static_cast<uint64>(it.itemuid()) == uid)
                return &it;
        }
        return nullptr;
    }

    bool AllocateEmptySlots(const std::vector<Protocol::ItemInfo>& items,
        int32 maxSlots,
        int32 needed,
        const std::vector<int32>& freedSlots,
        std::vector<int32>& outSlots)
    {
        outSlots.clear();
        outSlots.reserve(static_cast<size_t>(needed));

        std::vector<bool> used(maxSlots, false);
        for (const auto& it : items)
        {
            const int32 s = it.slot();
            if (0 <= s && s < maxSlots)
                used[s] = true;
        }

        // Slots that will be freed by a full-stack removal can be reused for incoming items.
        for (int32 s : freedSlots)
        {
            if (0 <= s && s < maxSlots)
                used[s] = false;
        }

        for (int32 n = 0; n < needed; ++n)
        {
            int32 found = -1;
            for (int32 s = 0; s < maxSlots; ++s)
            {
                if (!used[s])
                {
                    used[s] = true;
                    found = s;
                    break;
                }
            }

            if (found < 0)
                return false;

            outSlots.push_back(found);
        }

        return true;
    }

    void SendTradeResult(uint64 playerId, uint64 tradeId, bool success, Protocol::TradeFailCode failCode, const std::string& msg)
    {
        Protocol::S_TRADE_RESULT res;
        res.set_tradeid(tradeId);
        res.set_success(success);
        res.set_failcode(failCode);
        res.set_msg(msg);
        SendToPlayer(playerId, ClientPacketHandler::MakeSendBuffer(res));
    }

    void SendTradeCancelled(uint64 playerId, uint64 tradeId, Protocol::TradeCancelReason reason)
    {
        Protocol::S_TRADE_CANCELLED ntf;
        ntf.set_tradeid(tradeId);
        ntf.set_reason(reason);
        SendToPlayer(playerId, ClientPacketHandler::MakeSendBuffer(ntf));
    }
}



GameRoom::TradeSession* GameRoom::FindTrade_ActorOnly(uint64 tradeId)
{
    auto it = _trades.find(tradeId);
    if (it == _trades.end()) return nullptr;
    return &it->second;
}

GameRoom::TradeSession* GameRoom::FindTradeByPlayer_ActorOnly(uint64 playerId)
{
    auto it = _tradeByPlayer.find(playerId);
    if (it == _tradeByPlayer.end()) return nullptr;
    return FindTrade_ActorOnly(it->second);
}

void GameRoom::HandleTradeReqById(PlayerSessionRef session, uint64 fromPlayerId, uint64 targetPlayerId)
{
    if (!session) return;
    if (fromPlayerId == 0 || targetPlayerId == 0 || fromPlayerId == targetPlayerId)
    {
        SendTradeResult(fromPlayerId, 0, false, Protocol::TRADE_FAIL_INVALID_TARGET, "invalid target");
        return;
    }

    PlayerRef from = FindPlayer_ActorOnly(fromPlayerId);
    PlayerRef to = FindPlayer_ActorOnly(targetPlayerId);

    if (!from || !to)
    {
        SendTradeResult(fromPlayerId, 0, false, Protocol::TRADE_FAIL_INVALID_TARGET, "target not found");
        return;
    }

    if (auto toSession = to->GetSession())
    {
        if (toSession->IsMapChanging())
        {
            SendTradeResult(fromPlayerId, 0, false, Protocol::TRADE_FAIL_INVALID_TARGET, "target is map changing");
            return;
        }
    }


    if (from->ActiveTradeId_ActorOnly() != 0 || to->ActiveTradeId_ActorOnly() != 0)
    {
        SendTradeResult(fromPlayerId, 0, false, Protocol::TRADE_FAIL_ALREADY_TRADING, "already trading");
        return;
    }

    if (!PassDistance2D(*from->GetPosInfo(), *to->GetPosInfo(), 250.f))
    {
        SendTradeResult(fromPlayerId, 0, false, Protocol::TRADE_FAIL_DISTANCE_TOO_FAR, "too far");
        return;
    }

    const uint64 nowMs = ::GetTickCount64();
    const uint64 tradeId = AllocTradeId();

    TradeSession ts;
    ts.tradeId = tradeId;
    ts.playerAId = fromPlayerId;
    ts.playerBId = targetPlayerId;
    ts.state = TradeState::Invited;
    ts.createdAtMs = nowMs;
    ts.lastTouchedMs = nowMs;

    _trades.emplace(tradeId, std::move(ts));
    _tradeByPlayer[fromPlayerId] = tradeId;
    _tradeByPlayer[targetPlayerId] = tradeId;

    from->SetActiveTradeId_ActorOnly(tradeId);
    to->SetActiveTradeId_ActorOnly(tradeId);

    Protocol::S_TRADE_INVITE invite;
    invite.set_fromplayerid(fromPlayerId);
    invite.set_fromname(from->GetName());
    SendToPlayer(targetPlayerId, ClientPacketHandler::MakeSendBuffer(invite));
}

void GameRoom::HandleTradeInviteRespById(PlayerSessionRef session, uint64 responderId, bool accept)
{
    if (!session) return;
    PlayerRef responder = FindPlayer_ActorOnly(responderId);
    if (!responder) return;

    TradeSession* ts = FindTradeByPlayer_ActorOnly(responderId);
    if (!ts)
        return;

    if (ts->state != TradeState::Invited)
        return;

    const uint64 nowMs = ::GetTickCount64();
    ts->lastTouchedMs = nowMs;

    const uint64 aId = ts->playerAId;
    const uint64 bId = ts->playerBId;


    if (responderId != bId)
        return;

    PlayerRef a = FindPlayer_ActorOnly(aId);
    PlayerRef b = FindPlayer_ActorOnly(bId);
    if (!a || !b)
    {
        CancelTrade_ActorOnly(ts->tradeId, Protocol::TRADE_CANCEL_INTERNAL, Protocol::TRADE_FAIL_INTERNAL, "player missing");
        return;
    }

    if (!accept)
    {
        CancelTrade_ActorOnly(ts->tradeId, Protocol::TRADE_CANCEL_DECLINED, Protocol::TRADE_FAIL_REJECTED, "declined");
        return;
    }

    ts->state = TradeState::Active;
    ts->readyA = ts->readyB = false;
    ts->confirmA = ts->confirmB = false;

    Protocol::S_TRADE_START stA;
    stA.set_tradeid(ts->tradeId);
    stA.set_peerid(bId);
    stA.set_peername(b->GetName());

    Protocol::S_TRADE_START stB;
    stB.set_tradeid(ts->tradeId);
    stB.set_peerid(aId);
    stB.set_peername(a->GetName());

    SendToPlayer(aId, ClientPacketHandler::MakeSendBuffer(stA));
    SendToPlayer(bId, ClientPacketHandler::MakeSendBuffer(stB));

    SendReadyState_ActorOnly(ts->tradeId);
}

void GameRoom::HandleTradeOfferSetById(PlayerSessionRef session, uint64 playerId, Protocol::C_TRADE_OFFER_SET pkt)
{
    if (!session) return;

    const uint64 tradeId = pkt.tradeid();
    TradeSession* ts = FindTrade_ActorOnly(tradeId);
    if (!ts)
    {
        SendTradeResult(playerId, tradeId, false, Protocol::TRADE_FAIL_INVALID_STATE, "trade not found");
        return;
    }

    if (ts->state != TradeState::Active)
    {
        SendTradeResult(playerId, tradeId, false, Protocol::TRADE_FAIL_INVALID_STATE, "invalid state");
        return;
    }

    PlayerRef p = FindPlayer_ActorOnly(playerId);
    if (!p)
    {
        SendTradeResult(playerId, tradeId, false, Protocol::TRADE_FAIL_INTERNAL, "player missing");
        return;
    }

    const uint64 nowMs = ::GetTickCount64();
    ts->lastTouchedMs = nowMs;

    const uint64 itemUid = pkt.itemuid();
    const int32 count = pkt.count();

    auto& offer = (playerId == ts->playerAId) ? ts->offerA : ts->offerB;

    if (count <= 0)
    {
        offer.erase(itemUid);
    }
    else
    {
        Protocol::ItemInfo* it = FindItemByUid(p->GetItems(), itemUid);
        if (!it)
        {
            SendTradeResult(playerId, tradeId, false, Protocol::TRADE_FAIL_INVALID_ITEM, "item not found");
            return;
        }

        if (it->isequipped())
        {
            SendTradeResult(playerId, tradeId, false, Protocol::TRADE_FAIL_INVALID_ITEM, "equipped item");
            return;
        }

        if (count > it->count())
        {
            SendTradeResult(playerId, tradeId, false, Protocol::TRADE_FAIL_INVALID_ITEM, "not enough count");
            return;
        }

        TradeOfferEntry e;
        e.itemUid = itemUid;
        e.templateId = it->templateid();
        e.count = count;
        offer[itemUid] = e;
    }

    ts->readyA = ts->readyB = false;
    ts->confirmA = ts->confirmB = false;

    SendOfferUpdate_ActorOnly(tradeId, playerId);
    SendReadyState_ActorOnly(tradeId);
}

void GameRoom::HandleTradeReadyById(PlayerSessionRef session, uint64 playerId, Protocol::C_TRADE_READY pkt)
{
    if (!session) return;

    const uint64 tradeId = pkt.tradeid();
    const bool ready = pkt.ready();

    TradeSession* ts = FindTrade_ActorOnly(tradeId);
    if (!ts)
    {
        SendTradeResult(playerId, tradeId, false, Protocol::TRADE_FAIL_INVALID_STATE, "trade not found");
        return;
    }

    if (ts->state != TradeState::Active)
    {
        SendTradeResult(playerId, tradeId, false, Protocol::TRADE_FAIL_INVALID_STATE, "invalid state");
        return;
    }

    const uint64 nowMs = ::GetTickCount64();
    ts->lastTouchedMs = nowMs;

    if (playerId == ts->playerAId)
        ts->readyA = ready;
    else if (playerId == ts->playerBId)
        ts->readyB = ready;
    else
        return;

    if (!ready)
    {
        if (playerId == ts->playerAId) ts->confirmA = false;
        if (playerId == ts->playerBId) ts->confirmB = false;
    }

    SendReadyState_ActorOnly(tradeId);

    if (ts->readyA && ts->readyB)
    {
        ts->state = TradeState::Locked;

        Protocol::S_TRADE_LOCKED locked;
        locked.set_tradeid(tradeId);
        SendToPlayer(ts->playerAId, ClientPacketHandler::MakeSendBuffer(locked));
        SendToPlayer(ts->playerBId, ClientPacketHandler::MakeSendBuffer(locked));
    }
}

void GameRoom::HandleTradeConfirmById(PlayerSessionRef session, uint64 playerId, Protocol::C_TRADE_CONFIRM pkt)
{
    if (!session) return;

    const uint64 tradeId = pkt.tradeid();
    TradeSession* ts = FindTrade_ActorOnly(tradeId);
    if (!ts)
    {
        SendTradeResult(playerId, tradeId, false, Protocol::TRADE_FAIL_INVALID_STATE, "trade not found");
        return;
    }

    if (ts->state != TradeState::Locked)
    {
        SendTradeResult(playerId, tradeId, false, Protocol::TRADE_FAIL_INVALID_STATE, "invalid state");
        return;
    }

    const uint64 nowMs = ::GetTickCount64();
    ts->lastTouchedMs = nowMs;

    if (playerId == ts->playerAId)
        ts->confirmA = true;
    else if (playerId == ts->playerBId)
        ts->confirmB = true;
    else
        return;

    if (ts->confirmA && ts->confirmB)
    {
        ts->state = TradeState::Committing;

        Protocol::TradeFailCode failCode = Protocol::TRADE_FAIL_INTERNAL;
        std::string msg;

        if (!TryCommitTrade_ActorOnly(tradeId, failCode, msg))
        {
            CancelTrade_ActorOnly(tradeId, Protocol::TRADE_CANCEL_INTERNAL, failCode, msg);
            return;
        }

        SendTradeResult(ts->playerAId, tradeId, true, Protocol::TRADE_FAIL_NONE, "");
        SendTradeResult(ts->playerBId, tradeId, true, Protocol::TRADE_FAIL_NONE, "");

        _tradeByPlayer.erase(ts->playerAId);
        _tradeByPlayer.erase(ts->playerBId);

        PlayerRef a = FindPlayer_ActorOnly(ts->playerAId);
        PlayerRef b = FindPlayer_ActorOnly(ts->playerBId);
        if (a) a->SetActiveTradeId_ActorOnly(0);
        if (b) b->SetActiveTradeId_ActorOnly(0);

        _trades.erase(tradeId);
    }
}

void GameRoom::HandleTradeCancelById(PlayerSessionRef session, uint64 playerId, Protocol::C_TRADE_CANCEL pkt)
{
    if (!session) return;

    const uint64 tradeId = pkt.tradeid();
    TradeSession* ts = FindTrade_ActorOnly(tradeId);
    if (!ts)
        return;

    if (playerId != ts->playerAId && playerId != ts->playerBId)
        return;

    CancelTrade_ActorOnly(tradeId, pkt.reason());
}

void GameRoom::SendOfferUpdate_ActorOnly(uint64 tradeId, uint64 whoPlayerId)
{
    TradeSession* ts = FindTrade_ActorOnly(tradeId);
    if (!ts) return;

    const auto& offer = (whoPlayerId == ts->playerAId) ? ts->offerA : ts->offerB;

    Protocol::S_TRADE_OFFER_UPDATE ntf;
    ntf.set_tradeid(tradeId);
    ntf.set_whoplayerid(whoPlayerId);

    for (const auto& kv : offer)
    {
        const TradeOfferEntry& e = kv.second;
        auto* out = ntf.add_items();
        out->set_itemuid(e.itemUid);
        out->set_templateid(e.templateId);
        out->set_count(e.count);
    }

    SendToPlayer(ts->playerAId, ClientPacketHandler::MakeSendBuffer(ntf));
    SendToPlayer(ts->playerBId, ClientPacketHandler::MakeSendBuffer(ntf));
}

void GameRoom::SendReadyState_ActorOnly(uint64 tradeId)
{
    TradeSession* ts = FindTrade_ActorOnly(tradeId);
    if (!ts) return;

    Protocol::S_TRADE_READY_STATE ntf;
    ntf.set_tradeid(tradeId);
    ntf.set_aready(ts->readyA);
    ntf.set_bready(ts->readyB);

    SendToPlayer(ts->playerAId, ClientPacketHandler::MakeSendBuffer(ntf));
    SendToPlayer(ts->playerBId, ClientPacketHandler::MakeSendBuffer(ntf));
}

void GameRoom::CancelTrade_ActorOnly(uint64 tradeId, Protocol::TradeCancelReason reason, Protocol::TradeFailCode failCode, const std::string& msg)
{
    TradeSession* ts = FindTrade_ActorOnly(tradeId);
    if (!ts) return;

    const uint64 aId = ts->playerAId;
    const uint64 bId = ts->playerBId;

    SendTradeCancelled(aId, tradeId, reason);
    SendTradeCancelled(bId, tradeId, reason);

    if (failCode != Protocol::TRADE_FAIL_NONE)
    {
        SendTradeResult(aId, tradeId, false, failCode, msg);
        SendTradeResult(bId, tradeId, false, failCode, msg);
    }

    _tradeByPlayer.erase(aId);
    _tradeByPlayer.erase(bId);

    PlayerRef a = FindPlayer_ActorOnly(aId);
    PlayerRef b = FindPlayer_ActorOnly(bId);
    if (a) a->SetActiveTradeId_ActorOnly(0);
    if (b) b->SetActiveTradeId_ActorOnly(0);

    _trades.erase(tradeId);
}

void GameRoom::UpdateTrades_ActorOnly(uint64 nowMs)
{
    std::vector<uint64> toCancel;
    toCancel.reserve(8);

    for (const auto& kv : _trades)
    {
        const TradeSession& ts = kv.second;

        if (ts.state == TradeState::Committing)
            continue;

        if (nowMs > ts.lastTouchedMs && (nowMs - ts.lastTouchedMs) > kTradeTimeoutMs)
            toCancel.push_back(ts.tradeId);
    }

    for (uint64 tid : toCancel)
    {
        CancelTrade_ActorOnly(tid, Protocol::TRADE_CANCEL_TIMEOUT, Protocol::TRADE_FAIL_INTERNAL, "timeout");
    }
}

bool GameRoom::TryCommitTrade_ActorOnly(uint64 tradeId, Protocol::TradeFailCode& outFail, std::string& outMsg)
{
    TradeSession* ts = FindTrade_ActorOnly(tradeId);
    if (!ts)
    {
        outFail = Protocol::TRADE_FAIL_INVALID_STATE;
        outMsg = "trade not found";
        return false;
    }

    PlayerRef a = FindPlayer_ActorOnly(ts->playerAId);
    PlayerRef b = FindPlayer_ActorOnly(ts->playerBId);
    if (!a || !b)
    {
        outFail = Protocol::TRADE_FAIL_INTERNAL;
        outMsg = "player missing";
        return false;
    }

    for (const auto& kv : ts->offerA)
    {
        const TradeOfferEntry& e = kv.second;
        const Protocol::ItemInfo* it = FindItemByUidConst(a->GetItems(), e.itemUid);
        if (!it || it->isequipped() || e.count <= 0 || e.count > it->count())
        {
            outFail = Protocol::TRADE_FAIL_INVALID_ITEM;
            outMsg = "invalid offer A";
            return false;
        }
    }

    for (const auto& kv : ts->offerB)
    {
        const TradeOfferEntry& e = kv.second;
        const Protocol::ItemInfo* it = FindItemByUidConst(b->GetItems(), e.itemUid);
        if (!it || it->isequipped() || e.count <= 0 || e.count > it->count())
        {
            outFail = Protocol::TRADE_FAIL_INVALID_ITEM;
            outMsg = "invalid offer B";
            return false;
        }
    }
    std::vector<int32> slotsForA;
    std::vector<int32> slotsForB;

    std::vector<int32> freedSlotsA;
    std::vector<int32> freedSlotsB;
    freedSlotsA.reserve(ts->offerA.size());
    freedSlotsB.reserve(ts->offerB.size());

    // Receiver can reuse slots that will be freed by giving items away (full-stack removals).
    for (const auto& kv : ts->offerA)
    {
        const TradeOfferEntry& e = kv.second;
        const Protocol::ItemInfo* it = FindItemByUidConst(a->GetItems(), e.itemUid);
        if (it && e.count == it->count())
            freedSlotsA.push_back(it->slot());
    }

    for (const auto& kv : ts->offerB)
    {
        const TradeOfferEntry& e = kv.second;
        const Protocol::ItemInfo* it = FindItemByUidConst(b->GetItems(), e.itemUid);
        if (it && e.count == it->count())
            freedSlotsB.push_back(it->slot());
    }

    if (!AllocateEmptySlots(a->GetItems(), kTradeMaxInventorySlots, static_cast<int32>(ts->offerB.size()), freedSlotsA, slotsForA))
    {
        outFail = Protocol::TRADE_FAIL_INVENTORY_FULL;
        outMsg = "A inventory full";
        return false;
    }

    if (!AllocateEmptySlots(b->GetItems(), kTradeMaxInventorySlots, static_cast<int32>(ts->offerA.size()), freedSlotsB, slotsForB))
    {
        outFail = Protocol::TRADE_FAIL_INVENTORY_FULL;
        outMsg = "B inventory full";
        return false;
    }


    std::vector<uint64> removeA;
    std::vector<uint64> removeB;
    removeA.reserve(ts->offerA.size());
    removeB.reserve(ts->offerB.size());

    for (const auto& kv : ts->offerA)
    {
        const TradeOfferEntry& e = kv.second;
        Protocol::ItemInfo* it = FindItemByUid(a->GetItems(), e.itemUid);
        if (!it)
        {
            outFail = Protocol::TRADE_FAIL_INTERNAL;
            outMsg = "commit: item missing A";
            return false;
        }

        if (e.count == it->count())
        {
            removeA.push_back(e.itemUid);
        }
        else
        {
            it->set_count(it->count() - e.count);

            Persistence::PersistenceService::I().UpdateInventoryItem(
                a->GetPlayerId(),
                static_cast<uint64>(it->itemuid()),
                it->templateid(),
                it->slot(),
                it->count(),
                it->isequipped()
            );


            Protocol::S_CHANGE_ITEM ch;
            *ch.mutable_item() = *it;
            SendToPlayer(a->GetPlayerId(), ClientPacketHandler::MakeSendBuffer(ch));
        }
    }

    for (uint64 uid : removeA)
    {
        auto& items = a->GetItems();
        for (auto itv = items.begin(); itv != items.end(); ++itv)
        {
            if (static_cast<uint64>(itv->itemuid()) == uid)
            {
                Persistence::PersistenceService::I().RemoveInventoryItem(a->GetPlayerId(), uid);

                Protocol::S_REMOVE_ITEM rm;
                rm.set_itemuid(uid);
                SendToPlayer(a->GetPlayerId(), ClientPacketHandler::MakeSendBuffer(rm));

                items.erase(itv);
                break;
            }
        }
    }

    for (const auto& kv : ts->offerB)
    {
        const TradeOfferEntry& e = kv.second;
        Protocol::ItemInfo* it = FindItemByUid(b->GetItems(), e.itemUid);
        if (!it)
        {
            outFail = Protocol::TRADE_FAIL_INTERNAL;
            outMsg = "commit: item missing B";
            return false;
        }

        if (e.count == it->count())
        {
            removeB.push_back(e.itemUid);
        }
        else
        {
            it->set_count(it->count() - e.count);

            Persistence::PersistenceService::I().UpdateInventoryItem(
                b->GetPlayerId(),
                static_cast<uint64>(it->itemuid()),
                it->templateid(),
                it->slot(),
                it->count(),
                it->isequipped()
            );


            Protocol::S_CHANGE_ITEM ch;
            *ch.mutable_item() = *it;
            SendToPlayer(b->GetPlayerId(), ClientPacketHandler::MakeSendBuffer(ch));
        }
    }

    for (uint64 uid : removeB)
    {
        auto& items = b->GetItems();
        for (auto itv = items.begin(); itv != items.end(); ++itv)
        {
            if (static_cast<uint64>(itv->itemuid()) == uid)
            {
                Persistence::PersistenceService::I().RemoveInventoryItem(b->GetPlayerId(), uid);

                Protocol::S_REMOVE_ITEM rm;
                rm.set_itemuid(uid);
                SendToPlayer(b->GetPlayerId(), ClientPacketHandler::MakeSendBuffer(rm));

                items.erase(itv);
                break;
            }
        }
    }

    {
        int idx = 0;
        for (const auto& kv : ts->offerB)
        {
            const TradeOfferEntry& e = kv.second;
            const Protocol::ItemInfo* giverItem = FindItemByUidConst(b->GetItems(), e.itemUid);
            if (!giverItem)
            {
                // giver item might have been erased above (full removal). Use cached templateId.
            }

            Protocol::ItemInfo newItem;
            newItem.set_itemuid(GameItemUidGen::Alloc());
            newItem.set_templateid(e.templateId);
            newItem.set_count(e.count);
            newItem.set_slot(slotsForA[idx++]);
            newItem.set_isequipped(false);

            a->GetItems().push_back(newItem);

            Persistence::PersistenceService::I().UpdateInventoryItem(
                a->GetPlayerId(),
                static_cast<uint64>(newItem.itemuid()),
                newItem.templateid(),
                newItem.slot(),
                newItem.count(),
                newItem.isequipped()
            );


            Protocol::S_CHANGE_ITEM ch;
            *ch.mutable_item() = newItem;
            SendToPlayer(a->GetPlayerId(), ClientPacketHandler::MakeSendBuffer(ch));
        }
    }

    {
        int idx = 0;
        for (const auto& kv : ts->offerA)
        {
            const TradeOfferEntry& e = kv.second;

            Protocol::ItemInfo newItem;
            newItem.set_itemuid(GameItemUidGen::Alloc());
            newItem.set_templateid(e.templateId);
            newItem.set_count(e.count);
            newItem.set_slot(slotsForB[idx++]);
            newItem.set_isequipped(false);

            b->GetItems().push_back(newItem);

            Persistence::PersistenceService::I().UpdateInventoryItem(
                b->GetPlayerId(),
                static_cast<uint64>(newItem.itemuid()),
                newItem.templateid(),
                newItem.slot(),
                newItem.count(),
                newItem.isequipped()
            );


            Protocol::S_CHANGE_ITEM ch;
            *ch.mutable_item() = newItem;
            SendToPlayer(b->GetPlayerId(), ClientPacketHandler::MakeSendBuffer(ch));
        }
    }

    // FlushNow (Phase 1 safety)
    Persistence::AutoCommitService::I().RequestFlushNow(a->GetPlayerId());
    Persistence::AutoCommitService::I().RequestFlushNow(b->GetPlayerId());

    outFail = Protocol::TRADE_FAIL_NONE;
    outMsg.clear();
    return true;
}
