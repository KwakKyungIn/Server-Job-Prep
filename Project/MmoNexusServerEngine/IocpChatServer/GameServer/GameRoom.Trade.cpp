#include "pch.h"
#include "GameRoom.h"
#include "Player.h"
#include "PlayerSession.h"
#include "ClientPacketHandler.h"
#include "S2SPacketHandler.h"
#include "GameRoom.Net.h"
#include "PersistenceService.h"
#include "AutoCommitService.h"
#include "GameItemUidGen.h"
#include "DataManager.h"

namespace
{
    std::atomic<uint64> g_tradeIdGen{ 1 };

    uint64 AllocTradeId()
    {
        return g_tradeIdGen.fetch_add(1);
    }

    Protocol::ItemInfo* FindItemByUid(Vector<Protocol::ItemInfo>& items, uint64 uid)
    {
        for (auto& it : items)
        {
            if (static_cast<uint64>(it.itemuid()) == uid)
                return &it;
        }
        return nullptr;
    }

    const Protocol::ItemInfo* FindItemByUidConst(const Vector<Protocol::ItemInfo>& items, uint64 uid)
    {
        for (const auto& it : items)
        {
            if (static_cast<uint64>(it.itemuid()) == uid)
                return &it;
        }
        return nullptr;
    }

    bool AllocateEmptySlots(const Vector<Protocol::ItemInfo>& items,
        int32 maxSlots,
        int32 needed,
        const Vector<int32>& freedSlots,
        Vector<int32>& outSlots)
    {
        outSlots.clear();
        outSlots.reserve(static_cast<size_t>(needed));

        Vector<uint8> used(maxSlots, 0);
        for (const auto& it : items)
        {
            const int32 s = it.slot();
            if (0 <= s && s < maxSlots)
                used[s] = 1;
        }

        // Slots that will be freed by a full-stack removal can be reused for incoming items.
        for (int32 s : freedSlots)
        {
            if (0 <= s && s < maxSlots)
                used[s] = 0;
        }

        for (int32 n = 0; n < needed; ++n)
        {
            int32 found = -1;
            for (int32 s = 0; s < maxSlots; ++s)
            {
                if (!used[s])
                {
                    used[s] = 1;
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

        // Phase 2: DBAgent atomic commit (single SQL transaction for A/B)
        if (!StartTradeCommitPhase2_ActorOnly(tradeId, failCode, msg))
        {
            CancelTrade_ActorOnly(tradeId, Protocol::TRADE_CANCEL_INTERNAL, failCode, msg);
            return;
        }
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

    // Once DBAgent commit is in flight, do not allow user-driven cancellation.
    if (ts->state == TradeState::Committing)
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

    // Do not cancel a commit-in-flight trade unless it's an internal cancellation (e.g., DBAgent failure).
    if (ts->state == TradeState::Committing && reason != Protocol::TRADE_CANCEL_INTERNAL)
        return;

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
    Vector<uint64> toCancel;
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

bool GameRoom::BuildTradeCommitPlan_ActorOnly(uint64 tradeId, TradeCommitPlan& outPlan, Protocol::TradeFailCode& outFail, std::string& outMsg)
{
    outPlan = TradeCommitPlan{};

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

    auto isConsumable = [](int32 templateId) -> bool
        {
            const auto* t = DataManager::Instance()->GetItemTemplate(templateId);
            return t && t->itemtype() == Protocol::ITEM_TYPE_CONSUMABLE;
        };

    auto findBestStack = [](Vector<Protocol::ItemInfo>& items, int32 templateId) -> Protocol::ItemInfo*
        {
            Protocol::ItemInfo* best = nullptr;
            for (auto& it : items)
            {
                if (it.isequipped())
                    continue;
                if (it.templateid() != templateId)
                    continue;

                if (best == nullptr || it.count() > best->count())
                    best = &it;
            }
            return best;
        };

    auto upsertChange = [](Vector<Protocol::ItemInfo>& changes, const Protocol::ItemInfo& item)
        {
            const uint64 uid = (uint64)item.itemuid();
            for (auto& c : changes)
            {
                if ((uint64)c.itemuid() == uid)
                {
                    c = item;
                    return;
                }
            }
            changes.push_back(item);
        };

    auto buildUsedSlots = [](const Vector<Protocol::ItemInfo>& items, int32 maxSlots) -> Vector<uint8>
        {
            Vector<uint8> used(maxSlots, 0);
            for (const auto& it : items)
            {
                const int32 s = it.slot();
                if (s >= 0 && s < maxSlots)
                    used[s] = 1;
            }
            return used;
        };

    auto takeEmptySlot = [](Vector<uint8>& used, int32& outSlot) -> bool
        {
            for (int32 i = 0; i < static_cast<int32>(used.size()); ++i)
            {
                if (!used[i])
                {
                    used[i] = 1;
                    outSlot = i;
                    return true;
                }
            }
            return false;
        };

    auto eraseByUid = [](Vector<Protocol::ItemInfo>& items, uint64 uid) -> bool
        {
            for (auto it = items.begin(); it != items.end(); ++it)
            {
                if (static_cast<uint64>(it->itemuid()) == uid)
                {
                    items.erase(it);
                    return true;
                }
            }
            return false;
        };

    // Validate offers against current memory snapshot.
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

    // Work on temporary snapshots; memory is applied only after DBAgent success.
    Vector<Protocol::ItemInfo> aItems = a->GetItems();
    Vector<Protocol::ItemInfo> bItems = b->GetItems();

    // Apply giver A changes (remove or decrement).
    for (const auto& kv : ts->offerA)
    {
        const TradeOfferEntry& e = kv.second;
        Protocol::ItemInfo* it = FindItemByUid(aItems, e.itemUid);
        if (!it)
        {
            outFail = Protocol::TRADE_FAIL_INTERNAL;
            outMsg = "commit plan: item missing A";
            return false;
        }

        if (e.count == it->count())
        {
            outPlan.deletedAItemUids.push_back(e.itemUid);
            outPlan.notifyRemoveA.push_back(e.itemUid);

            if (!eraseByUid(aItems, e.itemUid))
            {
                outFail = Protocol::TRADE_FAIL_INTERNAL;
                outMsg = "commit plan: erase failed A";
                return false;
            }
        }
        else
        {
            it->set_count(it->count() - e.count);
            upsertChange(outPlan.notifyChangeA, *it);
        }
    }

    // Apply giver B changes (remove or decrement).
    for (const auto& kv : ts->offerB)
    {
        const TradeOfferEntry& e = kv.second;
        Protocol::ItemInfo* it = FindItemByUid(bItems, e.itemUid);
        if (!it)
        {
            outFail = Protocol::TRADE_FAIL_INTERNAL;
            outMsg = "commit plan: item missing B";
            return false;
        }

        if (e.count == it->count())
        {
            outPlan.deletedBItemUids.push_back(e.itemUid);
            outPlan.notifyRemoveB.push_back(e.itemUid);

            if (!eraseByUid(bItems, e.itemUid))
            {
                outFail = Protocol::TRADE_FAIL_INTERNAL;
                outMsg = "commit plan: erase failed B";
                return false;
            }
        }
        else
        {
            it->set_count(it->count() - e.count);
            upsertChange(outPlan.notifyChangeB, *it);
        }
    }

    // Build used-slot bitmap after giver changes.
    Vector<uint8> usedA = buildUsedSlots(aItems, kTradeMaxInventorySlots);
    Vector<uint8> usedB = buildUsedSlots(bItems, kTradeMaxInventorySlots);

    // Add incoming items to A (from B's offer).
    for (const auto& kv : ts->offerB)
    {
        const TradeOfferEntry& e = kv.second;

        if (isConsumable(e.templateId))
        {
            // ITEM_TYPE_CONSUMABLE: merge into the largest existing stack if present.
            if (Protocol::ItemInfo* dst = findBestStack(aItems, e.templateId))
            {
                dst->set_count(dst->count() + e.count);
                upsertChange(outPlan.notifyChangeA, *dst);
                continue;
            }
        }

        int32 slot = -1;
        if (!takeEmptySlot(usedA, slot))
        {
            outFail = Protocol::TRADE_FAIL_INVENTORY_FULL;
            outMsg = "A inventory full";
            return false;
        }

        Protocol::ItemInfo newItem;
        newItem.set_itemuid(GameItemUidGen::Alloc());
        newItem.set_templateid(e.templateId);
        newItem.set_count(e.count);
        newItem.set_slot(slot);
        newItem.set_isequipped(false);

        aItems.push_back(newItem);
        upsertChange(outPlan.notifyChangeA, newItem);
    }

    // Add incoming items to B (from A's offer).
    for (const auto& kv : ts->offerA)
    {
        const TradeOfferEntry& e = kv.second;

        if (isConsumable(e.templateId))
        {
            if (Protocol::ItemInfo* dst = findBestStack(bItems, e.templateId))
            {
                dst->set_count(dst->count() + e.count);
                upsertChange(outPlan.notifyChangeB, *dst);
                continue;
            }
        }

        int32 slot = -1;
        if (!takeEmptySlot(usedB, slot))
        {
            outFail = Protocol::TRADE_FAIL_INVENTORY_FULL;
            outMsg = "B inventory full";
            return false;
        }

        Protocol::ItemInfo newItem;
        newItem.set_itemuid(GameItemUidGen::Alloc());
        newItem.set_templateid(e.templateId);
        newItem.set_count(e.count);
        newItem.set_slot(slot);
        newItem.set_isequipped(false);

        bItems.push_back(newItem);
        upsertChange(outPlan.notifyChangeB, newItem);
    }

    outPlan.finalAItems = std::move(aItems);
    outPlan.finalBItems = std::move(bItems);

    outFail = Protocol::TRADE_FAIL_NONE;
    outMsg.clear();
    return true;
}

bool GameRoom::StartTradeCommitPhase2_ActorOnly(uint64 tradeId, Protocol::TradeFailCode& outFail, std::string& outMsg)
{
    TradeSession* ts = FindTrade_ActorOnly(tradeId);
    if (!ts)
    {
        outFail = Protocol::TRADE_FAIL_INVALID_STATE;
        outMsg = "trade not found";
        return false;
    }

    if (ts->commitPlan)
    {
        outFail = Protocol::TRADE_FAIL_INVALID_STATE;
        outMsg = "commit already in flight";
        return false;
    }

    TradeCommitPlan plan;
    if (!BuildTradeCommitPlan_ActorOnly(tradeId, plan, outFail, outMsg))
        return false;

    ts->commitPlan = std::make_unique<TradeCommitPlan>(std::move(plan));

    // Build S2S request.
    Protocol::S2S_REQ_TRADE_COMMIT req;
    req.set_tradeid(tradeId);
    req.set_channelid(GetChannelId());
    req.set_mapid(GetMapId());
    req.set_instanceid(GetInstanceId());
    req.set_playeraid(ts->playerAId);
    req.set_playerbid(ts->playerBId);

    for (const auto& it : ts->commitPlan->finalAItems)
        *req.add_finalaitems() = it;
    for (uint64 uid : ts->commitPlan->deletedAItemUids)
        req.add_deletedaitemuids(uid);

    for (const auto& it : ts->commitPlan->finalBItems)
        *req.add_finalbitems() = it;
    for (uint64 uid : ts->commitPlan->deletedBItemUids)
        req.add_deletedbitemuids(uid);

    // Optional request id (useful for stale response filtering).
    static std::atomic<uint64> s_tradeCommitReqGen{ 1 };
    ts->commitRequestId = s_tradeCommitReqGen.fetch_add(1);
    req.set_requestid(ts->commitRequestId);

    extern shared_ptr<PacketSession> G_DBSession;
    if (G_DBSession == nullptr)
    {
        outFail = Protocol::TRADE_FAIL_INTERNAL;
        outMsg = "DB session missing";
        ts->commitPlan.reset();
        ts->commitRequestId = 0;
        return false;
    }

    // Note: S2SPacketHandler is auto-generated from Protocol_S2S.proto.
    G_DBSession->Send(S2SPacketHandler::MakeSendBuffer(req));
    return true;
}

void GameRoom::OnTradeCommitResult(Protocol::S2S_RES_TRADE_COMMIT pkt)
{
    // This is called on GameRoom actor thread.
    OnTradeCommitResult_ActorOnly(pkt);
}

void GameRoom::OnTradeCommitResult_ActorOnly(const Protocol::S2S_RES_TRADE_COMMIT& pkt)
{
    const uint64 tradeId = pkt.tradeid();
    TradeSession* ts = FindTrade_ActorOnly(tradeId);
    if (!ts)
        return;

    if (ts->state != TradeState::Committing)
        return;

    if (ts->commitRequestId != 0 && pkt.requestid() != 0 && pkt.requestid() != ts->commitRequestId)
        return; // stale response

    if (!ts->commitPlan)
    {
        CancelTrade_ActorOnly(tradeId, Protocol::TRADE_CANCEL_INTERNAL, Protocol::TRADE_FAIL_INTERNAL, "missing commit plan");
        return;
    }

    if (!pkt.success())
    {
        Protocol::TradeFailCode fail = pkt.failcode();
        if (fail == Protocol::TRADE_FAIL_NONE)
            fail = Protocol::TRADE_FAIL_INTERNAL;
        CancelTrade_ActorOnly(tradeId, Protocol::TRADE_CANCEL_INTERNAL, fail, "DB commit failed");
        return;
    }

    PlayerRef a = FindPlayer_ActorOnly(ts->playerAId);
    PlayerRef b = FindPlayer_ActorOnly(ts->playerBId);
    if (!a || !b)
    {
        CancelTrade_ActorOnly(tradeId, Protocol::TRADE_CANCEL_INTERNAL, Protocol::TRADE_FAIL_INTERNAL, "player missing");
        return;
    }

    const TradeCommitPlan& plan = *ts->commitPlan;

    // Apply memory snapshots.
    a->GetItems() = plan.finalAItems;
    b->GetItems() = plan.finalBItems;

    // Sync Redis (no extra autocommit flush).
    for (uint64 uid : plan.deletedAItemUids)
        Persistence::PersistenceService::I().RemoveInventoryItem(a->GetPlayerId(), uid, /*markDirty=*/false);
    for (const auto& it : plan.finalAItems)
        Persistence::PersistenceService::I().UpdateInventoryItem(a->GetPlayerId(), static_cast<uint64>(it.itemuid()), it.templateid(), it.slot(), it.count(), it.isequipped(), /*markDirty=*/false);
    Persistence::PersistenceService::I().ClearDirtyOnCommitSuccess(a->GetPlayerId(), /*coreOk=*/false, /*invOk=*/true, /*qsOk=*/false);

    for (uint64 uid : plan.deletedBItemUids)
        Persistence::PersistenceService::I().RemoveInventoryItem(b->GetPlayerId(), uid, /*markDirty=*/false);
    for (const auto& it : plan.finalBItems)
        Persistence::PersistenceService::I().UpdateInventoryItem(b->GetPlayerId(), static_cast<uint64>(it.itemuid()), it.templateid(), it.slot(), it.count(), it.isequipped(), /*markDirty=*/false);
    Persistence::PersistenceService::I().ClearDirtyOnCommitSuccess(b->GetPlayerId(), /*coreOk=*/false, /*invOk=*/true, /*qsOk=*/false);

    // Notify clients (inventory delta).
    for (uint64 uid : plan.notifyRemoveA)
    {
        Protocol::S_REMOVE_ITEM rm;
        rm.set_itemuid(uid);
        SendToPlayer(a->GetPlayerId(), ClientPacketHandler::MakeSendBuffer(rm));
    }
    for (const auto& it : plan.notifyChangeA)
    {
        Protocol::S_CHANGE_ITEM ch;
        *ch.mutable_item() = it;
        SendToPlayer(a->GetPlayerId(), ClientPacketHandler::MakeSendBuffer(ch));
    }

    for (uint64 uid : plan.notifyRemoveB)
    {
        Protocol::S_REMOVE_ITEM rm;
        rm.set_itemuid(uid);
        SendToPlayer(b->GetPlayerId(), ClientPacketHandler::MakeSendBuffer(rm));
    }
    for (const auto& it : plan.notifyChangeB)
    {
        Protocol::S_CHANGE_ITEM ch;
        *ch.mutable_item() = it;
        SendToPlayer(b->GetPlayerId(), ClientPacketHandler::MakeSendBuffer(ch));
    }

    SendTradeResult(ts->playerAId, tradeId, true, Protocol::TRADE_FAIL_NONE, "");
    SendTradeResult(ts->playerBId, tradeId, true, Protocol::TRADE_FAIL_NONE, "");

    _tradeByPlayer.erase(ts->playerAId);
    _tradeByPlayer.erase(ts->playerBId);

    a->SetActiveTradeId_ActorOnly(0);
    b->SetActiveTradeId_ActorOnly(0);

    _trades.erase(tradeId);
}
