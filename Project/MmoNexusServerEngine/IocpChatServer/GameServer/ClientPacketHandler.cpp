#include "pch.h"
#include "ClientPacketHandler.h"
#include "S2SPacketHandler.h" 
#include "PlayerSession.h"
#include "Player.h"
#include "GameSessionManager.h"
#include "Job.h" 
#include "GameRoom.h" 
#include "RedisManager.h"
#include "RoomManager.h"
#include "DataManager.h"
#include <atomic>
#include <chrono>
#include "PartyActor.h"
#include "PartyManagerCore.h"
#include "InstanceActor.h"

namespace
{
	std::atomic<uint64> G_MapChangeTokenSeq{ 1 };

	uint64 MakeMapChangeToken(uint64 playerId, uint64 sessionId)
	{
		uint64 seq = G_MapChangeTokenSeq.fetch_add(1, std::memory_order_relaxed);
		uint64 now = (uint64)std::chrono::duration_cast<std::chrono::milliseconds>(
			std::chrono::steady_clock::now().time_since_epoch()).count();

		// 충돌만 안 나면 됨
		return (playerId << 32) ^ (sessionId << 16) ^ seq ^ now;
	}
}

// ===== [Helper] Dungeon에서 강제 월드 복귀 =====
static void MakeSafeReturn(PlayerRef p, int32& outMapId, int64& outInstId, Protocol::PositionInfo& outPos)
{
	outMapId = p->GetReturnMapId();
	outInstId = p->GetReturnInstanceId();
	outPos = p->GetReturnPos();

	DataManager* dm = DataManager::Instance();
	if (!dm || !dm->IsValidMapId(outMapId))
	{
		outMapId = (dm ? dm->GetDefaultMapId() : 1);
		outInstId = 0;

		const MapConfig* cfg = dm ? dm->GetMapConfig(outMapId) : nullptr;
		outPos.Clear();
		outPos.set_x(cfg ? cfg->spawnX : 50.f);
		outPos.set_y(cfg ? cfg->spawnY : 0.f);
		outPos.set_z(cfg ? cfg->spawnZ : 50.f);
	}
}

static void SendMapChangeBegin(PlayerSessionRef ms, uint64 playerId,
	int32 targetMapId, int64 targetInstanceId, const Protocol::PositionInfo& spawn)
{
	if (!ms) return;
	if (ms->IsMapChanging()) return;
	if (playerId == 0) return;

	const uint64 token = MakeMapChangeToken(playerId, ms->GetSessionId());
	if (!ms->TryBeginMapChange(token, targetMapId, targetInstanceId, spawn))
		return;

	Protocol::S_MAP_CHANGE_BEGIN beginPkt;
	beginPkt.set_token(token);
	beginPkt.set_targetmapid(targetMapId);
	beginPkt.mutable_spawn()->CopyFrom(spawn);
	beginPkt.set_instanceid(targetInstanceId);
	ms->Send(ClientPacketHandler::MakeSendBuffer(beginPkt));
}




static void ForceReturnToWorld(PlayerSessionRef ms)
{
	if (!ms) return;

	ms->Post([](PlayerSessionRef self)
		{
			const uint64 pid = self->GetPlayerId_AnyThread();
			if (pid == 0) return;

			RoomActorRef room = self->GetCurrentRoom_ActorOnly();
			auto gr = (room && room->GetKind() == RoomKind::Game) ? std::dynamic_pointer_cast<GameRoom>(room) : nullptr;
			if (!gr) return;

			gr->PushJob([gr, self, pid]()
				{
					PlayerRef p = gr->FindPlayer_ActorOnly(pid); // <- 네 함수명 맞춰
					if (!p) return;

					int32 rm = 0; int64 ri = 0; Protocol::PositionInfo rp;
					MakeSafeReturn(p, rm, ri, rp);

					self->Post([pid, rm, ri, rp](PlayerSessionRef s) mutable
						{
							SendMapChangeBegin(s, pid, rm, ri, rp); // playerId 버전 SendMapChangeBegin
						});
				});
		});
}







// Global DB Session Reference
extern shared_ptr<PacketSession> G_DBSession;

PacketHandlerFunc ClientPacketHandler::GPacketHandler[UINT16_MAX];



bool ClientPacketHandler::Handle_INVALID(PacketSessionRef& session, BYTE* buffer, int32 len)
{
	return false;
}

// [GAME ENTRY] 게임 입장 요청 (로그인 후 캐릭터 선택 완료 시점)
bool ClientPacketHandler::Handle_C_ENTER_GAME(PacketSessionRef& session, Protocol::C_ENTER_GAME& pkt)
{
	PlayerSessionRef ps = static_pointer_cast<PlayerSession>(session);
	if (!ps) return false;

	// 1) Redis 토큰 검증 (Blocking이지만 일단 유지)
	std::string token = pkt.token();
	std::string value = GRedisManager->Get(token);

	if (value.empty())
	{
		printf("❌ [EnterGame] Invalid Token: %s\n", token.c_str());
		ps->Disconnect(L"Invalid Token");
		return false;
	}

	uint64 playerId = std::stoull(value);

	int32 channelId = pkt.channelid();
	if (channelId <= 0) channelId = 1;

	int32 mapId = pkt.mapid();
	DataManager* dm = DataManager::Instance();

	// ✅ ENTER_GAME은 "월드맵만" 허용. 던전맵 요청이면 기본 월드맵으로 교정
	if (!dm)
	{
		mapId = 1;
	}
	else
	{
		if (!dm->IsValidMapId(mapId) || !dm->IsWorldMapId(mapId))
			mapId = dm->GetDefaultWorldMapId();

		// 방어: cfg가 없으면 기본 월드맵으로 한번 더 교정
		if (dm->GetMapConfig(mapId) == nullptr)
			mapId = dm->GetDefaultWorldMapId();
	}

	const MapConfig* cfg = (dm ? dm->GetMapConfig(mapId) : nullptr);


	// 2) spawn 계산까지 끝났다는 전제: cfg 기반 spawn 만들었지?
	Protocol::PositionInfo spawn;
	spawn.set_x(cfg ? cfg->spawnX : 50.f);
	spawn.set_y(cfg ? cfg->spawnY : 0.f);
	spawn.set_z(cfg ? cfg->spawnZ : 50.f);

	// 3) 이제부터는 Session이 Player를 들지 않는다.
	//    Session Actor에서: playerId 바인딩 + 로비에 “Player 생성/소유” 위임 + DB 요청
	ps->Post([playerId, channelId, mapId, spawn](PlayerSessionRef ps) mutable
		{
			GameSessionManager::GSessionManager->BindPlayerId(ps, playerId);
			ps->SetPlayerId_ActorOnly(playerId);
			if (GLobbyRoom)
			{
				GLobbyRoom->Push([ps, playerId, channelId, mapId, spawn]() mutable
					{
						GLobbyRoom->EnterGame(ps, playerId, channelId, mapId, spawn);
					});
			}

			if (G_DBSession)
			{
				Protocol::S2S_REQ_LOAD_PLAYER_DATA reqStat;
				reqStat.set_playerid(playerId);
				reqStat.set_gamesessionid(ps->GetSessionId());
				G_DBSession->Send(S2SPacketHandler::MakeSendBuffer(reqStat));

				Protocol::S2S_REQ_ITEMS_LOAD reqItem;
				reqItem.set_playerid(playerId);
				reqItem.set_gamesessionid(ps->GetSessionId());
				G_DBSession->Send(S2SPacketHandler::MakeSendBuffer(reqItem));
			}
		});


	return true;
}



// [MOVE] 이동 요청
bool ClientPacketHandler::Handle_C_MOVE(PacketSessionRef& session, Protocol::C_MOVE& pkt)
{
	PlayerSessionRef ps = static_pointer_cast<PlayerSession>(session);
	if (!ps) return false;

	if (ps->IsMapChanging())
		return true;

	const uint64 playerId = ps->GetPlayerId_AnyThread();
	if (playerId == 0)
		return true;

	ps->PostRoom([playerId, pkt](PlayerSessionRef self, RoomActorRef room) mutable
		{
			if (!room) return;
			if (self->IsMapChanging()) return;
			if (room->GetKind() != RoomKind::Game) return;

			auto gr = std::static_pointer_cast<GameRoom>(room);
			gr->Push([gr, self, playerId, pkt]() mutable
				{
					gr->HandleMoveById(self, playerId, pkt);
				});
		});

	return true;
}

bool ClientPacketHandler::Handle_C_EQUIP_ITEM(PacketSessionRef& session, Protocol::C_EQUIP_ITEM& pkt)
{
	PlayerSessionRef ps = static_pointer_cast<PlayerSession>(session);
	if (!ps) return false;

	if (ps->IsMapChanging())
		return true;

	const uint64 playerId = ps->GetPlayerId_AnyThread();
	if (playerId == 0)
		return true;

	ps->PostRoom([playerId, pkt](PlayerSessionRef self, RoomActorRef room) mutable
		{
			if (!room) return;
			if (self->IsMapChanging()) return;
			if (room->GetKind() != RoomKind::Game) return;

			auto gr = std::static_pointer_cast<GameRoom>(room);
			gr->Push([gr, self, playerId, pkt]() mutable
				{
					gr->HandleEquipItemById(self, playerId, pkt);
				});
		});

	return true;
}


bool ClientPacketHandler::Handle_C_USE_ITEM(PacketSessionRef& session, Protocol::C_USE_ITEM& pkt)
{
	PlayerSessionRef ps = static_pointer_cast<PlayerSession>(session);
	if (!ps) return false;

	if (ps->IsMapChanging())
		return true;

	const uint64 playerId = ps->GetPlayerId_AnyThread();
	if (playerId == 0)
		return true;

	ps->PostRoom([playerId, pkt](PlayerSessionRef self, RoomActorRef room) mutable
		{
			if (!room) return;
			if (self->IsMapChanging()) return;
			if (room->GetKind() != RoomKind::Game) return;

			auto gr = std::static_pointer_cast<GameRoom>(room);
			gr->Push([gr, self, playerId, pkt]() mutable
				{
					gr->HandleUseItemById(self, playerId, pkt);
				});
		});

	return true;
}

// [LOGIN] 인증 요청

bool ClientPacketHandler::Handle_C_SKILL(PacketSessionRef& session, Protocol::C_SKILL& pkt)
{
	PlayerSessionRef ps = static_pointer_cast<PlayerSession>(session);
	if (!ps) return true;

	if (ps->IsMapChanging())
		return true;

	const int32 skillId = pkt.skillid();
	const uint64 playerId = ps->GetPlayerId_AnyThread();
	if (playerId == 0)
		return true;

	ps->PostRoom([playerId, skillId](PlayerSessionRef self, RoomActorRef room) mutable
		{
			if (!room) return;
			if (self->IsMapChanging()) return;
			if (room->GetKind() != RoomKind::Game) return;

			auto gr = std::static_pointer_cast<GameRoom>(room);
			gr->Push([gr, self, playerId, skillId]()
				{
					gr->HandleSkillById(self, playerId, skillId);
				});
		});

	return true;
}

// [CHAT] 채팅 요청
bool ClientPacketHandler::Handle_C_CHAT_REQ(PacketSessionRef& session, Protocol::C_CHAT_REQ& pkt)
{
	PlayerSessionRef ps = static_pointer_cast<PlayerSession>(session);
	if (!ps) return true;

	if (ps->IsMapChanging())
		return true;

	const std::string msg = pkt.message();
	const uint64 playerId = ps->GetPlayerId_AnyThread();
	if (playerId == 0)
		return true;

	ps->PostRoom([playerId, msg](PlayerSessionRef self, RoomActorRef room) mutable
		{
			if (!room) return;
			if (self->IsMapChanging()) return;
			if (room->GetKind() != RoomKind::Game) return;

			auto gr = std::static_pointer_cast<GameRoom>(room);
			gr->Push([gr, self, playerId, msg]() mutable
				{
					gr->HandleChatById(self, playerId, msg);
				});
		});

	return true;
}


//맵이동
bool ClientPacketHandler::Handle_C_MAP_CHANGE_REQ(PacketSessionRef& session, Protocol::C_MAP_CHANGE_REQ& pkt)
{
	PlayerSessionRef ps = static_pointer_cast<PlayerSession>(session);
	if (!ps) return false;

	if (ps->IsMapChanging())
		return true;

	const uint64 playerId = ps->GetPlayerId_AnyThread();
	if (playerId == 0)
		return true;

	const int32 targetMapId = pkt.targetmapid();
	const int64 targetInstanceId = 0; // 월드 이동은 0

	DataManager* dm = DataManager::Instance();

	if (!dm || !dm->IsValidMapId(targetMapId) || !dm->IsWorldMapId(targetMapId))
	{
		std::cout << "❌ [MapChange] rejected. targetMapId=" << targetMapId << std::endl;
		return true;
	}

	const MapConfig* cfg = dm->GetMapConfig(targetMapId);
	if (!cfg)
		return true;

	Protocol::PositionInfo spawn;
	spawn.set_x(cfg->spawnX);
	spawn.set_y(cfg->spawnY);
	spawn.set_z(cfg->spawnZ);

	ps->Post([playerId, targetMapId, targetInstanceId, spawn](PlayerSessionRef self) mutable
		{
			if (self->IsMapChanging())
				return;

			const uint64 token = MakeMapChangeToken(playerId, self->GetSessionId());
			if (!self->TryBeginMapChange(token, targetMapId, targetInstanceId, spawn))
				return;

			Protocol::S_MAP_CHANGE_BEGIN beginPkt;
			beginPkt.set_token(token);
			beginPkt.set_targetmapid(targetMapId);
			beginPkt.mutable_spawn()->CopyFrom(spawn);
			beginPkt.set_instanceid(targetInstanceId);

			self->Send(MakeSendBuffer(beginPkt));
		});

	return true;
}


bool ClientPacketHandler::Handle_C_MAP_CHANGE_ACK(PacketSessionRef& session, Protocol::C_MAP_CHANGE_ACK& pkt)
{
	PlayerSessionRef ps = static_pointer_cast<PlayerSession>(session);
	if (!ps) return false;

	const uint64 token = pkt.token();

	ps->Post([token](PlayerSessionRef self)
		{
			int32 targetMapId = 0;
			int64 targetInstanceId = 0;
			Protocol::PositionInfo spawn;

			if (!self->TryConsumeMapChangeAck(token, targetMapId, targetInstanceId, spawn))
				return;

			const uint64 playerId = self->GetPlayerId_AnyThread();
			if (playerId == 0)
			{
				self->CancelMapChange();
				return;
			}

			auto room = self->GetCurrentRoom_ActorOnly();
			if (!room || room->GetKind() != RoomKind::Game)
			{
				self->CancelMapChange();
				return;
			}

			auto gr = std::static_pointer_cast<GameRoom>(room);
			gr->Push([gr, self, playerId, targetMapId, targetInstanceId, spawn]() mutable
				{
					gr->TransferMapChangeById(self, playerId, targetMapId, targetInstanceId, spawn);
				});
		});

	return true;
}


bool ClientPacketHandler::Handle_C_HEART_BEAT_REQ(PacketSessionRef& session, Protocol::C_HEART_BEAT_REQ& pkt)
{
	return true;
}





bool ClientPacketHandler::Handle_C_LOGIN(PacketSessionRef& session, Protocol::C_LOGIN& pkt)
{
	return false;
}

//=======================================파티===========================================


// 파티 스냅샷 -> S_PARTY_INFO_NTF
// 파티 스냅샷 -> S_PARTY_INFO_NTF
// Party snapshot -> S_PARTY_INFO_NTF
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
				ForceReturnToWorld(ps);
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
					ForceReturnToWorld(kicked);
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
						ForceReturnToWorld(ms);
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

bool ClientPacketHandler::Handle_C_DUNGEON_ENTER_REQ(PacketSessionRef& session, Protocol::C_DUNGEON_ENTER_REQ& pkt)
{
	PlayerSessionRef ps = static_pointer_cast<PlayerSession>(session);
	if (!ps) return false;

	const int32 dungeonMapId = pkt.dungeonmapid();

	DataManager* dm = DataManager::Instance();
	if (!dm || !dm->IsValidMapId(dungeonMapId) || !dm->IsDungeonMapId(dungeonMapId))
	{
		ps->Post([dungeonMapId](PlayerSessionRef self)
			{
				Protocol::S_DUNGEON_ENTER_RES res;
				res.set_success(false);
				res.set_dungeonmapid(dungeonMapId);
				res.set_instanceid(0);
				res.set_reason(Protocol::DUNGEON_ENTER_FAIL_INVALID_MAP);
				self->Send(MakeSendBuffer(res));
			});
		return true;
	}

	const MapConfig* cfg = dm->GetMapConfig(dungeonMapId);
	if (!cfg) return true;

	Protocol::PositionInfo spawn;
	spawn.set_x(cfg->spawnX);
	spawn.set_y(cfg->spawnY);
	spawn.set_z(cfg->spawnZ);

	ps->Post([dungeonMapId, spawn](PlayerSessionRef self) mutable
		{
			const uint64 requesterId = self->GetPlayerId_AnyThread();
			if (requesterId == 0)
			{
				Protocol::S_DUNGEON_ENTER_RES res;
				res.set_success(false);
				res.set_dungeonmapid(dungeonMapId);
				res.set_instanceid(0);
				res.set_reason(Protocol::DUNGEON_ENTER_FAIL_INTERNAL);
				self->Send(MakeSendBuffer(res));
				return;
			}

			RoomActorRef room = self->GetCurrentRoom_ActorOnly();
			auto gr = (room && room->GetKind() == RoomKind::Game) ? std::dynamic_pointer_cast<GameRoom>(room) : nullptr;
			if (!gr)
			{
				Protocol::S_DUNGEON_ENTER_RES res;
				res.set_success(false);
				res.set_dungeonmapid(dungeonMapId);
				res.set_instanceid(0);
				res.set_reason(Protocol::DUNGEON_ENTER_FAIL_INTERNAL);
				self->Send(MakeSendBuffer(res));
				return;
			}

			// ✅ channelId는 "player 값"으로만 읽는다 (Room actor에서)
			gr->PushJob([gr, requesterId, dungeonMapId, spawn]()
				{
					int32 channelId = 0;

					// 네 GameRoom에 이 함수가 이미 있다고 했던 경계 규칙 기반
					// (없으면: FindPlayer_ActorOnly를 네가 쓰는 이름으로 바꿔)
					PlayerRef p = gr->FindPlayer_ActorOnly(requesterId);
					if (p)
						channelId = p->GetChannelId();

					PartyActor::Instance().Push([requesterId, channelId, dungeonMapId, spawn]()
						{
							auto& core = PartyActor::Instance().Core();
							const uint64 partyId = core.GetPartyIdByPlayerId(requesterId);

							if (partyId == 0)
							{
								if (auto s = GameSessionManager::GSessionManager->FindByPlayerId(requesterId))
								{
									s->Post([dungeonMapId](PlayerSessionRef self2)
										{
											Protocol::S_DUNGEON_ENTER_RES res;
											res.set_success(false);
											res.set_dungeonmapid(dungeonMapId);
											res.set_instanceid(0);
											res.set_reason(Protocol::DUNGEON_ENTER_FAIL_NOT_IN_PARTY);
											self2->Send(MakeSendBuffer(res));
										});
								}
								return;
							}

							// 이미 던전 메타 있으면 막기
							{
								int64 curInst = 0; PartyManagerCore::DungeonState st; bool tr = false;
								if (core.GetDungeonInfo(partyId, curInst, st, tr))
								{
									if (curInst != 0 || tr)
									{
										if (auto s = GameSessionManager::GSessionManager->FindByPlayerId(requesterId))
										{
											s->Post([dungeonMapId](PlayerSessionRef self2)
												{
													Protocol::S_DUNGEON_ENTER_RES res;
													res.set_success(false);
													res.set_dungeonmapid(dungeonMapId);
													res.set_instanceid(0);
													res.set_reason(Protocol::DUNGEON_ENTER_FAIL_INTERNAL);
													self2->Send(MakeSendBuffer(res));
												});
										}
										return;
									}
								}
							}

							if (!core.TryBeginDungeonTransition(partyId, PartyManagerCore::DungeonState::ENTERING))
							{
								if (auto s = GameSessionManager::GSessionManager->FindByPlayerId(requesterId))
								{
									s->Post([dungeonMapId](PlayerSessionRef self2)
										{
											Protocol::S_DUNGEON_ENTER_RES res;
											res.set_success(false);
											res.set_dungeonmapid(dungeonMapId);
											res.set_instanceid(0);
											res.set_reason(Protocol::DUNGEON_ENTER_FAIL_INTERNAL);
											self2->Send(MakeSendBuffer(res));
										});
								}
								return;
							}

							std::vector<uint64> members;
							core.GetMembers(partyId, members);

							InstanceActor::Instance().Push([partyId, members, channelId, dungeonMapId, spawn, requesterId]()
								{
									InstanceManagerCore::InstanceInfo inst;
									if (!InstanceActor::Instance().Core().CreateOrGetForParty(partyId, channelId, dungeonMapId, members, inst))
									{
										PartyActor::Instance().Push([partyId]()
											{
												auto& pc = PartyActor::Instance().Core();
												pc.EndDungeonTransition(partyId, PartyManagerCore::DungeonState::NONE);
												pc.ClearPartyInstance(partyId);
											});

										if (auto s = GameSessionManager::GSessionManager->FindByPlayerId(requesterId))
										{
											s->Post([dungeonMapId](PlayerSessionRef self2)
												{
													Protocol::S_DUNGEON_ENTER_RES res;
													res.set_success(false);
													res.set_dungeonmapid(dungeonMapId);
													res.set_instanceid(0);
													res.set_reason(Protocol::DUNGEON_ENTER_FAIL_INTERNAL);
													self2->Send(MakeSendBuffer(res));
												});
										}
										return;
									}

									const int64 instanceId = inst.instanceId;

									PartyActor::Instance().Push([partyId, instanceId]()
										{
											auto& pc = PartyActor::Instance().Core();
											pc.SetPartyInstance(partyId, instanceId, PartyManagerCore::DungeonState::IN_DUNGEON);
											pc.EndDungeonTransition(partyId, PartyManagerCore::DungeonState::IN_DUNGEON);
										});

									// requester에게 결과
									if (auto req = GameSessionManager::GSessionManager->FindByPlayerId(requesterId))
									{
										req->Post([dungeonMapId, instanceId](PlayerSessionRef self2)
											{
												Protocol::S_DUNGEON_ENTER_RES res;
												res.set_success(true);
												res.set_dungeonmapid(dungeonMapId);
												res.set_instanceid(instanceId);
												res.set_reason(Protocol::DUNGEON_ENTER_OK);
												self2->Send(MakeSendBuffer(res));
											});
									}

									// 파티원 전원 MapChangeBegin
									for (uint64 pid : members)
									{
										auto ms = GameSessionManager::GSessionManager->FindByPlayerId(pid);
										if (!ms) continue;

										ms->Post([pid, dungeonMapId, instanceId, spawn](PlayerSessionRef self2) mutable
											{
												if (self2->IsMapChanging())
													return;

												RoomActorRef room = self2->GetCurrentRoom_ActorOnly();
												auto gr2 = (room && room->GetKind() == RoomKind::Game) ? std::dynamic_pointer_cast<GameRoom>(room) : nullptr;

												if (!gr2)
												{
													SendMapChangeBegin(self2, pid, dungeonMapId, instanceId, spawn);
													return;
												}

												// return 저장은 Room actor에서 (player 값 기반)
												gr2->PushJob([gr2, self2, pid, dungeonMapId, instanceId, spawn]() mutable
													{
														gr2->SaveReturnLocation_ActorOnly(pid);

														self2->Post([pid, dungeonMapId, instanceId, spawn](PlayerSessionRef s) mutable
															{
																SendMapChangeBegin(s, pid, dungeonMapId, instanceId, spawn);
															});
													});
											});
									}
								});
						});
				});
		});

	return true;
}

bool ClientPacketHandler::Handle_C_DUNGEON_EXIT_REQ(PacketSessionRef& session, Protocol::C_DUNGEON_EXIT_REQ& pkt)
{
	PlayerSessionRef ps = static_pointer_cast<PlayerSession>(session);
	if (!ps) return false;

	if (ps->IsMapChanging())
		return true;

	// SessionActor에서 시작 (player 접근 금지)
	ps->Post([](PlayerSessionRef self)
		{
			const uint64 requesterId = self->GetPlayerId_AnyThread();
			if (requesterId == 0)
			{
				Protocol::S_DUNGEON_EXIT_RES res;
				res.set_success(false);
				res.set_returnmapid(1);
				res.set_returninstanceid(0);
				res.set_reason(Protocol::DUNGEON_EXIT_FAIL_INTERNAL);
				self->Send(MakeSendBuffer(res));
				return;
			}

			RoomActorRef room = self->GetCurrentRoom_ActorOnly();
			auto gr = (room && room->GetKind() == RoomKind::Game) ? std::dynamic_pointer_cast<GameRoom>(room) : nullptr;
			if (!gr)
			{
				Protocol::S_DUNGEON_EXIT_RES res;
				res.set_success(false);
				res.set_returnmapid(1);
				res.set_returninstanceid(0);
				res.set_reason(Protocol::DUNGEON_EXIT_FAIL_INTERNAL);
				self->Send(MakeSendBuffer(res));
				return;
			}

			// 던전 여부 판정은 RoomActor에서
			gr->PushJob([gr, requesterId]()
				{
					PlayerRef p = gr->FindPlayer_ActorOnly(requesterId); // <- 네 함수명 맞춰
					if (!p)
					{
						if (auto s = GameSessionManager::GSessionManager->FindByPlayerId(requesterId))
						{
							s->Post([](PlayerSessionRef self2)
								{
									Protocol::S_DUNGEON_EXIT_RES res;
									res.set_success(false);
									res.set_returnmapid(1);
									res.set_returninstanceid(0);
									res.set_reason(Protocol::DUNGEON_EXIT_FAIL_INTERNAL);
									self2->Send(MakeSendBuffer(res));
								});
						}
						return;
					}

					// 0이면 던전 아님
					if (p->GetInstanceId() == 0)
					{
						const int32 curMap = p->GetMapId();
						const int64 curInst = p->GetInstanceId();

						if (auto s = GameSessionManager::GSessionManager->FindByPlayerId(requesterId))
						{
							s->Post([curMap, curInst](PlayerSessionRef self2)
								{
									Protocol::S_DUNGEON_EXIT_RES res;
									res.set_success(false);
									res.set_returnmapid(curMap);
									res.set_returninstanceid(curInst);
									res.set_reason(Protocol::DUNGEON_EXIT_FAIL_NOT_IN_DUNGEON);
									self2->Send(MakeSendBuffer(res));
								});
						}
						return;
					}

					// ===== 여기부터는 PartyActor/InstanceActor 흐름 =====
					PartyActor::Instance().Push([requesterId]()
						{
							auto& core = PartyActor::Instance().Core();
							const uint64 partyId = core.GetPartyIdByPlayerId(requesterId);

							if (partyId == 0)
							{
								if (auto s = GameSessionManager::GSessionManager->FindByPlayerId(requesterId))
								{
									s->Post([](PlayerSessionRef self2)
										{
											Protocol::S_DUNGEON_EXIT_RES res;
											res.set_success(false);
											res.set_returnmapid(1);
											res.set_returninstanceid(0);
											res.set_reason(Protocol::DUNGEON_EXIT_FAIL_NOT_IN_PARTY);
											self2->Send(MakeSendBuffer(res));
										});
								}
								return;
							}

							// EXITING transition
							if (!core.TryBeginDungeonTransition(partyId, PartyManagerCore::DungeonState::EXITING))
							{
								if (auto s = GameSessionManager::GSessionManager->FindByPlayerId(requesterId))
								{
									s->Post([](PlayerSessionRef self2)
										{
											Protocol::S_DUNGEON_EXIT_RES res;
											res.set_success(false);
											res.set_returnmapid(1);
											res.set_returninstanceid(0);
											res.set_reason(Protocol::DUNGEON_EXIT_FAIL_INTERNAL);
											self2->Send(MakeSendBuffer(res));
										});
								}
								return;
							}

							std::vector<uint64> members;
							core.GetMembers(partyId, members);

							InstanceActor::Instance().Push([partyId, members, requesterId]()
								{
									InstanceManagerCore::InstanceInfo closed;
									const bool closedOk = InstanceActor::Instance().Core().CloseForParty(partyId, closed);

									if (!closedOk)
									{
										// 롤백: 다시 IN_DUNGEON
										PartyActor::Instance().Push([partyId]()
											{
												auto& pc = PartyActor::Instance().Core();
												pc.EndDungeonTransition(partyId, PartyManagerCore::DungeonState::IN_DUNGEON);
											});

										if (auto req = GameSessionManager::GSessionManager->FindByPlayerId(requesterId))
										{
											req->Post([](PlayerSessionRef self2)
												{
													Protocol::S_DUNGEON_EXIT_RES res;
													res.set_success(false);
													res.set_returnmapid(1);
													res.set_returninstanceid(0);
													res.set_reason(Protocol::DUNGEON_EXIT_FAIL_INTERNAL);
													self2->Send(MakeSendBuffer(res));
												});
										}
										return;
									}

									// room closing 마킹(purge 유도)
									if (closed.instanceId != 0 && GRoomManager)
									{
										auto room = GRoomManager->FindRoom(closed.channelId, closed.mapId, closed.instanceId);
										if (room) room->MarkClosing(true);
									}

									// party 메타 정리 + transition 종료(NONE)
									PartyActor::Instance().Push([partyId]()
										{
											auto& pc = PartyActor::Instance().Core();
											pc.ClearPartyInstance(partyId);
											pc.EndDungeonTransition(partyId, PartyManagerCore::DungeonState::NONE);
										});

									// requester에게 exit res(성공) - return은 RoomActor에서 계산
									if (auto req = GameSessionManager::GSessionManager->FindByPlayerId(requesterId))
									{
										req->Post([requesterId](PlayerSessionRef self2)
											{
												RoomActorRef room = self2->GetCurrentRoom_ActorOnly();
												auto gr2 = (room && room->GetKind() == RoomKind::Game) ? std::dynamic_pointer_cast<GameRoom>(room) : nullptr;

												if (!gr2)
												{
													Protocol::S_DUNGEON_EXIT_RES res;
													res.set_success(true);
													res.set_returnmapid(1);
													res.set_returninstanceid(0);
													res.set_reason(Protocol::DUNGEON_EXIT_OK);
													self2->Send(MakeSendBuffer(res));
													return;
												}

												gr2->PushJob([gr2, self2, requesterId]()
													{
														PlayerRef p = gr2->FindPlayer_ActorOnly(requesterId);
														int32 rm = 1; int64 ri = 0; Protocol::PositionInfo rp;

														if (p)
															MakeSafeReturn(p, rm, ri, rp);

														self2->Post([rm, ri](PlayerSessionRef s) mutable
															{
																Protocol::S_DUNGEON_EXIT_RES res;
																res.set_success(true);
																res.set_returnmapid(rm);
																res.set_returninstanceid(ri);
																res.set_reason(Protocol::DUNGEON_EXIT_OK);
																s->Send(MakeSendBuffer(res));
															});
													});
											});
									}

									// 파티원 전원 월드 복귀 MapChangeBegin (온라인만)
									for (uint64 pid : members)
									{
										auto ms = GameSessionManager::GSessionManager->FindByPlayerId(pid);
										if (!ms) continue;

										ms->Post([pid](PlayerSessionRef self2) mutable
											{
												if (self2->IsMapChanging())
													return;

												RoomActorRef room = self2->GetCurrentRoom_ActorOnly();
												auto gr2 = (room && room->GetKind() == RoomKind::Game) ? std::dynamic_pointer_cast<GameRoom>(room) : nullptr;
												if (!gr2) return;

												gr2->PushJob([gr2, self2, pid]()
													{
														PlayerRef p = gr2->FindPlayer_ActorOnly(pid);
														if (!p) return;

														int32 rm = 1; int64 ri = 0; Protocol::PositionInfo rp;
														MakeSafeReturn(p, rm, ri, rp);

														self2->Post([pid, rm, ri, rp](PlayerSessionRef s) mutable
															{
																SendMapChangeBegin(s, pid, rm, ri, rp);
															});
													});
											});
									}
								});
						});
				});
		});

	return true;
}
