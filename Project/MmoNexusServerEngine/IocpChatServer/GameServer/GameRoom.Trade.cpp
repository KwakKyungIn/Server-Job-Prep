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
#include <limits>

namespace
{
    // 거래 ID 발급기. 여러 스레드에서 접근할 수도 있으니 atomic으로 선언함
    std::atomic<uint64> g_tradeIdGen{ 1 };

    uint64 AllocTradeId()
    {
        return g_tradeIdGen.fetch_add(1);
    }

    // UID로 아이템 찾는 헬퍼 함수
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

    // 인벤토리 빈 슬롯 계산 함수 (거래 로직의 핵심)
    // 단순히 빈 칸만 세는 게 아니라, 거래로 인해 빠져나갈 아이템의 슬롯까지 고려해서 계산해야 함
    // A가 B에게 아이템 3개를 주고 2개를 받는 상황이라면, 현재 인벤이 꽉 차 있어도 거래가 가능해야 하기 때문
    bool AllocateEmptySlots(const Vector<Protocol::ItemInfo>& items,
        int32 maxSlots,
        int32 needed,
        const Vector<int32>& freedSlots,
        Vector<int32>& outSlots)
    {
        outSlots.clear();
        outSlots.reserve(static_cast<size_t>(needed));

        // 현재 사용 중인 슬롯 마킹
        Vector<uint8> used(maxSlots, 0);
        for (const auto& it : items)
        {
            const int32 s = it.slot();
            if (0 <= s && s < maxSlots)
                used[s] = 1;
        }

        // 이번 거래로 사라질 아이템들의 슬롯은 재사용 가능하므로 빈 것으로 처리
        for (int32 s : freedSlots)
        {
            if (0 <= s && s < maxSlots)
                used[s] = 0;
        }

        // 필요한 만큼 빈 슬롯 확보
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

            // 공간 부족하면 실패
            if (found < 0)
                return false;

            outSlots.push_back(found);
        }

        return true;
    }

    // 거래 결과 패킷 전송 헬퍼
    void SendTradeResult(uint64 playerId, uint64 tradeId, bool success, Protocol::TradeFailCode failCode, const std::string& msg)
    {
        Protocol::S_TRADE_RESULT res;
        res.set_tradeid(tradeId);
        res.set_success(success);
        res.set_failcode(failCode);
        res.set_msg(msg);
        SendToPlayer(playerId, ClientPacketHandler::MakeSendBuffer(res));
    }

    // 거래 취소 알림 헬퍼
    void SendTradeCancelled(uint64 playerId, uint64 tradeId, Protocol::TradeCancelReason reason)
    {
        Protocol::S_TRADE_CANCELLED ntf;
        ntf.set_tradeid(tradeId);
        ntf.set_reason(reason);
        SendToPlayer(playerId, ClientPacketHandler::MakeSendBuffer(ntf));
    }
}


// 메모리에 있는 거래 세션 찾기
GameRoom::TradeSession* GameRoom::FindTrade_ActorOnly(uint64 tradeId)
{
    auto it = _trades.find(tradeId);
    if (it == _trades.end()) return nullptr;
    return &it->second;
}

// 플레이어 ID로 현재 진행 중인 거래 찾기
GameRoom::TradeSession* GameRoom::FindTradeByPlayer_ActorOnly(uint64 playerId)
{
    auto it = _tradeByPlayer.find(playerId);
    if (it == _tradeByPlayer.end()) return nullptr;
    return FindTrade_ActorOnly(it->second);
}

// 1. 거래 요청 (Handshake 시작)
void GameRoom::HandleTradeReqById(PlayerSessionRef session, uint64 fromPlayerId, uint64 targetPlayerId)
{
    if (!session) return;

    // 자기 자신한테 거래 걸거나 타겟 ID가 이상하면 거부
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

    // 상대방이 맵 이동 중이면 상태가 불안정하므로 거래 불가
    if (auto toSession = to->GetSession())
    {
        if (toSession->IsMapChanging())
        {
            SendTradeResult(fromPlayerId, 0, false, Protocol::TRADE_FAIL_INVALID_TARGET, "target is map changing");
            return;
        }
    }

    // 이미 다른 사람이랑 거래 중인지 확인
    if (from->ActiveTradeId_ActorOnly() != 0 || to->ActiveTradeId_ActorOnly() != 0)
    {
        SendTradeResult(fromPlayerId, 0, false, Protocol::TRADE_FAIL_ALREADY_TRADING, "already trading");
        return;
    }

    // 거리가 너무 멀면 거래 불가 (원격 거래 핵 방지)
    if (!PassDistance2D(*from->GetPosInfo(), *to->GetPosInfo(), 250.f))
    {
        SendTradeResult(fromPlayerId, 0, false, Protocol::TRADE_FAIL_DISTANCE_TOO_FAR, "too far");
        return;
    }

    const uint64 nowMs = ::GetTickCount64();
    const uint64 tradeId = AllocTradeId();

    // 거래 세션 생성 및 초기화
    TradeSession ts;
    ts.tradeId = tradeId;
    ts.playerAId = fromPlayerId;
    ts.playerBId = targetPlayerId;
    ts.state = TradeState::Invited; // 초대 단계
    ts.createdAtMs = nowMs;
    ts.lastTouchedMs = nowMs;

    _trades.emplace(tradeId, std::move(ts));
    _tradeByPlayer[fromPlayerId] = tradeId;
    _tradeByPlayer[targetPlayerId] = tradeId;

    from->SetActiveTradeId_ActorOnly(tradeId);
    to->SetActiveTradeId_ActorOnly(tradeId);

    // 상대방에게 초대 패킷 전송
    Protocol::S_TRADE_INVITE invite;
    invite.set_fromplayerid(fromPlayerId);
    invite.set_fromname(from->GetName());
    SendToPlayer(targetPlayerId, ClientPacketHandler::MakeSendBuffer(invite));
}

// 2. 거래 수락/거절 처리
void GameRoom::HandleTradeInviteRespById(PlayerSessionRef session, uint64 responderId, bool accept)
{
    if (!session) return;
    PlayerRef responder = FindPlayer_ActorOnly(responderId);
    if (!responder) return;

    TradeSession* ts = FindTradeByPlayer_ActorOnly(responderId);
    if (!ts)
        return;

    // 상태 체크: 초대 상태여야만 응답 가능
    if (ts->state != TradeState::Invited)
        return;

    const uint64 nowMs = ::GetTickCount64();
    ts->lastTouchedMs = nowMs;

    const uint64 aId = ts->playerAId;
    const uint64 bId = ts->playerBId;

    // 요청받은 사람(B)이 아닌데 응답이 오면 무시
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

    // 수락했으므로 실제 거래창 오픈 상태로 전환
    ts->state = TradeState::Active;
    ts->readyA = ts->readyB = false;
    ts->confirmA = ts->confirmB = false;

    // 양쪽 클라에 거래창 열라고 패킷 보냄
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

// 3. 물품 등록/수정
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

    // 거래 중일 때만 물건 올릴 수 있음 (잠금 상태면 수정 불가)
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

    // 개수 0 이하면 목록에서 제거
    if (count <= 0)
    {
        offer.erase(itemUid);
    }
    else
    {
        // 실제로 인벤토리에 그 아이템이 있는지 검증 (클라이언트는 믿지 않는다)
        Protocol::ItemInfo* it = FindItemByUid(p->GetItems(), itemUid);
        if (!it)
        {
            SendTradeResult(playerId, tradeId, false, Protocol::TRADE_FAIL_INVALID_ITEM, "item not found");
            return;
        }

        // 장착 중인 아이템은 거래 불가
        if (it->isequipped())
        {
            SendTradeResult(playerId, tradeId, false, Protocol::TRADE_FAIL_INVALID_ITEM, "equipped item");
            return;
        }

        // 가진 개수보다 많이 올리면 거절
        if (count > it->count())
        {
            SendTradeResult(playerId, tradeId, false, Protocol::TRADE_FAIL_INVALID_ITEM, "not enough count");
            return;
        }

        // Offer 목록에 임시 등록
        TradeOfferEntry e;
        e.itemUid = itemUid;
        e.templateId = it->templateid();
        e.count = count;
        offer[itemUid] = e;
    }

    // 물품이 변경되었으니 준비/확인 상태는 모두 초기화 (사기 방지)
    ts->readyA = ts->readyB = false;
    ts->confirmA = ts->confirmB = false;

    // 변경된 오퍼 내용을 양쪽에 전송
    SendOfferUpdate_ActorOnly(tradeId, playerId);
    SendReadyState_ActorOnly(tradeId);
}

// 3-1. 골드 제안/수정
void GameRoom::HandleTradeGoldSetById(PlayerSessionRef session, uint64 playerId, Protocol::C_TRADE_GOLD_SET pkt)
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

    const int64 gold = pkt.gold();
    if (gold < 0)
    {
        SendTradeResult(playerId, tradeId, false, Protocol::TRADE_FAIL_INTERNAL, "invalid gold");
        return;
    }

    if (gold > p->GetGold())
    {
        SendTradeResult(playerId, tradeId, false, Protocol::TRADE_FAIL_NOT_ENOUGH_GOLD, "not enough gold");
        return;
    }

    const uint64 nowMs = ::GetTickCount64();
    ts->lastTouchedMs = nowMs;

    if (playerId == ts->playerAId)
        ts->offerGoldA = gold;
    else if (playerId == ts->playerBId)
        ts->offerGoldB = gold;
    else
        return;

    ts->readyA = ts->readyB = false;
    ts->confirmA = ts->confirmB = false;

    SendOfferUpdate_ActorOnly(tradeId, playerId);
    SendReadyState_ActorOnly(tradeId);
}

// 4. 준비(Ready) 단계
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

    // 준비 취소하면 최종 확인도 같이 풀림
    if (!ready)
    {
        if (playerId == ts->playerAId) ts->confirmA = false;
        if (playerId == ts->playerBId) ts->confirmB = false;
    }

    SendReadyState_ActorOnly(tradeId);

    // 둘 다 준비 완료되면 '잠금(Locked)' 상태로 전환 -> 이제 물품 수정 불가
    if (ts->readyA && ts->readyB)
    {
        ts->state = TradeState::Locked;

        Protocol::S_TRADE_LOCKED locked;
        locked.set_tradeid(tradeId);
        SendToPlayer(ts->playerAId, ClientPacketHandler::MakeSendBuffer(locked));
        SendToPlayer(ts->playerBId, ClientPacketHandler::MakeSendBuffer(locked));
    }
}

// 5. 최종 확인(Confirm) 단계
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

    // Locked 상태여야만 Confirm 가능
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

    // 둘 다 최종 확인을 눌렀다 -> 트랜잭션 시작
    if (ts->confirmA && ts->confirmB)
    {
        ts->state = TradeState::Committing; // 커밋 중 상태로 변경 (취소 불가)

        Protocol::TradeFailCode failCode = Protocol::TRADE_FAIL_INTERNAL;
        std::string msg;

        // DB로 넘기기 전에 메모리 상에서 먼저 검증 및 계획 수립 (Phase 1)
        if (!StartTradeCommitPhase2_ActorOnly(tradeId, failCode, msg))
        {
            // 계획 수립 실패하면 즉시 롤백
            CancelTrade_ActorOnly(tradeId, Protocol::TRADE_CANCEL_INTERNAL, failCode, msg);
            return;
        }
    }
}

// 거래 취소 요청
void GameRoom::HandleTradeCancelById(PlayerSessionRef session, uint64 playerId, Protocol::C_TRADE_CANCEL pkt)
{
    if (!session) return;

    const uint64 tradeId = pkt.tradeid();
    TradeSession* ts = FindTrade_ActorOnly(tradeId);
    if (!ts)
        return;

    if (playerId != ts->playerAId && playerId != ts->playerBId)
        return;

    // 이미 커밋(DB 저장) 중이면 취소 불가능. 
    // 여기서 취소해버리면 DB는 성공했는데 메모리는 롤백되는 대참사 발생함.
    if (ts->state == TradeState::Committing)
        return;

    CancelTrade_ActorOnly(tradeId, pkt.reason());
}

void GameRoom::SendOfferUpdate_ActorOnly(uint64 tradeId, uint64 whoPlayerId)
{
    TradeSession* ts = FindTrade_ActorOnly(tradeId);
    if (!ts) return;

    const auto& offer = (whoPlayerId == ts->playerAId) ? ts->offerA : ts->offerB;
    const int64 offerGold = (whoPlayerId == ts->playerAId) ? ts->offerGoldA : ts->offerGoldB;

    Protocol::S_TRADE_OFFER_UPDATE ntf;
    ntf.set_tradeid(tradeId);
    ntf.set_whoplayerid(whoPlayerId);
    ntf.set_gold(offerGold);

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

// 거래 강제 종료 및 정리
void GameRoom::CancelTrade_ActorOnly(uint64 tradeId, Protocol::TradeCancelReason reason, Protocol::TradeFailCode failCode, const std::string& msg)
{
    TradeSession* ts = FindTrade_ActorOnly(tradeId);
    if (!ts) return;

    // 커밋 중일 때는 내부적인 실패(DB 에러 등)를 제외하곤 취소 불가
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

        // 커밋 중인 건 타임아웃 체크 안 함 (DB 응답 기다려야 함)
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

// [핵심 로직] 거래 확정 전 메모리 시뮬레이션 (Phase 1)
// DB에 보내기 전에 이 거래가 진짜 가능한지, 인벤토리는 충분한지, 아이템 합치기는 되는지 미리 계산해봄
// 성공하면 DB에 보낼 '계획서(Plan)'를 생성함
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

    // 골드 검증 및 최종값 계산
    const int64 goldA = a->GetGold();
    const int64 goldB = b->GetGold();
    const int64 offerGoldA = ts->offerGoldA;
    const int64 offerGoldB = ts->offerGoldB;

    if (offerGoldA < 0 || offerGoldB < 0)
    {
        outFail = Protocol::TRADE_FAIL_INTERNAL;
        outMsg = "invalid gold";
        return false;
    }

    if (offerGoldA > goldA || offerGoldB > goldB)
    {
        outFail = Protocol::TRADE_FAIL_NOT_ENOUGH_GOLD;
        outMsg = "not enough gold";
        return false;
    }

    const int64 afterA = goldA - offerGoldA;
    const int64 afterB = goldB - offerGoldB;
    if (offerGoldB > 0 && afterA > ((std::numeric_limits<int64>::max)() - offerGoldB))
    {
        outFail = Protocol::TRADE_FAIL_INTERNAL;
        outMsg = "gold overflow";
        return false;
    }
    if (offerGoldA > 0 && afterB > ((std::numeric_limits<int64>::max)() - offerGoldA))
    {
        outFail = Protocol::TRADE_FAIL_INTERNAL;
        outMsg = "gold overflow";
        return false;
    }

    outPlan.finalGoldA = afterA + offerGoldB;
    outPlan.finalGoldB = afterB + offerGoldA;

    // 소모품인지 확인 (스택 가능 여부)
    auto isConsumable = [](int32 templateId) -> bool
        {
            const auto* t = DataManager::Instance()->GetItemTemplate(templateId);
            return t && t->itemtype() == Protocol::ITEM_TYPE_CONSUMABLE;
        };

    // 합칠 수 있는 아이템 스택 찾기 (슬롯 절약)
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

    // 변경사항 기록 헬퍼
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

    // 사용 중인 슬롯 비트맵 생성
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

    // 빈 슬롯 찾기
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

    // 임시 컨테이너에서 아이템 제거
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

    // 1단계: 제공하려는 아이템이 진짜 있는지 최종 검증
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

    // 실제 인벤토리를 건드리지 않고, 복사본(Snapshot)을 떠서 시뮬레이션 진행
    Vector<Protocol::ItemInfo> aItems = a->GetItems();
    Vector<Protocol::ItemInfo> bItems = b->GetItems();

    // 2단계: 주는 쪽(Giver) 처리 - 아이템 삭제 혹은 개수 차감
    // A의 물건 처리
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
            // 전부 주면 삭제 목록에 추가
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
            // 일부만 주면 개수만 깜
            it->set_count(it->count() - e.count);
            upsertChange(outPlan.notifyChangeA, *it);
        }
    }

    // B의 물건 처리
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

    // 3단계: 받는 쪽(Receiver) 처리 - 아이템 추가
    // 여기서 중요한 건, 위에서 아이템을 삭제하면서 생긴 '빈 슬롯'을 재활용할 수 있어야 한다는 점
    Vector<uint8> usedA = buildUsedSlots(aItems, kTradeMaxInventorySlots);
    Vector<uint8> usedB = buildUsedSlots(bItems, kTradeMaxInventorySlots);

    // B가 준 물건을 A가 받음
    for (const auto& kv : ts->offerB)
    {
        const TradeOfferEntry& e = kv.second;

        if (isConsumable(e.templateId))
        {
            // 소모품이면 기존 스택에 합치기 시도
            if (Protocol::ItemInfo* dst = findBestStack(aItems, e.templateId))
            {
                dst->set_count(dst->count() + e.count);
                upsertChange(outPlan.notifyChangeA, *dst);
                continue;
            }
        }

        // 새 슬롯 할당
        int32 slot = -1;
        if (!takeEmptySlot(usedA, slot))
        {
            outFail = Protocol::TRADE_FAIL_INVENTORY_FULL;
            outMsg = "A inventory full";
            return false;
        }

        // 새 아이템 생성 (UID도 새로 발급)
        Protocol::ItemInfo newItem;
        newItem.set_itemuid(GameItemUidGen::Alloc());
        newItem.set_templateid(e.templateId);
        newItem.set_count(e.count);
        newItem.set_slot(slot);
        newItem.set_isequipped(false);

        aItems.push_back(newItem);
        upsertChange(outPlan.notifyChangeA, newItem);
    }

    // A가 준 물건을 B가 받음
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

    // 계획 성공. 최종 결과물 저장
    outPlan.finalAItems = std::move(aItems);
    outPlan.finalBItems = std::move(bItems);

    outFail = Protocol::TRADE_FAIL_NONE;
    outMsg.clear();
    return true;
}

// [핵심 로직] 거래 커밋 Phase 2 (DB 전송)
bool GameRoom::StartTradeCommitPhase2_ActorOnly(uint64 tradeId, Protocol::TradeFailCode& outFail, std::string& outMsg)
{
    TradeSession* ts = FindTrade_ActorOnly(tradeId);
    if (!ts)
    {
        outFail = Protocol::TRADE_FAIL_INVALID_STATE;
        outMsg = "trade not found";
        return false;
    }

    // 이미 진행 중인지 확인
    if (ts->commitPlan)
    {
        outFail = Protocol::TRADE_FAIL_INVALID_STATE;
        outMsg = "commit already in flight";
        return false;
    }

    // 위에서 만든 Phase 1 시뮬레이션 돌림
    TradeCommitPlan plan;
    if (!BuildTradeCommitPlan_ActorOnly(tradeId, plan, outFail, outMsg))
        return false;

    // 계획을 세션에 저장해두고 (나중에 결과 오면 적용해야 하니까)
    ts->commitPlan = std::make_unique<TradeCommitPlan>(std::move(plan));

    // DBAgent에게 "이대로 DB에 한 방에(Transaction) 처리해줘"라고 요청
    // S2S(Server-to-Server) 패킷 생성
    Protocol::S2S_REQ_TRADE_COMMIT req;
    req.set_tradeid(tradeId);
    req.set_channelid(GetChannelId());
    req.set_mapid(GetMapId());
    req.set_instanceid(GetInstanceId());
    req.set_playeraid(ts->playerAId);
    req.set_playerbid(ts->playerBId);
    req.set_finalgolda(ts->commitPlan->finalGoldA);
    req.set_finalgoldb(ts->commitPlan->finalGoldB);

    // 변경될 최종 아이템 목록과 삭제될 아이템 목록을 전부 보냄
    // DBAgent는 이걸 받아서 SQL Transaction 안에서 싹 다 처리하거나, 실패하면 하나도 처리 안 함
    for (const auto& it : ts->commitPlan->finalAItems)
        *req.add_finalaitems() = it;
    for (uint64 uid : ts->commitPlan->deletedAItemUids)
        req.add_deletedaitemuids(uid);

    for (const auto& it : ts->commitPlan->finalBItems)
        *req.add_finalbitems() = it;
    for (uint64 uid : ts->commitPlan->deletedBItemUids)
        req.add_deletedbitemuids(uid);

    // 요청 ID 발급 (응답 매칭용)
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

    // 전송
    G_DBSession->Send(S2SPacketHandler::MakeSendBuffer(req));
    return true;
}

// DB 스레드에서 결과를 받아서 다시 GameRoom 스레드로 넘겨주는 브릿지 함수
void GameRoom::OnTradeCommitResult(Protocol::S2S_RES_TRADE_COMMIT pkt)
{
    OnTradeCommitResult_ActorOnly(pkt);
}

// [핵심 로직] 거래 커밋 결과 처리
// DB에서 "성공했어"라고 하면 메모리에 반영하고 클라에 알림
// "실패했어"라고 하면 메모리 건드리지 않고 거래 취소 처리
void GameRoom::OnTradeCommitResult_ActorOnly(const Protocol::S2S_RES_TRADE_COMMIT& pkt)
{
    const uint64 tradeId = pkt.tradeid();
    TradeSession* ts = FindTrade_ActorOnly(tradeId);
    if (!ts)
        return;

    // 상태 검증
    if (ts->state != TradeState::Committing)
        return;

    // 요청 ID 검증 (혹시나 타임아웃 되어서 재요청했거나 옛날 응답일까봐)
    if (ts->commitRequestId != 0 && pkt.requestid() != 0 && pkt.requestid() != ts->commitRequestId)
        return; // stale response

    if (!ts->commitPlan)
    {
        CancelTrade_ActorOnly(tradeId, Protocol::TRADE_CANCEL_INTERNAL, Protocol::TRADE_FAIL_INTERNAL, "missing commit plan");
        return;
    }

    // DB 트랜잭션 실패 시 처리 (롤백)
    if (!pkt.success())
    {
        Protocol::TradeFailCode fail = pkt.failcode();
        if (fail == Protocol::TRADE_FAIL_NONE)
            fail = Protocol::TRADE_FAIL_INTERNAL;
        CancelTrade_ActorOnly(tradeId, Protocol::TRADE_CANCEL_INTERNAL, fail, "DB commit failed");
        return;
    }

    // === 여기서부터는 성공 처리 ===
    PlayerRef a = FindPlayer_ActorOnly(ts->playerAId);
    PlayerRef b = FindPlayer_ActorOnly(ts->playerBId);
    if (!a || !b)
    {
        CancelTrade_ActorOnly(tradeId, Protocol::TRADE_CANCEL_INTERNAL, Protocol::TRADE_FAIL_INTERNAL, "player missing");
        return;
    }

    const TradeCommitPlan& plan = *ts->commitPlan;

    // 1. 메모리 반영: 아까 시뮬레이션해둔 결과를 진짜 플레이어 데이터에 덮어씌움
    a->GetItems() = plan.finalAItems;
    b->GetItems() = plan.finalBItems;

    a->SetGold(plan.finalGoldA);
    b->SetGold(plan.finalGoldB);

    // 2. Redis 캐시 동기화: RDBMS는 업데이트됐으니 Redis도 맞춰줌
    // markDirty=false로 하는 이유는 이미 DB에는 들어갔으니까 또 DB 저장 큐에 넣을 필요가 없어서임
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

    Persistence::PersistenceService::I().UpdatePlayerGold(a->GetPlayerId(), a->GetGold(), /*markDirty=*/false);
    Persistence::PersistenceService::I().UpdatePlayerGold(b->GetPlayerId(), b->GetGold(), /*markDirty=*/false);

    {
        Protocol::S_GOLD_UPDATE goldPktA;
        goldPktA.set_gold(a->GetGold());
        SendToPlayer(a->GetPlayerId(), ClientPacketHandler::MakeSendBuffer(goldPktA));

        Protocol::S_GOLD_UPDATE goldPktB;
        goldPktB.set_gold(b->GetGold());
        SendToPlayer(b->GetPlayerId(), ClientPacketHandler::MakeSendBuffer(goldPktB));
    }

    // 3. 클라이언트 알림: 아이템 사라지고 생긴 거 패킷으로 쏴줌 (델타 업데이트)
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

    // 최종 성공 메세지 전송
    SendTradeResult(ts->playerAId, tradeId, true, Protocol::TRADE_FAIL_NONE, "");
    SendTradeResult(ts->playerBId, tradeId, true, Protocol::TRADE_FAIL_NONE, "");

    // 세션 정리
    _tradeByPlayer.erase(ts->playerAId);
    _tradeByPlayer.erase(ts->playerBId);

    a->SetActiveTradeId_ActorOnly(0);
    b->SetActiveTradeId_ActorOnly(0);

    _trades.erase(tradeId);
}
