#include "pch.h"
#include "PartyActor.h"

#include "ClientPacketHandler.MapChangeUtil.h"
#include "GameSessionManager.h"
#include "InstanceActor.h"
#include "InstanceManagerCore.h"
#include "Player.h"
#include "PlayerSession.h"
#include "RoomManager.h"
#include "GameRoom.h"

// 인스턴스 룸이 닫힐 때 호출되는 헬퍼 함수
// 굳이 없는 방을 GetOrCreate로 만들어서 닫을 필요는 없으니까 Find로 체크함
static void MarkInstanceRoomClosing(const InstanceManagerCore::InstanceInfo& closed)
{
    if (!GRoomManager) return;
    auto room = GRoomManager->FindRoom(closed.channelId, closed.mapId, closed.instanceId);
    if (room) room->MarkClosing(true);
}

namespace
{
	void MarkForceReturnAsync(uint64 playerId)
	{
		if (playerId == 0) return;
		PartyActor::Instance().Push([playerId]()
			{
				PartyActor::Instance().Core().MarkForceReturn(playerId);
			});
	}

	void RequestForceReturnToWorld(uint64 playerId, int64 expectedInstanceId)
	{
		if (playerId == 0 || expectedInstanceId == 0)
			return;

		if (!GameSessionManager::GSessionManager)
			return;

		auto session = GameSessionManager::GSessionManager->FindByPlayerId(playerId);
		if (!session)
		{
			MarkForceReturnAsync(playerId);
			return;
		}

		session->Post([playerId, expectedInstanceId](PlayerSessionRef self)
			{
				if (!self || self->IsMapChanging())
					return;

				RoomActorRef room = self->GetCurrentRoom_ActorOnly();
				auto gr = (room && room->GetKind() == RoomKind::Game)
					? std::dynamic_pointer_cast<GameRoom>(room)
					: nullptr;

				if (!gr)
					return;

				gr->PushJob([gr, self, playerId, expectedInstanceId]()
					{
						PlayerRef p = gr->FindPlayer_ActorOnly(playerId);
						if (!p)
							return;

						if (p->GetInstanceId() == 0 || p->GetInstanceId() != expectedInstanceId)
							return;

						int32 rm = 0;
						int64 ri = 0;
						Protocol::PositionInfo rp;
						MapChangeUtil::MakeSafeReturn(p, rm, ri, rp);

						int32 targetChannelId = p->GetChannelId();
						if (targetChannelId <= 0)
							targetChannelId = gr->GetChannelId();

						self->Post([playerId, targetChannelId, rm, ri, rp](PlayerSessionRef s) mutable
							{
								MapChangeUtil::SendMapChangeBegin(s, playerId, targetChannelId, rm, ri, rp);
							});
					});
			});
	}

	void RequestForceReturnToWorld(const HashSet<uint64>& members, int64 expectedInstanceId)
	{
		if (expectedInstanceId == 0)
			return;

		for (uint64 pid : members)
			RequestForceReturnToWorld(pid, expectedInstanceId);
	}
}

// 플레이어가 스스로 파티를 나갈 때의 처리
// 그냥 나가면 끝이 아니라, 던전 안에 있었다면 던전 처리까지 연쇄적으로 일어나야 함
void PartyActor::LeaveAndHandleInstance(uint64 playerId)
{
    auto& core = _core;

    const uint64 partyId = core.GetPartyIdByPlayerId(playerId);
    if (partyId == 0) return;

    // Leave 처리가 성공하면 메모리에서 정보가 날아가니까
    // 인스턴스 ID 같은 중요 메타데이터는 미리 스냅샷을 떠놔야 후처리가 가능함
    const PartyManagerCore::Party before = core.GetSnapshot(partyId);

    PartyManagerCore::Party after;
    bool disbanded = false;

    // 파티 로직 실행 (메모리 상에서 제거)
    if (!core.Leave(playerId, after, disbanded))
        return;

    // ==========================================================
    // [Case A] 던전 안에서 탈퇴한 경우
    // 파티에서는 나갔지만, 아직 물리적으로는 던전 맵 안에 있을 수 있음
    // InstanceActor에게 요청해서 해당 유저를 강제 퇴장(Eject) 시켜야 함
    // ==========================================================
    if (before.instanceId != 0)
    {
        const int64 instId = before.instanceId;
        const uint64 pid = playerId;
        const uint64 beforePartyId = before.partyId; // 람다 캡처용

        // 다른 액터(Instance)로 일감을 던짐 (비동기)
        InstanceActor::Instance().Push([instId, pid, beforePartyId]()
            {
                bool empty = false;
                // 인스턴스 관리자에게 멤버 제거 요청
                if (!InstanceActor::Instance().Core().EjectMember(instId, pid, empty))
                    return;

                // 만약 이 사람이 마지막 멤버였다면 방을 폭파해야 함
                if (empty)
                {
                    InstanceManagerCore::InstanceInfo closed;
                    if (InstanceActor::Instance().Core().CloseByInstanceId(instId, closed))
                    {
                        // 룸 매니저에게 방 닫힘 알림
                        MarkInstanceRoomClosing(closed);

                        // 다시 파티 액터로 돌아와서 파티 메타데이터 정리
                        // (던전 ID가 남아있으면 나중에 재입장할 때 꼬임)
                        PartyActor::Instance().Push([beforePartyId]()
                            {
                                PartyActor::Instance().Core().ClearPartyInstance(beforePartyId);
                            });
                    }
                }

                // 인스턴스에서 빠졌으면 마을로 강제 귀환 처리
                RequestForceReturnToWorld(pid, instId);
            });
    }

    // ==========================================================
    // [Case B] 전원 탈퇴로 인해 파티가 해산된 경우
    // 던전도 더 이상 유지할 필요가 없으므로 닫아야 함
    // ==========================================================
    if (disbanded && before.instanceId != 0)
    {
        const uint64 closedPartyId = before.partyId;

        InstanceActor::Instance().Push([closedPartyId]()
            {
                InstanceManagerCore::InstanceInfo closed;
                // 파티 ID 기준으로 인스턴스 폐쇄 요청
                if (InstanceActor::Instance().Core().CloseForParty(closedPartyId, closed))
                {
                    MarkInstanceRoomClosing(closed);

                    // 파티는 이미 사라졌지만 안전하게 메타 데이터 정리
                    PartyActor::Instance().Push([closedPartyId]()
                        {
                            PartyActor::Instance().Core().ClearPartyInstance(closedPartyId);
                        });

                    // 아직 맵에 남아있는 유저들이 있다면 강제 귀환 시켜야 함
                    RequestForceReturnToWorld(closed.members, closed.instanceId);
                }
            });
    }
}

// 파티장이 파티를 해산시켰을 때
void PartyActor::DisbandAndHandleInstance(uint64 leaderId)
{
    auto& core = _core;

    PartyManagerCore::Party disbandedParty;
    if (!core.Disband(leaderId, disbandedParty))
        return;

    // [Case C] 던전 도는 중에 해산하면 인스턴스도 같이 종료
    if (disbandedParty.instanceId != 0)
    {
        const uint64 closedPartyId = disbandedParty.partyId;

        InstanceActor::Instance().Push([closedPartyId]()
            {
                InstanceManagerCore::InstanceInfo closed;
                if (InstanceActor::Instance().Core().CloseForParty(closedPartyId, closed))
                {
                    MarkInstanceRoomClosing(closed);
                    RequestForceReturnToWorld(closed.members, closed.instanceId);
                }
            });
    }
}

// 파티장이 멤버를 강퇴했을 때
void PartyActor::KickAndHandleInstance(uint64 leaderId, uint64 targetId)
{
    auto& core = _core;

    const uint64 partyId = core.GetPartyIdByPlayerId(leaderId);
    if (partyId == 0) return;

    // 역시나 후처리를 위해 스냅샷 먼저
    const PartyManagerCore::Party before = core.GetSnapshot(partyId);

    PartyManagerCore::Party after;
    if (!core.Kick(leaderId, targetId, after))
        return;

    // [Case D] 던전 안에서 강퇴당하면 인스턴스에서도 쫓겨나야 함
    // 로직 자체는 자진 탈퇴(Leave)와 거의 유사함
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

                // 강퇴된 멤버는 마을로 강제 귀환 처리
                RequestForceReturnToWorld(pid, instId);
            });
    }
}
