#include "pch.h"
#include "PartyActor.h"

#include "InstanceActor.h"
#include "InstanceManagerCore.h"
#include "RoomManager.h"
#include "GameRoom.h"

// room 생성하면 안 되니까 FindRoom 필요 (전에 추가하라고 했던 그거)
static void MarkInstanceRoomClosing(const InstanceManagerCore::InstanceInfo& closed)
{
    if (!GRoomManager) return;
    auto room = GRoomManager->FindRoom(closed.channelId, closed.mapId, closed.instanceId);
    if (room) room->MarkClosing(true);
}

void PartyActor::LeaveAndHandleInstance(uint64 playerId)
{
    auto& core = _core;

    const uint64 partyId = core.GetPartyIdByPlayerId(playerId);
    if (partyId == 0) return;

    // ✅ Leave 전에 스냅샷(여기서 instanceId 확인)
    const PartyManagerCore::Party before = core.GetSnapshot(partyId);

    PartyManagerCore::Party after;
    bool disbanded = false;

    // ✅ 여기서 성공/실패 확정
    if (!core.Leave(playerId, after, disbanded))
        return;

    // ==========================================================
    // 6-A) 던전 안에서 Leave 성공 -> 인스턴스에서 제거(강제 퇴출 트리거)
    //      + 마지막 1명이면 인스턴스 자동 종료(B)
    // ==========================================================
    if (before.instanceId != 0)
    {
        const int64 instId = before.instanceId;
        const uint64 pid = playerId;
        const uint64 beforePartyId = before.partyId; // 캡처용

        InstanceActor::Instance().Push([instId, pid, beforePartyId]()
            {
                bool empty = false;
                if (!InstanceActor::Instance().Core().EjectMember(instId, pid, empty))
                    return;

                if (empty)
                {
                    InstanceManagerCore::InstanceInfo closed;
                    if (InstanceActor::Instance().Core().CloseByInstanceId(instId, closed))
                    {
                        MarkInstanceRoomClosing(closed);

                        // ✅ 파티 메타도 정리 (던전 stuck 방지)
                        PartyActor::Instance().Push([beforePartyId]()
                            {
                                PartyActor::Instance().Core().ClearPartyInstance(beforePartyId);
                            });
                    }
                }

                // TODO: ForceReturnToTown(pid, instId);
            });
    }

    // ==========================================================
    // 6-B) 전원 Leave로 파티가 해산 -> 인스턴스 자동 Close
    // ==========================================================
    if (disbanded && before.instanceId != 0)
    {
        const uint64 closedPartyId = before.partyId;

        InstanceActor::Instance().Push([closedPartyId]()
            {
                InstanceManagerCore::InstanceInfo closed;
                if (InstanceActor::Instance().Core().CloseForParty(closedPartyId, closed))
                {
                    MarkInstanceRoomClosing(closed);

                    // ✅ 파티 메타 정리(이미 파티는 없어졌지만, 안전)
                    PartyActor::Instance().Push([closedPartyId]()
                        {
                            PartyActor::Instance().Core().ClearPartyInstance(closedPartyId);
                        });

                    // TODO: 남은 멤버들 강제 퇴출(마을) 브로드캐스트/맵체인지
                }
            });
    }
}
void PartyActor::DisbandAndHandleInstance(uint64 leaderId)
{
    auto& core = _core;

    PartyManagerCore::Party disbandedParty;
    if (!core.Disband(leaderId, disbandedParty))
        return;

    // 6-C) 파티 해산 -> 인스턴스도 자동 Close
    if (disbandedParty.instanceId != 0)
    {
        const uint64 closedPartyId = disbandedParty.partyId;

        InstanceActor::Instance().Push([closedPartyId]()
            {
                InstanceManagerCore::InstanceInfo closed;
                if (InstanceActor::Instance().Core().CloseForParty(closedPartyId, closed))
                {
                    MarkInstanceRoomClosing(closed);
                    // TODO: 멤버들 강제 퇴출(마을) 연결
                }
            });
    }
}

void PartyActor::KickAndHandleInstance(uint64 leaderId, uint64 targetId)
{
    auto& core = _core;

    const uint64 partyId = core.GetPartyIdByPlayerId(leaderId);
    if (partyId == 0) return;

    const PartyManagerCore::Party before = core.GetSnapshot(partyId);

    PartyManagerCore::Party after;
    if (!core.Kick(leaderId, targetId, after))
        return;

    // 6-D) 던전 안에서 Kick 성공 -> 인스턴스에서 제거(강제 퇴출 트리거)
    //      + 마지막 1명이면 인스턴스 자동 종료(B)
    if (before.instanceId != 0)
    {
        const int64 instId = before.instanceId;
        const uint64 pid = targetId;
        const uint64 beforePartyId = before.partyId;

        InstanceActor::Instance().Push([instId, pid, beforePartyId]()
            {
                bool empty = false;
                if (!InstanceActor::Instance().Core().EjectMember(instId, pid, empty))
                    return;

                if (empty)
                {
                    InstanceManagerCore::InstanceInfo closed;
                    if (InstanceActor::Instance().Core().CloseByInstanceId(instId, closed))
                    {
                        MarkInstanceRoomClosing(closed);

                        PartyActor::Instance().Push([beforePartyId]()
                            {
                                PartyActor::Instance().Core().ClearPartyInstance(beforePartyId);
                            });
                    }
                }

                // TODO: ForceReturnToTown(pid, instId);
            });
    }
}