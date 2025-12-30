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

static Protocol::S_PARTY_INFO_NTF MakeNoPartyInfoNtf()
{
	Protocol::S_PARTY_INFO_NTF ntf;
	ntf.set_hasparty(false);
	ntf.set_partyid(0);
	ntf.set_leaderid(0);
	ntf.set_version(0);
	return ntf;
}

// ✅ 어디서 호출해도 안전: 조회는 PartyActor에서, 전송은 세션 Post로
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

			target->Post([info](PlayerSessionRef self) mutable
				{
					self->Send(ClientPacketHandler::MakeSendBuffer(info));
				});
		});
}

// ✅ 어디서 호출해도 안전: 조회는 PartyActor에서, 전송은 각 세션 Post로
static void BroadcastPartyInfo(uint64 partyId)
{
	if (partyId == 0) return;

	PartyActor::Instance().Push([partyId]()
		{
			auto& core = PartyActor::Instance().Core();
			auto snap = core.GetSnapshot(partyId);
			if (snap.partyId == 0) return;

			Protocol::S_PARTY_INFO_NTF info = MakePartyInfoNtf(snap);

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

bool ClientPacketHandler::Handle_C_PARTY_CHAT_REQ(PacketSessionRef& session, Protocol::C_PARTY_CHAT_REQ& pkt)
{
	PlayerSessionRef ps = static_pointer_cast<PlayerSession>(session);
	if (!ps) return true;

	const std::string msg = pkt.message();

	// ✅ Session은 PlayerRef 금지. ID만.
	const uint64 senderId = ps->GetPlayerId_AnyThread();
	if (senderId == 0) return true;

	// ✅ Option A: GameSessionManager 캐시에서 이름 조회
	std::string senderName = GameSessionManager::GSessionManager->GetPlayerName(senderId);
	if (senderName.empty())
		senderName = "Unknown"; // (원하면 std::to_string(senderId)로)

	PartyActor::Instance().Push([senderId, senderName, msg]()
		{
			auto& core = PartyActor::Instance().Core();

			const uint64 partyId = core.GetPartyIdByPlayerId(senderId);
			if (partyId == 0) return;

			std::vector<uint64> members;
			core.GetMembers(partyId, members);

			for (uint64 memberId : members)
			{
				auto target = GameSessionManager::GSessionManager->FindByPlayerId(memberId);
				if (!target) continue;

				// ✅ 전송은 "대상 세션 Actor"에서만
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

			uint64 partyId = 0;
			const bool ok = core.Create(leaderId, partyId);
			const uint32 version = ok ? core.GetSnapshot(partyId).version : 0;

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

			if (ok)
				BroadcastPartyInfo(partyId);
		});

	return true;
}

bool ClientPacketHandler::Handle_C_PARTY_INVITE_REQ(PacketSessionRef& session, Protocol::C_PARTY_INVITE_REQ& pkt)
{
	PlayerSessionRef ps = static_pointer_cast<PlayerSession>(session);
	if (!ps) return true;

	if (ps->IsMapChanging())
		return true;

	const uint64 inviterId = ps->GetPlayerId_AnyThread();
	if (inviterId == 0) return true;

	const uint64 targetId = pkt.targetplayerid();

	// ✅ inviterName: Option A 캐시
	std::string inviterName = GameSessionManager::GSessionManager->GetPlayerName(inviterId);
	if (inviterName.empty()) inviterName = "Unknown";

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

			PartyManagerCore::PendingInvite inv;
			const bool ok = core.Invite(inviterId, targetId, inv);

			// version은 party가 살아있을 때만
			uint32 version = 0;
			{
				auto snap = core.GetSnapshot(partyId);
				version = (snap.partyId != 0) ? snap.version : 0;
			}

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
			const bool ok = core.AcceptInvite(targetId, partyId, accept, after);

			const uint64 afterPartyId = after.partyId;
			const uint32 version = afterPartyId ? after.version : 0;

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

			if (accept && afterPartyId != 0)
			{
				BroadcastPartyInfo(afterPartyId);
				return;
			}

			// reject면 내 파티정보 갱신(보통 0)
			if (!accept)
			{
				const uint64 curPartyId = core.GetPartyIdByPlayerId(targetId);
				SendPartyInfoTo(ps, curPartyId);
			}
		});

	return true;
}

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

			// ✅ Leave + 던전이면 Instance eject/close/roomClosing 처리까지
			PartyActor::Instance().LeaveAndHandleInstance(pid);

			// ✅ 성공 판정: 매핑 제거됐으면 OK
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

			// 떠난 사람: 파티 없음
			SendPartyInfoTo(ps, 0);

			// 남은 파티원 갱신(파티가 아직 존재하면)
			PartyManagerCore::Party afterSnap = core.GetSnapshot(partyId);
			if (afterSnap.partyId != 0)
				BroadcastPartyInfo(partyId);

			// ✅ 던전 내 Leave면 즉시 강제 복귀
			if (wasInDungeon)
				MapChangeUtil::ForceReturnToWorld(ps);
		});

	return true;
}

bool ClientPacketHandler::Handle_C_PARTY_KICK_REQ(PacketSessionRef& session, Protocol::C_PARTY_KICK_REQ& pkt)
{
	PlayerSessionRef ps = static_pointer_cast<PlayerSession>(session);
	if (!ps) return true;
	if (ps->IsMapChanging()) return true;

	const uint64 leaderId = ps->GetPlayerId_AnyThread();
	if (leaderId == 0) return true;

	const uint64 targetId = pkt.targetplayerid();

	// 빠른 컷(세션 스레드 안전)
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

			// ✅ Kick + (던전이면) 인스턴스 eject/close/roomClosing 처리까지
			PartyActor::Instance().KickAndHandleInstance(leaderId, targetId);

			// ✅ 성공 판정
			const bool isMemberNow = core.IsMember(partyId, targetId);
			const bool ok = (wasMember && !isMemberNow);

			uint32 version = 0;
			{
				PartyManagerCore::Party afterSnap = core.GetSnapshot(partyId);
				version = (afterSnap.partyId != 0) ? afterSnap.version : 0;
			}

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

			// 남은 파티원 갱신(파티가 아직 존재하면)
			PartyManagerCore::Party afterSnap = core.GetSnapshot(partyId);
			if (afterSnap.partyId != 0)
				BroadcastPartyInfo(partyId);

			// 킥당한 사람: 파티 없음
			auto kicked = GameSessionManager::GSessionManager->FindByPlayerId(targetId);
			if (kicked)
			{
				kicked->Post([](PlayerSessionRef self)
					{
						Protocol::S_PARTY_INFO_NTF off = MakeNoPartyInfoNtf();
						self->Send(ClientPacketHandler::MakeSendBuffer(off));
					});
			}

			// ✅ 던전 내 Kick이면 즉시 강제 복귀
			if (wasInDungeon)
			{
				if (kicked)
				{
					MapChangeUtil::ForceReturnToWorld(kicked);
				}
				else
				{
					// 오프라인이면 "재접속 시 강제 복귀" 플래그
					core.MarkForceReturn(targetId);
				}
			}
		});

	return true;
}

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

			// ✅ disband 전에 멤버/던전정보 확보(없어지기 전에)
			const PartyManagerCore::Party before = core.GetSnapshot(partyId);

			std::vector<uint64> members;
			members.reserve(before.members.size());
			for (uint64 id : before.members) members.push_back(id);

			const bool wasInDungeon = (before.instanceId != 0);

			// ✅ Disband + (던전이면) 인스턴스 Close/RoomClosing 처리
			PartyActor::Instance().DisbandAndHandleInstance(leaderId);

			// ✅ 성공 판정: leader가 파티에서 빠졌으면 성공
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

			// 전원에게 "파티 없음" + (던전이면) 강제 복귀(or 오프라인 플래그)
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
						core.MarkForceReturn(id); // 오프라인이면 재접속 강제복귀
				}
			}
		});

	return true;
}


//helper
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

struct PartyStatusCollector
{
	uint64 partyId = 0;
	uint32 version = 0;

	int32 remaining = 0;                 // "응답 받아야 하는 세션 수"
	std::vector<PartyStatusItem> items;  // 요청자 세션 actor thread에서만 push
};


bool ClientPacketHandler::Handle_C_PARTY_STATUS_REQ(PacketSessionRef& session, Protocol::C_PARTY_STATUS_REQ& pkt)
{
	PlayerSessionRef ps = static_pointer_cast<PlayerSession>(session);
	if (!ps) return true;

	// 네트워크 스레드 빠른 컷
	if (ps->IsMapChanging())
		return true;

	const uint64 myId = ps->GetPlayerId_AnyThread();
	if (myId == 0)
	{
		ps->Post([](PlayerSessionRef self)
			{
				Protocol::S_PARTY_STATUS_NTF ntf;
				ntf.set_partyid(0);
				ntf.set_version(0);
				self->Send(ClientPacketHandler::MakeSendBuffer(ntf));
			});
		return true;
	}

	// 파티 스냅샷은 PartyActor에서만
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

			std::vector<uint64> memberIds;
			memberIds.reserve(snap.members.size());
			for (uint64 id : snap.members)
				memberIds.push_back(id);

			// fan-out은 "요청자 세션 Actor"에서
			ps->Post([partyId, version, memberIds = std::move(memberIds)](PlayerSessionRef requester) mutable
				{
					struct Target
					{
						uint64 playerId = 0;
						PlayerSessionRef session;
					};

					std::vector<Target> targets;
					targets.reserve(memberIds.size());

					for (uint64 id : memberIds)
					{
						auto ms = GameSessionManager::GSessionManager->FindByPlayerId(id);
						if (ms)
							targets.push_back(Target{ id, ms });
					}

					auto collector = MakeShared<PartyStatusCollector>();
					collector->partyId = partyId;
					collector->version = version;
					collector->remaining = static_cast<int32>(targets.size());
					collector->items.reserve(targets.size());

					// 온라인 대상이 0명이면 빈 리스트로 응답
					if (collector->remaining == 0)
					{
						Protocol::S_PARTY_STATUS_NTF ntf;
						ntf.set_partyid(partyId);
						ntf.set_version(version);
						requester->Send(ClientPacketHandler::MakeSendBuffer(ntf));
						return;
					}

					// 각 멤버 세션에게 "내 currentRoom에서 내 상태 뽑아서 보내" 요청
					for (const Target& t : targets)
					{
						const uint64 memberId = t.playerId;
						PlayerSessionRef memberSession = t.session;

						memberSession->Post([requester, collector, memberId](PlayerSessionRef ms)
							{
								// 멤버 세션 Actor에서 room 확인 (PostRoom 쓰면 room 없을 때 remaining이 안 줄어듦)
								if (ms->IsMapChanging())
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
									// room 없음/로비 등 -> 응답 1개 완료 처리만
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

								// ✅ Player 상태는 GameRoom actor thread에서만 읽는다
								gr->PushJob([gr, requester, collector, memberId]()
									{
										PartyStatusItem item;
										bool has = false;

										// ✅ 아래 FindPlayer_ActorOnly는 GameRoom에 추가해줘야 함(아래에 패치 있음)
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

										// 결과 합치기/remaining 감소는 요청자 세션 Actor에서만
										requester->Post([collector, has, item = std::move(item)](PlayerSessionRef req) mutable
											{
												if (has)
													collector->items.push_back(std::move(item));

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
									});
							});
					}
				});
		});

	return true;
}
