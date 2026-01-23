#include "pch.h"
#include "ClientPacketHandler.h"
#include "PlayerSession.h"
#include "PersistenceService.h"
#include "Protocol_S2S.pb.h"

// 클라이언트가 주기적으로 보내는 하트비트 패킷 처리
// 연결이 살아있는지 확인하는 용도고, 로직은 딱히 없다
bool ClientPacketHandler::Handle_C_HEART_BEAT_REQ(PacketSessionRef& session, Protocol::C_HEART_BEAT_REQ& pkt)
{
    return true;
}


static constexpr int32 QS_MAX = 12;

// 퀵슬롯 설정 요청 핸들러
// 단순한 DB 저장이 아니라, 아이템 중복 등록 방지 로직이 포함되어 있다
bool ClientPacketHandler::Handle_C_SET_QUICKSLOT(PacketSessionRef& session, Protocol::C_SET_QUICKSLOT& pkt)
{
    PlayerSessionRef ps = static_pointer_cast<PlayerSession>(session);
    if (!ps) return false;

    const int32 idx = pkt.slotindex();
    if (idx < 0 || idx >= QS_MAX)
        return true;

    // 세션에서 플레이어 ID를 안전하게 가져온다
    const uint64 pid = ps->GetPlayerId_AnyThread();
    if (pid == 0)
        return true;

    // 맵 이동 중에 퀵슬롯 바꾸면 DB 정합성이 꼬일 수 있으니 막는다
    if (ps->IsMapChanging())
        return true;

    Protocol::QuickSlotRefType rt = pkt.reftype();
    uint64 refId = pkt.refid();

    // 빈 슬롯 요청이 오거나 ID가 없으면 삭제(NONE)로 처리
    if (rt == Protocol::QS_NONE || refId == 0)
    {
        rt = Protocol::QS_NONE;
        refId = 0;
    }
    else
    {
        // 유효하지 않은 타입이면 무시
        if (rt != Protocol::QS_ITEM && rt != Protocol::QS_SKILL)
            return true;
    }

    // DB 접근이 필요하므로 세션 액터 스레드에 태워서 순차적으로 처리함
    ps->Post([pid, idx, rt, refId](PlayerSessionRef self) mutable
        {
            // [중복 방지 규칙]
            // 같은 아이템이 여러 퀵슬롯에 등록되는 것을 막는다
            // 예: 1번 슬롯에 있는 포션을 2번 슬롯으로 옮기면, 1번 슬롯은 비워야 한다
            if (rt == Protocol::QS_ITEM && refId != 0)
            {
                Protocol::S2S_REQ_SAVE_QUICKSLOT snap;
                // 현재 퀵슬롯 상태를 스냅샷으로 떠와서 검사
                if (Persistence::PersistenceService::I().BuildSnapshot_QuickSlot(pid, snap))
                {
                    for (const auto& s : snap.slots())
                    {
                        // 지금 등록하려는 슬롯은 건너뜀
                        if (s.slotindex() == idx)
                            continue;
                        // 아이템이 아니면 건너뜀
                        if (s.reftype() != Protocol::QS_ITEM)
                            continue;
                        // 다른 아이템이면 건너뜀
                        if ((uint64)s.refid() != refId)
                            continue;

                        // 중복 발견! 기존에 있던 슬롯을 비워준다 (DB + 메모리 갱신)
                        Persistence::PersistenceService::I().UpdateQuickSlot(pid, s.slotindex(), Protocol::QS_NONE, 0, /*markDirty=*/true);

                        // 클라이언트에게도 기존 슬롯이 비워졌음을 알려야 UI 갱신이 됨
                        Protocol::S_SET_QUICKSLOT cleared;
                        cleared.set_success(true);
                        auto* cs = cleared.mutable_slot();
                        cs->set_slotindex(s.slotindex());
                        cs->set_reftype(Protocol::QS_NONE);
                        cs->set_refid(0);

                        self->Send(ClientPacketHandler::MakeSendBuffer(cleared));
                    }
                }
            }

            // 이제 요청받은 슬롯에 데이터를 덮어쓴다 (Redis + Dirty Flag 설정)
            Persistence::PersistenceService::I().UpdateQuickSlot(pid, idx, rt, refId, /*markDirty=*/true);

            // 최종적으로 클라이언트에게 변경 성공 응답 전송
            Protocol::S_SET_QUICKSLOT out;
            out.set_success(true);

            auto* s = out.mutable_slot();
            s->set_slotindex(idx);
            s->set_reftype(rt);
            s->set_refid(refId);

            self->Send(ClientPacketHandler::MakeSendBuffer(out));
        });

    return true;
}