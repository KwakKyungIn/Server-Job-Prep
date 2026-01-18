#include "pch.h"
#include "ClientPacketHandler.h"
#include "PlayerSession.h"
#include "PersistenceService.h"

bool ClientPacketHandler::Handle_C_HEART_BEAT_REQ(PacketSessionRef& session, Protocol::C_HEART_BEAT_REQ& pkt)
{
	return true;
}


static constexpr int32 QS_MAX = 12;

bool ClientPacketHandler::Handle_C_SET_QUICKSLOT(PacketSessionRef& session, Protocol::C_SET_QUICKSLOT& pkt)
{
    PlayerSessionRef ps = static_pointer_cast<PlayerSession>(session);
    if (!ps) return false;

    const int32 idx = pkt.slotindex();
    if (idx < 0 || idx >= QS_MAX)
        return true; // 무시

    //  pid 확보: 너 코드 스타일상 AnyThread getter 있을 확률 큼
    // 없으면 네가 가진 방식(세션->playerId 조회)으로 바꿔.
    const uint64 pid = ps->GetPlayerId_AnyThread();
    if (pid == 0)
        return true;

    // MapChange 중이면 업데이트 막고 싶으면 이거 유지
    if (ps->IsMapChanging())
        return true;

    Protocol::QuickSlotRefType rt = pkt.reftype();
    uint64 refId = pkt.refid();

    // normalize "clear"
    if (rt == Protocol::QS_NONE || refId == 0)
    {
        rt = Protocol::QS_NONE;
        refId = 0;
    }
    else
    {
        // 최소 sanity
        if (rt != Protocol::QS_ITEM && rt != Protocol::QS_SKILL)
            return true;
    }

    ps->Post([pid, idx, rt, refId](PlayerSessionRef self) mutable
        {
            //  Redis 반영 + dirty
            Persistence::PersistenceService::I().UpdateQuickSlot(pid, idx, rt, refId, /*markDirty=*/true);

            //  Ack (확정 echo)
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
