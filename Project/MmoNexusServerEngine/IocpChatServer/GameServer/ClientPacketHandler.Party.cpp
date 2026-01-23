#include "pch.h"
#include "ClientPacketHandler.h"
#include "PlayerSession.h"
#include "Player.h"
#include "GameSessionManager.h"
#include "GameRoom.h" 
#include "RoomManager.h"
#include "PartyActor.h"
#include "PartyManagerCore.h"
#include "ClientPacketHandler.MapChangeUtil.h"

// 파티 정보를 클라이언트에게 보낼 패킷 형태로 변환하는 헬퍼 함수
static Protocol::S_PARTY_INFO_NTF MakePartyInfoNtf(const PartyManagerCore::Party& p)
{
	Protocol::S_PARTY_INFO_NTF ntf;
	ntf.set_hasparty(true);
	ntf.set_partyid(p.partyId);
	ntf.set_leaderid(p.leaderId);
	ntf.set_version(p.version);
	for (uint64 id : p.members)
		ntf.add_memberids(id);
	return ntf;
}

// 파티가 없는 상태(초기화)를 알리는 패킷 생성
static Protocol::S_PARTY_INFO_NTF MakeNoPartyInfoNtf()
{
	Protocol::S_PARTY_INFO_NTF ntf;
	ntf.set_hasparty(false);
	ntf.set_partyid(0);
	ntf.set_leaderid(0);
	ntf.set_version(0);
	return ntf;
}

// 특정 대상에게 파티 정보를 전송하는 유틸리티
// 어느 스레드에서 호출해도 안전하도록 PartyActor에게 작업을 위임하는 방식으로 구현함
// 조회는 PartyActor 스레드에서, 전송은 Session 스레드에서 수행하여 락을 최소화했다
static void SendPartyInfoTo(PlayerSessionRef target, uint64 partyId)
{
	if (!target) return;

	PartyActor::Instance().Push([target, partyId]()
		{
			auto& core = PartyActor::Instance().Core();

			Protocol::S_PARTY_INFO_NTF info;
			if (partyId == 0)
			{
				info = MakeNoPartyInfoNtf();
			}
			else
			{
				auto snap = core.GetSnapshot(partyId);
				info = (snap.partyId == 0) ? MakeNoPartyInfoNtf() : MakePartyInfoNtf(snap);
			}

			// 패킷 생성은 PartyActor에서 했지만, 실제 send는 세션의 컨텍스트로 넘겨서 처리
			target->Post([info](PlayerSessionRef self) mutable
				{
					self->Send(ClientPacketHandler::MakeSendBuffer(info));
				});
		});
}

// 파티원 전원에게 최신 파티 정보를 브로드캐스팅하는 함수
// 파티 상태가 변할 때마다(가입, 탈퇴 등) 호출되어 클라이언트와 싱크를 맞춘다
static void BroadcastPartyInfo(uint64 partyId)
{
	if (partyId == 0) return;

	PartyActor::Instance().Push([partyId]()
		{
			auto& core = PartyActor::Instance().Core();
			auto snap = core.GetSnapshot(partyId);
			if (snap.partyId == 0) return;

			Protocol::S_PARTY_INFO_NTF info = MakePartyInfoNtf(snap);

			// 멤버 목록을 순회하며 접속 중인 세션을 찾아 패킷을 보낸다
			for (uint64 id : snap.members)
			{
				auto s = GameSessionManager::GSessionManager->FindByPlayerId(id);
				if (!s) continue;

				s->Post([info](PlayerSessionRef self) mutable
					{
						self->Send(ClientPacketHandler::MakeSendBuffer(info));
					});
			}
		});
}

// 파티 채팅 요청 핸들러
// 파티원들은 각자 다른 맵(다른 스레드)에 있을 수 있으므로, PartyActor가 중계소 역할을 한다
bool ClientPacketHandler::Handle_C_PARTY_CHAT_REQ(PacketSessionRef& session, Protocol::C_PARTY_CHAT_REQ& pkt)
{
	PlayerSessionRef ps = static_pointer_cast<PlayerSession>(session);
	if (!ps) return true;

	const std::string msg = pkt.message();

	// Session에서는 Player 객체에 직접 접근하지 않고 ID만 가져온다 (댕글링 포인터 방지)
	const uint64 senderId = ps->GetPlayerId_AnyThread();
	if (senderId == 0) return true;

	// 보낸 사람 이름은 세션 매니저 캐시에서 빠르게 조회
	std::string senderName = GameSessionManager::GSessionManager->GetPlayerName(senderId);
	if (senderName.empty())
		senderName = "Unknown";

	// PartyActor에게 메시지 전파 작업을 맡김
	PartyActor::Instance().Push([senderId, senderName, msg]()
		{
			auto& core = PartyActor::Instance().Core();

			// 보낸 사람이 실제로 파티에 속해있는지 검증
			const uint64 partyId = core.GetPartyIdByPlayerId(senderId);
			if (partyId == 0) return;

			Vector<uint64> members;
			core.GetMembers(partyId, members);

			// 모든 파티원에게 메시지 전송
			for (uint64 memberId : members)
			{
				auto target = GameSessionManager::GSessionManager->FindByPlayerId(memberId);
				if (!target) continue;

				// 전송은 반드시 대상 세션의 Actor 컨텍스트 안에서 이루어져야 안전함
				target->Post([partyId, senderId, senderName, msg](PlayerSessionRef self) mutable
					{
						Protocol::S_PARTY_CHAT_NTF ntf;
						ntf.set_partyid(partyId);
						ntf.set_senderid(senderId);
						ntf.set_sendername(senderName);
						ntf.set_message(msg);

						self->Send(ClientPacketHandler::MakeSendBuffer(ntf));
					});
			}
		});

	return true;
}

// 파티 생성 요청 핸들러
bool ClientPacketHandler::Handle_C_PARTY_CREATE_REQ(PacketSessionRef& session, Protocol::C_PARTY_CREATE_REQ& pkt)
{
	PlayerSessionRef ps = static_pointer_cast<PlayerSession>(session);
	if (!ps) return true;

	if (ps->IsMapChanging())
		return true;

	const uint64 leaderId = ps->GetPlayerId_AnyThread();
	if (leaderId == 0) return true;

	PartyActor::Instance().Push([ps, leaderId]()
		{
			auto& core = PartyActor::Instance().Core();

			// 이미 파티에 속해있다면 생성 실패 처리
			if (core.GetPartyIdByPlayerId(leaderId) != 0)
			{
				ps->Post([](PlayerSessionRef self)
					{
						Protocol::S_PARTY_RESULT res;
						res.set_op(Protocol::PARTY_OP_CREATE);
						res.set_success(false);
						res.set_reason(Protocol::PARTY_REASON_ALREADY_IN_PARTY);
						self->Send(ClientPacketHandler::MakeSendBuffer(res));
					});
				return;
			}

			// 파티 생성 로직 수행
			uint64 partyId = 0;
			const bool ok = core.Create(leaderId, partyId);
			const uint32 version = ok ? core.GetSnapshot(partyId).version : 0;

			// 결과 전송
			ps->Post([ok, partyId, version](PlayerSessionRef self)
				{
					Protocol::S_PARTY_RESULT res;
					res.set_op(Protocol::PARTY_OP_CREATE);
					res.set_success(ok);
					res.set_reason(ok ? Protocol::PARTY_REASON_OK : Protocol::PARTY_REASON_INTERNAL_ERROR);
					res.set_partyid(partyId);
					res.set_version(version);
					self->Send(ClientPacketHandler::MakeSendBuffer(res));
				});

			// 성공했다면 파티 정보 갱신 (UI 업데이트용)
			if (ok)
				BroadcastPartyInfo(partyId);
		});

	return true;
}

// 파티 초대 요청 핸들러
bool ClientPacketHandler::Handle_C_PARTY_INVITE_REQ(PacketSessionRef& session, Protocol::C_PARTY_INVITE_REQ& pkt)
{
	PlayerSessionRef ps = static_pointer_cast<PlayerSession>(session);
	if (!ps) return true;

	if (ps->IsMapChanging())
		return true;

	const uint64 inviterId = ps->GetPlayerId_AnyThread();
	if (inviterId == 0) return true;

	const uint64 targetId = pkt.targetplayerid();

	std::string inviterName = GameSessionManager::GSessionManager->GetPlayerName(inviterId);
	if (inviterName.empty()) inviterName = "Unknown";

	// 자기 자신 초대 방지
	if (targetId == 0 || targetId == inviterId)
	{
		ps->Post([](PlayerSessionRef self)
			{
				Protocol::S_PARTY_RESULT res;
				res.set_op(Protocol::PARTY_OP_INVITE);
				res.set_success(false);
				res.set_reason(Protocol::PARTY_REASON_SELF_TARGET);
				self->Send(ClientPacketHandler::MakeSendBuffer(res));
			});
		return true;
	}

	// 대상이 오프라인이면 실패
	auto targetSession = GameSessionManager::GSessionManager->FindByPlayerId(targetId);
	if (!targetSession)
	{
		ps->Post([](PlayerSessionRef self)
			{
				Protocol::S_PARTY_RESULT res;
				res.set_op(Protocol::PARTY_OP_INVITE);
				res.set_success(false);
				res.set_reason(Protocol::PARTY_REASON_NO_TARGET);
				self->Send(ClientPacketHandler::MakeSendBuffer(res));
			});
		return true;
	}

	// 실제 초대 로직은 PartyActor에서 순차적으로 처리
	PartyActor::Instance().Push([ps, targetSession, inviterId, inviterName, targetId]()
		{
			auto& core = PartyActor::Instance().Core();

			const uint64 partyId = core.GetPartyIdByPlayerId(inviterId);
			if (partyId == 0)
			{
				ps->Post([](PlayerSessionRef self)
					{
						Protocol::S_PARTY_RESULT res;
						res.set_op(Protocol::PARTY_OP_INVITE);
						res.set_success(false);
						res.set_reason(Protocol::PARTY_REASON_NOT_IN_PARTY);
						self->Send(ClientPacketHandler::MakeSendBuffer(res));
					});
				return;
			}

			// 보류 중인 초대 목록에 추가 (타임아웃 등은 Core에서 관리)
			PartyManagerCore::PendingInvite inv;
			const bool ok = core.Invite(inviterId, targetId, inv);

			uint32 version = 0;
			{
				auto snap = core.GetSnapshot(partyId);
				version = (snap.partyId != 0) ? snap.version : 0;
			}

			// 초대자에게 결과 알림
			ps->Post([ok, partyId, version](PlayerSessionRef self)
				{
					Protocol::S_PARTY_RESULT res;
					res.set_op(Protocol::PARTY_OP_INVITE);
					res.set_success(ok);
					res.set_reason(ok ? Protocol::PARTY_REASON_OK : Protocol::PARTY_REASON_ALREADY_IN_PARTY);
					res.set_partyid(partyId);
					res.set_version(version);
					self->Send(ClientPacketHandler::MakeSendBuffer(res));
				});

			if (!ok) return;

			// 초대받은 사람에게 알림 팝업 전송
			targetSession->Post([partyId, inviterId, inviterName](PlayerSessionRef self) mutable
				{
					Protocol::S_PARTY_INVITE_NTF ntf;
					ntf.set_partyid(partyId);
					ntf.set_inviterid(inviterId);
					ntf.set_invitername(inviterName);
					self->Send(ClientPacketHandler::MakeSendBuffer(ntf));
				});
		});

	return true;
}

// 파티 초대 수락/거절 핸들러
bool ClientPacketHandler::Handle_C_PARTY_INVITE_ACCEPT_REQ(PacketSessionRef& session, Protocol::C_PARTY_INVITE_ACCEPT_REQ& pkt)
{
	PlayerSessionRef ps = static_pointer_cast<PlayerSession>(session);
	if (!ps) return true;

	const uint64 targetId = ps->GetPlayerId_AnyThread();
	if (targetId == 0) return true;

	const uint64 partyId = pkt.partyid();
	const bool accept = pkt.accept();

	PartyActor::Instance().Push([ps, targetId, partyId, accept]()
		{
			auto& core = PartyActor::Instance().Core();

			PartyManagerCore::Party after;
			// 수락 시 멤버 추가 처리, 거절 시 펜딩 목록에서 제거
			const bool ok = core.AcceptInvite(targetId, partyId, accept, after);

			const uint64 afterPartyId = after.partyId;
			const uint32 version = afterPartyId ? after.version : 0;

			// 응답 패킷 전송
			ps->Post([ok, partyId, accept, version](PlayerSessionRef self)
				{
					Protocol::S_PARTY_RESULT res;
					res.set_op(accept ? Protocol::PARTY_OP_INVITE_ACCEPT : Protocol::PARTY_OP_INVITE_REJECT);
					res.set_success(ok);
					res.set_reason(ok
						? (accept ? Protocol::PARTY_REASON_OK : Protocol::PARTY_REASON_REJECTED)
						: Protocol::PARTY_REASON_INVALID_PARTY);
					res.set_partyid(partyId);
					res.set_version(version);
					self->Send(ClientPacketHandler::MakeSendBuffer(res));
				});

			if (!ok) return;

			// 수락해서 파티원이 되었다면 전체 멤버에게 갱신 알림
			if (accept && afterPartyId != 0)
			{
				BroadcastPartyInfo(afterPartyId);
				return;
			}

			// 거절했다면 본인의 파티 UI를 초기화 (혹시 잔상이 남았을 경우 대비)
			if (!accept)
			{
				const uint64 curPartyId = core.GetPartyIdByPlayerId(targetId);
				SendPartyInfoTo(ps, curPartyId);
			}
		});

	return true;
}

// 파티 탈퇴 요청 핸들러
// 단순 탈퇴뿐만 아니라 던전 인스턴스 처리까지 포함된 중요 로직
bool ClientPacketHandler::Handle_C_PARTY_LEAVE_REQ(PacketSessionRef& session, Protocol::C_PARTY_LEAVE_REQ& pkt)
{
	PlayerSessionRef ps = static_pointer_cast<PlayerSession>(session);
	if (!ps) return true;
	if (ps->IsMapChanging()) return true;

	const uint64 pid = ps->GetPlayerId_AnyThread();
	if (pid == 0) return true;

	PartyActor::Instance().Push([ps, pid]()
		{
			auto& core = PartyActor::Instance().Core();

			const uint64 partyId = core.GetPartyIdByPlayerId(pid);
			if (partyId == 0)
			{
				// 파티가 없는데 탈퇴하려는 경우 예외 처리
				ps->Post([](PlayerSessionRef self)
					{
						Protocol::S_PARTY_RESULT res;
						res.set_op(Protocol::PARTY_OP_LEAVE);
						res.set_success(false);
						res.set_reason(Protocol::PARTY_REASON_NOT_IN_PARTY);
						res.set_partyid(0);
						res.set_version(0);
						self->Send(ClientPacketHandler::MakeSendBuffer(res));
					});
				return;
			}

			const PartyManagerCore::Party before = core.GetSnapshot(partyId);
			const bool wasInDungeon = (before.instanceId != 0);

			// 던전 진행 중에 탈퇴하면 인스턴스에서도 나가야 함
			// PartyActor 내부에서 InstanceActor와 통신하여 적절한 정리를 수행한다
			PartyActor::Instance().LeaveAndHandleInstance(pid);

			// 매핑이 제거되었는지 확인하여 성공 여부 판단
			const bool ok = (core.GetPartyIdByPlayerId(pid) == 0);

			uint32 version = 0;
			{
				PartyManagerCore::Party afterSnap = core.GetSnapshot(partyId);
				version = (afterSnap.partyId != 0) ? afterSnap.version : 0;
			}

			ps->Post([ok, partyId, version](PlayerSessionRef self)
				{
					Protocol::S_PARTY_RESULT res;
					res.set_op(Protocol::PARTY_OP_LEAVE);
					res.set_success(ok);
					res.set_reason(ok ? Protocol::PARTY_REASON_OK : Protocol::PARTY_REASON_INTERNAL_ERROR);
					res.set_partyid(partyId);
					res.set_version(version);
					self->Send(ClientPacketHandler::MakeSendBuffer(res));
				});

			if (!ok) return;

			// 떠난 사람은 파티 정보 초기화
			SendPartyInfoTo(ps, 0);

			// 남은 파티원들에게 멤버 변경 알림
			PartyManagerCore::Party afterSnap = core.GetSnapshot(partyId);
			if (afterSnap.partyId != 0)
				BroadcastPartyInfo(partyId);

			// 던전에서 탈퇴했다면 강제로 마을로 귀환시킴
			if (wasInDungeon)
				MapChangeUtil::ForceReturnToWorld(ps);
		});

	return true;
}

// 파티장 강퇴(Kick) 요청 핸들러
bool ClientPacketHandler::Handle_C_PARTY_KICK_REQ(PacketSessionRef& session, Protocol::C_PARTY_KICK_REQ& pkt)
{
	PlayerSessionRef ps = static_pointer_cast<PlayerSession>(session);
	if (!ps) return true;
	if (ps->IsMapChanging()) return true;

	const uint64 leaderId = ps->GetPlayerId_AnyThread();
	if (leaderId == 0) return true;

	const uint64 targetId = pkt.targetplayerid();

	if (targetId == 0 || targetId == leaderId)
	{
		ps->Post([](PlayerSessionRef self)
			{
				Protocol::S_PARTY_RESULT res;
				res.set_op((int32)Protocol::PARTY_OP_KICK);
				res.set_success(false);
				res.set_reason((int32)Protocol::PARTY_REASON_SELF_TARGET);
				self->Send(ClientPacketHandler::MakeSendBuffer(res));
			});
		return true;
	}

	PartyActor::Instance().Push([ps, leaderId, targetId]()
		{
			auto& core = PartyActor::Instance().Core();

			const uint64 partyId = core.GetPartyIdByPlayerId(leaderId);
			if (partyId == 0)
			{
				ps->Post([](PlayerSessionRef self)
					{
						Protocol::S_PARTY_RESULT res;
						res.set_op((int32)Protocol::PARTY_OP_KICK);
						res.set_success(false);
						res.set_reason((int32)Protocol::PARTY_REASON_NOT_IN_PARTY);
						self->Send(ClientPacketHandler::MakeSendBuffer(res));
					});
				return;
			}

			const PartyManagerCore::Party before = core.GetSnapshot(partyId);
			const bool wasInDungeon = (before.instanceId != 0);

			const bool wasMember = core.IsMember(partyId, targetId);

			// 강퇴 역시 던전 인스턴스 처리가 필요함
			// Kick 함수 내부에서 권한 검사 및 인스턴스 정리 로직이 돈다
			PartyActor::Instance().KickAndHandleInstance(leaderId, targetId);

			const bool isMemberNow = core.IsMember(partyId, targetId);
			const bool ok = (wasMember && !isMemberNow);

			uint32 version = 0;
			{
				PartyManagerCore::Party afterSnap = core.GetSnapshot(partyId);
				version = (afterSnap.partyId != 0) ? afterSnap.version : 0;
			}

			// 리더에게 결과 전송
			ps->Post([ok, partyId, version](PlayerSessionRef self)
				{
					Protocol::S_PARTY_RESULT res;
					res.set_op((int32)Protocol::PARTY_OP_KICK);
					res.set_success(ok);
					res.set_reason(ok ? (int32)Protocol::PARTY_REASON_OK
						: (int32)Protocol::PARTY_REASON_INTERNAL_ERROR);
					res.set_partyid(partyId);
					res.set_version(version);
					self->Send(ClientPacketHandler::MakeSendBuffer(res));
				});

			if (!ok) return;

			// 남은 멤버들에게 알림
			PartyManagerCore::Party afterSnap = core.GetSnapshot(partyId);
			if (afterSnap.partyId != 0)
				BroadcastPartyInfo(partyId);

			// 강퇴당한 사람에게 알림 (접속 중이라면)
			auto kicked = GameSessionManager::GSessionManager->FindByPlayerId(targetId);
			if (kicked)
			{
				kicked->Post([](PlayerSessionRef self)
					{
						Protocol::S_PARTY_INFO_NTF off = MakeNoPartyInfoNtf();
						self->Send(ClientPacketHandler::MakeSendBuffer(off));
					});
			}

			// 던전에서 강퇴당했으면 강제 귀환 처리
			if (wasInDungeon)
			{
				if (kicked)
				{
					MapChangeUtil::ForceReturnToWorld(kicked);
				}
				else
				{
					// 오프라인 상태라면 DB나 플래그에 표시해서 다음에 접속할 때 마을로 보내야 함
					core.MarkForceReturn(targetId);
				}
			}
		});

	return true;
}

// 파티 해산 요청 핸들러
bool ClientPacketHandler::Handle_C_PARTY_DISBAND_REQ(PacketSessionRef& session, Protocol::C_PARTY_DISBAND_REQ& pkt)
{
	PlayerSessionRef ps = static_pointer_cast<PlayerSession>(session);
	if (!ps) return true;
	if (ps->IsMapChanging()) return true;

	const uint64 leaderId = ps->GetPlayerId_AnyThread();
	if (leaderId == 0) return true;

	PartyActor::Instance().Push([ps, leaderId]()
		{
			auto& core = PartyActor::Instance().Core();

			const uint64 partyId = core.GetPartyIdByPlayerId(leaderId);
			if (partyId == 0)
			{
				ps->Post([](PlayerSessionRef self)
					{
						Protocol::S_PARTY_RESULT res;
						res.set_op((int32)Protocol::PARTY_OP_DISBAND);
						res.set_success(false);
						res.set_reason((int32)Protocol::PARTY_REASON_NOT_IN_PARTY);
						self->Send(ClientPacketHandler::MakeSendBuffer(res));
					});
				return;
			}

			// 해산하면 파티 정보가 날아가므로 미리 멤버 목록 백업
			const PartyManagerCore::Party before = core.GetSnapshot(partyId);

			Vector<uint64> members;
			members.reserve(before.members.size());
			for (uint64 id : before.members) members.push_back(id);

			const bool wasInDungeon = (before.instanceId != 0);

			// 해산 및 인스턴스 종료 처리
			// 던전 인스턴스도 함께 닫히도록 유도한다
			PartyActor::Instance().DisbandAndHandleInstance(leaderId);

			const bool ok = (core.GetPartyIdByPlayerId(leaderId) == 0);

			ps->Post([ok, partyId](PlayerSessionRef self)
				{
					Protocol::S_PARTY_RESULT res;
					res.set_op((int32)Protocol::PARTY_OP_DISBAND);
					res.set_success(ok);
					res.set_reason(ok ? (int32)Protocol::PARTY_REASON_OK
						: (int32)Protocol::PARTY_REASON_INTERNAL_ERROR);
					res.set_partyid(partyId);
					res.set_version(0);
					self->Send(ClientPacketHandler::MakeSendBuffer(res));
				});

			if (!ok) return;

			// 전원에게 파티 해제 알림 및 던전 강제 귀환 수행
			for (uint64 id : members)
			{
				auto ms = GameSessionManager::GSessionManager->FindByPlayerId(id);
				if (ms)
				{
					ms->Post([](PlayerSessionRef self)
						{
							Protocol::S_PARTY_INFO_NTF off = MakeNoPartyInfoNtf();
							self->Send(ClientPacketHandler::MakeSendBuffer(off));
						});

					if (wasInDungeon)
						MapChangeUtil::ForceReturnToWorld(ms);
				}
				else
				{
					if (wasInDungeon)
						core.MarkForceReturn(id);
				}
			}
		});

	return true;
}


// 파티원들의 상태 정보를 수집하기 위한 임시 구조체
struct PartyStatusItem
{
	uint64 playerId = 0;
	uint64 objectId = 0;
	std::string name;

	int32 level = 1;
	int32 hp = 0;
	int32 maxHp = 0;

	int32 mapId = 0;
	int32 channelId = 0;

	Protocol::PositionInfo pos;
};

// Fan-out / Fan-in 패턴을 구현하기 위한 수집기 객체
// 여러 스레드(각 파티원의 RoomActor)에서 응답이 올 때마다 카운트를 줄이고
// 마지막 응답이 오면 결과를 취합해서 보낸다.
struct PartyStatusCollector
{
	uint64 partyId = 0;
	uint32 version = 0;

	int32 remaining = 0;                 // 응답 대기 중인 파티원 수
	Vector<PartyStatusItem> items;       // 수집된 정보 (경합 방지를 위해 요청자 세션 스레드에서만 접근)
};


// 파티원 상태 조회 핸들러 (HP, 위치 등 실시간 정보)
// 플레이어 정보는 각 RoomActor 스레드에 흩어져 있으므로, 이를 안전하게 긁어모으는 것이 핵심
bool ClientPacketHandler::Handle_C_PARTY_STATUS_REQ(PacketSessionRef& session, Protocol::C_PARTY_STATUS_REQ& pkt)
{
	PlayerSessionRef ps = static_pointer_cast<PlayerSession>(session);
	if (!ps) return true;

	if (ps->IsMapChanging())
		return true;

	const uint64 myId = ps->GetPlayerId_AnyThread();
	if (myId == 0)
	{
		// 플레이어 ID가 없으면 빈 정보 리턴 (방어 코드)
		ps->Post([](PlayerSessionRef self)
			{
				Protocol::S_PARTY_STATUS_NTF ntf;
				ntf.set_partyid(0);
				ntf.set_version(0);
				self->Send(ClientPacketHandler::MakeSendBuffer(ntf));
			});
		return true;
	}

	// 1단계: PartyActor에서 파티 멤버 목록 스냅샷을 가져옴
	PartyActor::Instance().Push([ps, myId]()
		{
			auto& core = PartyActor::Instance().Core();

			const uint64 partyId = core.GetPartyIdByPlayerId(myId);
			if (partyId == 0)
			{
				ps->Post([](PlayerSessionRef self)
					{
						Protocol::S_PARTY_STATUS_NTF ntf;
						ntf.set_partyid(0);
						ntf.set_version(0);
						self->Send(ClientPacketHandler::MakeSendBuffer(ntf));
					});
				return;
			}

			const PartyManagerCore::Party snap = core.GetSnapshot(partyId);
			const uint32 version = snap.version;

			Vector<uint64> memberIds;
			memberIds.reserve(snap.members.size());
			for (uint64 id : snap.members)
				memberIds.push_back(id);

			// 2단계: 요청자의 세션 Actor로 돌아와서 Fan-out 준비
			// 여기서부터 분산된 요청을 시작한다
			ps->Post([partyId, version, memberIds = std::move(memberIds)](PlayerSessionRef requester) mutable
				{
					struct Target
					{
						uint64 playerId = 0;
						PlayerSessionRef session;
					};

					Vector<Target> targets;
					targets.reserve(memberIds.size());

					// 접속 중인 멤버들의 세션을 찾는다
					for (uint64 id : memberIds)
					{
						auto ms = GameSessionManager::GSessionManager->FindByPlayerId(id);
						if (ms)
							targets.push_back(Target{ id, ms });
					}

					// 수집기(Collector) 생성 - shared_ptr로 생명주기 관리
					auto collector = MakeShared<PartyStatusCollector>();
					collector->partyId = partyId;
					collector->version = version;
					collector->remaining = static_cast<int32>(targets.size());
					collector->items.reserve(targets.size());

					if (collector->remaining == 0)
					{
						Protocol::S_PARTY_STATUS_NTF ntf;
						ntf.set_partyid(partyId);
						ntf.set_version(version);
						requester->Send(ClientPacketHandler::MakeSendBuffer(ntf));
						return;
					}

					// 3단계: 각 멤버의 세션으로 "너네 방(RoomActor)에 가서 상태 좀 읽어와" 요청 전송
					for (const Target& t : targets)
					{
						const uint64 memberId = t.playerId;
						PlayerSessionRef memberSession = t.session;

						memberSession->Post([requester, collector, memberId](PlayerSessionRef ms)
							{
								// 맵 이동 중이면 정보를 읽을 수 없으므로 스킵 처리
								if (ms->IsMapChanging())
								{
									// 스킵하더라도 collector 카운트는 줄여줘야 함 (안 그러면 무한 대기)
									requester->Post([collector](PlayerSessionRef req) mutable
										{
											collector->remaining--;
											if (collector->remaining == 0)
											{
												// 마지막 응답이면 결과 전송 (아래 로직과 중복되지만 안전을 위해)
												Protocol::S_PARTY_STATUS_NTF ntf;
												ntf.set_partyid(collector->partyId);
												ntf.set_version(collector->version);
												for (auto& it : collector->items)
												{
													auto* st = ntf.add_members();
													st->set_playerid(it.playerId);
													st->set_objectid(it.objectId);
													st->set_name(it.name);
													st->set_level(it.level);
													st->set_hp(it.hp);
													st->set_maxhp(it.maxHp);
													st->set_mapid(it.mapId);
													st->set_channelid(it.channelId);
													st->mutable_posinfo()->CopyFrom(it.pos);
												}
												req->Send(ClientPacketHandler::MakeSendBuffer(ntf));
											}
										});
									return;
								}

								RoomActorRef room = ms->GetCurrentRoom_ActorOnly();
								if (!room || room->GetKind() != RoomKind::Game)
								{
									// 방이 없거나 로비면 정보 수집 불가 -> 빈 응답 처리
									requester->Post([collector](PlayerSessionRef req) mutable
										{
											collector->remaining--;
											if (collector->remaining == 0)
											{
												Protocol::S_PARTY_STATUS_NTF ntf;
												ntf.set_partyid(collector->partyId);
												ntf.set_version(collector->version);
												for (auto& it : collector->items)
												{
													auto* st = ntf.add_members();
													st->set_playerid(it.playerId);
													st->set_objectid(it.objectId);
													st->set_name(it.name);
													st->set_level(it.level);
													st->set_hp(it.hp);
													st->set_maxhp(it.maxHp);
													st->set_mapid(it.mapId);
													st->set_channelid(it.channelId);
													st->mutable_posinfo()->CopyFrom(it.pos);
												}
												req->Send(ClientPacketHandler::MakeSendBuffer(ntf));
											}
										});
									return;
								}

								auto gr = std::dynamic_pointer_cast<GameRoom>(room);
								if (!gr)
								{
									requester->Post([collector](PlayerSessionRef req) mutable
										{
											collector->remaining--;
											if (collector->remaining == 0)
											{
												Protocol::S_PARTY_STATUS_NTF ntf;
												ntf.set_partyid(collector->partyId);
												ntf.set_version(collector->version);
												for (auto& it : collector->items)
												{
													auto* st = ntf.add_members();
													st->set_playerid(it.playerId);
													st->set_objectid(it.objectId);
													st->set_name(it.name);
													st->set_level(it.level);
													st->set_hp(it.hp);
													st->set_maxhp(it.maxHp);
													st->set_mapid(it.mapId);
													st->set_channelid(it.channelId);
													st->set_channelid(it.channelId);
													st->mutable_posinfo()->CopyFrom(it.pos);
												}
												req->Send(ClientPacketHandler::MakeSendBuffer(ntf));
											}
										});
									return;
								}

								// 4단계: 실제 Player 객체가 사는 GameRoom 스레드로 진입
								// 여기서만 Player의 HP, 위치 등을 안전하게 읽을 수 있음 (Lock Free 보장)
								gr->PushJob([gr, requester, collector, memberId]()
									{
										PartyStatusItem item;
										bool has = false;

										PlayerRef mp = gr->FindPlayer_ActorOnly(memberId);
										if (mp)
										{
											has = true;
											item.playerId = mp->GetPlayerId();
											item.objectId = mp->GetObjectId();
											item.name = mp->GetName();

											if (auto si = mp->GetStatInfo())
											{
												item.level = si->level();
												item.hp = si->hp();
												item.maxHp = si->maxhp();
											}

											item.mapId = mp->GetMapId();
											item.channelId = mp->GetChannelId();

											if (auto pi = mp->GetPosInfo())
												item.pos.CopyFrom(*pi);
										}

										// 5단계: 수집된 정보를 들고 다시 요청자의 세션 Actor로 복귀
										// Fan-in: 여기서 결과를 하나씩 취합한다
										requester->Post([collector, has, item = std::move(item)](PlayerSessionRef req) mutable
											{
												if (has)
													collector->items.push_back(std::move(item));

												collector->remaining--;

												// 모든 파티원의 응답이 도착했으면 최종 패킷 전송
												if (collector->remaining == 0)
												{
													Protocol::S_PARTY_STATUS_NTF ntf;
													ntf.set_partyid(collector->partyId);
													ntf.set_version(collector->version);

													for (auto& it : collector->items)
													{
														auto* st = ntf.add_members();
														st->set_playerid(it.playerId);
														st->set_objectid(it.objectId);
														st->set_name(it.name);

														st->set_level(it.level);
														st->set_hp(it.hp);
														st->set_maxhp(it.maxHp);

														st->set_mapid(it.mapId);
														st->set_channelid(it.channelId);

														st->mutable_posinfo()->CopyFrom(it.pos);
													}

													req->Send(ClientPacketHandler::MakeSendBuffer(ntf));
												}
											});
									});
							});
					}
				});
		});

	return true;
}