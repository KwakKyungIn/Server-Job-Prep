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
	if (!dm || !dm->IsValidMapId(mapId))
		mapId = (dm ? dm->GetDefaultMapId() : 1);

	const MapConfig* cfg = (dm ? dm->GetMapConfig(mapId) : nullptr);

	// 2) Player 생성/초기화는 여기서 해도 OK (아직 공유 안 됨)
	PlayerRef player = ObjectPool<Player>::MakeShared();
	{
		Protocol::PlayerInfo tempInfo;
		tempInfo.set_playerid(playerId);
		tempInfo.set_name("Player_" + std::to_string(playerId));

		auto pos = tempInfo.mutable_posinfo();
		pos->set_x(cfg ? cfg->spawnX : 50.f);
		pos->set_y(cfg ? cfg->spawnY : 0.f);
		pos->set_z(cfg ? cfg->spawnZ : 50.f);

		player->Init(tempInfo);
	}
	player->SetChannelId(channelId);
	player->SetMapId(mapId);

	// 3) 세션 상태 변경/DB 요청은 세션 Actor에서
	ps->Post([player, playerId](PlayerSessionRef ps)
		{
			ps->SetPlayer(player);
			player->SetSession(ps);

			GameSessionManager::GSessionManager->BindPlayerId(ps, playerId);

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

	// 빠른 컷(atomic read만) - OK
	if (ps->IsMapChanging())
		return true;

	// 이제부터는 세션 Actor에게 위임
	ps->PostPlayer([pkt](PlayerSessionRef ps, PlayerRef player)
		{
			if (ps->IsMapChanging())
				return;

			auto room = player->GetRoom();
			if (!room) return;

			room->PushJob(&GameRoom::HandleMove, ps,player, pkt);
		});

	return true;
}

bool ClientPacketHandler::Handle_C_EQUIP_ITEM(PacketSessionRef& session, Protocol::C_EQUIP_ITEM& pkt)
{
	PlayerSessionRef ps = static_pointer_cast<PlayerSession>(session);
	if (!ps) return false;

	if (ps->IsMapChanging())
		return true;

	ps->PostPlayer([pkt](PlayerSessionRef ps, PlayerRef player)
		{
			if (ps->IsMapChanging())
				return;

			Protocol::ItemInfo* targetItem = nullptr;
			auto& items = player->GetItems();

			for (auto& item : items)
			{
				if (item.itemuid() == pkt.itemuid())
				{
					targetItem = &item;
					break;
				}
			}

			if (!targetItem)
				return;

			targetItem->set_isequipped(pkt.equip());
			player->RefreshStats();

			// 장착 결과
			{
				Protocol::S_EQUIP_ITEM res;
				res.set_itemuid(pkt.itemuid());
				res.set_equipped(pkt.equip());
				res.set_slotindex(pkt.slotindex());
				ps->Send(MakeSendBuffer(res));
			}

			// 스탯 갱신
			{
				Protocol::S_CHANGE_STAT st;
				st.mutable_statinfo()->CopyFrom(*player->GetStatInfo());
				ps->Send(MakeSendBuffer(st));
			}
		});

	return true;
}


bool ClientPacketHandler::Handle_C_USE_ITEM(PacketSessionRef& session, Protocol::C_USE_ITEM& pkt)
{
	PlayerSessionRef ps = static_pointer_cast<PlayerSession>(session);
	if (!ps) return false;

	if (ps->IsMapChanging())
		return true;

	ps->PostPlayer([pkt](PlayerSessionRef ps, PlayerRef player)
		{
			if (ps->IsMapChanging())
				return;

			auto room = player->GetRoom();
			if (!room) return;

			room->PushJob(&GameRoom::HandleUseItem, ps,player, pkt);
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

	ps->PostPlayer([pkt](PlayerSessionRef ps, PlayerRef player)
		{
			auto room = player->GetRoom();
			if (!room) return;

			// 룸에서 처리(판정/쿨타임/HP체크 등도 룸으로 넘기는 게 깔끔)
			room->PushJob([player, pkt]()
				{
					player->UseSkill(pkt.skillid());
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

	ps->PostPlayer([msg](PlayerSessionRef ps, PlayerRef player)
		{
			auto room = player->GetRoom();
			if (!room) return;

			Protocol::S_CHAT_NTF ntf;
			ntf.set_playerid(player->GetPlayerId());
			ntf.set_name(player->GetName());
			ntf.set_message(msg);

			room->PushJob([room, ntf]()
				{
					room->BroadcastChat(ntf);
				});
		});

	return true;
}


//맵이동
bool ClientPacketHandler::Handle_C_MAP_CHANGE_REQ(PacketSessionRef& session, Protocol::C_MAP_CHANGE_REQ& pkt)
{
	PlayerSessionRef ps = static_pointer_cast<PlayerSession>(session);
	if (!ps) return false;

	// 빠른 컷 OK
	if (ps->IsMapChanging())
		return true;

	const int32 targetMapId = pkt.targetmapid();

	// DataManager 읽기/검증은 여기서 해도 되지만,
	// player/map 비교는 세션 Actor에서 하자.
	DataManager* dm = DataManager::Instance();
	if (!dm || !dm->IsValidMapId(targetMapId))
		return false;

	const MapConfig* cfg = dm->GetMapConfig(targetMapId);
	if (!cfg)
		return false;

	Protocol::PositionInfo spawn;
	spawn.set_x(cfg->spawnX);
	spawn.set_y(cfg->spawnY);
	spawn.set_z(cfg->spawnZ);

	ps->PostPlayer([ps, targetMapId, spawn](PlayerSessionRef ps, PlayerRef player)
		{
			if (ps->IsMapChanging())
				return;

			// 같은 맵이면 무시
			if (player->GetMapId() == targetMapId)
				return;

			const uint64 token = MakeMapChangeToken(player->GetPlayerId(), ps->GetSessionId());
			if (ps->TryBeginMapChange(token, targetMapId, spawn) == false)
				return;

			Protocol::S_MAP_CHANGE_BEGIN beginPkt;
			beginPkt.set_token(token);
			beginPkt.set_targetmapid(targetMapId);
			beginPkt.mutable_spawn()->CopyFrom(spawn);

			ps->Send(MakeSendBuffer(beginPkt));
		});

	return true;
}


bool ClientPacketHandler::Handle_C_MAP_CHANGE_ACK(PacketSessionRef& session, Protocol::C_MAP_CHANGE_ACK& pkt)
{
	PlayerSessionRef ps = static_pointer_cast<PlayerSession>(session);
	if (!ps) return false;

	const uint64 token = pkt.token();

	ps->PostPlayer([ps, token](PlayerSessionRef ps, PlayerRef player)
		{
			int32 targetMapId = 0;
			Protocol::PositionInfo spawn;

			if (ps->TryConsumeMapChangeAck(token, targetMapId, spawn) == false)
				return;

			const int32 channelId = player->GetChannelId();
			auto newRoom = GRoomManager->GetOrCreateRoom(channelId, targetMapId);
			if (!newRoom)
			{
				ps->CancelMapChange();
				return;
			}

			auto oldRoom = player->GetRoom();

			if (!oldRoom)
			{
				// 룸 없으면 그냥 새 룸으로 진입 요청
				// (mapId/pos 변경은 너 구조상 oldRoom에서 하도록 했는데,
				//  oldRoom이 없으니 여기서는 “최소 변경”만 하고 newRoom으로 보낸다)
				player->SetMapId(targetMapId);
				player->GetPosInfo()->CopyFrom(spawn);

				newRoom->PushJob(&GameRoom::EnterMapChange, ps, player);
				return;
			}

			// Leave는 oldRoom, Enter는 newRoom (이건 너가 이미 잘 짰음)
			oldRoom->PushJob([ps, player, oldRoom, newRoom, targetMapId, spawn]()
				{
					// ✅ Leave 시그니처가 (session, player)면 이렇게
					oldRoom->Leave(ps, player);

					// ✅ room thread에서 player 메타/좌표 갱신 (지금 구조 유지)
					player->SetMapId(targetMapId);
					player->GetPosInfo()->CopyFrom(spawn);

					// ✅ EnterMapChange도 (session, player)로 바꿨으면 같이 넘겨야 함
					newRoom->PushJob(&GameRoom::EnterMapChange, ps, player);
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

	ps->PostPlayer([msg](PlayerSessionRef ps, PlayerRef player)
		{
			const uint64 senderId = player->GetPlayerId();
			const std::string senderName = player->GetName();

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

						// 전송은 "대상 세션 Actor"에서
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
		});

	return true;
}


bool ClientPacketHandler::Handle_C_PARTY_CREATE_REQ(PacketSessionRef& session, Protocol::C_PARTY_CREATE_REQ& pkt)
{
	PlayerSessionRef ps = static_pointer_cast<PlayerSession>(session);
	if (!ps) return true;

	if (ps->IsMapChanging())
		return true;

	ps->PostPlayer([](PlayerSessionRef ps, PlayerRef player)
		{
			const uint64 leaderId = player->GetPlayerId();

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
						BroadcastPartyInfo(partyId); // Info만
				});
		});

	return true;
}

bool ClientPacketHandler::Handle_C_PARTY_INVITE_REQ(PacketSessionRef& session, Protocol::C_PARTY_INVITE_REQ& pkt)
{
	PlayerSessionRef ps = static_pointer_cast<PlayerSession>(session);
	if (!ps) return true;

	if (ps->IsMapChanging())
		return true;

	const uint64 targetId = pkt.targetplayerid();

	ps->PostPlayer([targetId](PlayerSessionRef ps, PlayerRef player)
		{
			const uint64 inviterId = player->GetPlayerId();
			const std::string inviterName = player->GetName();

			if (targetId == 0 || targetId == inviterId)
			{
				Protocol::S_PARTY_RESULT res;
				res.set_op(Protocol::PARTY_OP_INVITE);
				res.set_success(false);
				res.set_reason(Protocol::PARTY_REASON_SELF_TARGET);
				ps->Send(ClientPacketHandler::MakeSendBuffer(res));
				return;
			}

			auto targetSession = GameSessionManager::GSessionManager->FindByPlayerId(targetId);
			if (!targetSession)
			{
				Protocol::S_PARTY_RESULT res;
				res.set_op(Protocol::PARTY_OP_INVITE);
				res.set_success(false);
				res.set_reason(Protocol::PARTY_REASON_NO_TARGET);
				ps->Send(ClientPacketHandler::MakeSendBuffer(res));
				return;
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
					const uint32 version = core.GetSnapshot(partyId).version;

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
		});

	return true;
}

bool ClientPacketHandler::Handle_C_PARTY_INVITE_ACCEPT_REQ(PacketSessionRef& session, Protocol::C_PARTY_INVITE_ACCEPT_REQ& pkt)
{
	PlayerSessionRef ps = static_pointer_cast<PlayerSession>(session);
	if (!ps) return true;

	const uint64 partyId = pkt.partyid();
	const bool accept = pkt.accept();

	ps->PostPlayer([partyId, accept](PlayerSessionRef ps, PlayerRef player)
		{
			const uint64 targetId = player->GetPlayerId();

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

					if (!accept)
					{
						const uint64 curPartyId = core.GetPartyIdByPlayerId(targetId); // 보통 0
						SendPartyInfoTo(ps, curPartyId);
					}
				});
		});

	return true;
}

bool ClientPacketHandler::Handle_C_PARTY_LEAVE_REQ(PacketSessionRef& session, Protocol::C_PARTY_LEAVE_REQ& pkt)
{
	PlayerSessionRef ps = static_pointer_cast<PlayerSession>(session);
	if (!ps) return true;

	if (ps->IsMapChanging())
		return true;

	ps->PostPlayer([](PlayerSessionRef ps, PlayerRef player)
		{
			const uint64 pid = player->GetPlayerId();

			PartyActor::Instance().Push([ps, pid]()
				{
					auto& core = PartyActor::Instance().Core();

					const uint64 beforePartyId = core.GetPartyIdByPlayerId(pid);

					PartyManagerCore::Party after;
					bool disbanded = false;
					const bool ok = core.Leave(pid, after, disbanded);

					const uint32 version = after.partyId ? after.version : 0;

					// 결과 응답(요청자) - 세션 Actor에서
					ps->Post([ok, beforePartyId, version](PlayerSessionRef self)
						{
							Protocol::S_PARTY_RESULT res;
							res.set_op(Protocol::PARTY_OP_LEAVE);
							res.set_success(ok);
							res.set_reason(ok ? Protocol::PARTY_REASON_OK : Protocol::PARTY_REASON_NOT_IN_PARTY);
							res.set_partyid(beforePartyId);
							res.set_version(version);
							self->Send(ClientPacketHandler::MakeSendBuffer(res));
						});

					if (!ok) return;

					// 떠난 사람은 party false
					SendPartyInfoTo(ps, 0);

					// 남은 파티원 갱신(Info만)
					if (!disbanded && after.partyId != 0)
						BroadcastPartyInfo(after.partyId);
				});
		});

	return true;
}

bool ClientPacketHandler::Handle_C_PARTY_KICK_REQ(PacketSessionRef& session, Protocol::C_PARTY_KICK_REQ& pkt)
{
	PlayerSessionRef ps = static_pointer_cast<PlayerSession>(session);
	if (!ps) return true;

	if (ps->IsMapChanging()) return true;

	const uint64 targetId = pkt.targetplayerid();

	ps->PostPlayer([targetId](PlayerSessionRef ps, PlayerRef player)
		{
			const uint64 leaderId = player->GetPlayerId();

			PartyActor::Instance().Push([ps, leaderId, targetId]()
				{
					auto& core = PartyActor::Instance().Core();

					PartyManagerCore::Party after;
					const bool ok = core.Kick(leaderId, targetId, after);

					const uint64 partyId = ok ? after.partyId : 0;
					const uint32 version = ok ? after.version : 0;

					// 요청자 결과
					ps->Post([ok, partyId, version](PlayerSessionRef self)
						{
							Protocol::S_PARTY_RESULT res;
							res.set_op((int32)Protocol::PARTY_OP_KICK);
							res.set_success(ok);
							res.set_reason(ok ? (int32)Protocol::PARTY_REASON_OK : (int32)Protocol::PARTY_REASON_INTERNAL_ERROR);
							res.set_partyid(partyId);
							res.set_version(version);
							self->Send(ClientPacketHandler::MakeSendBuffer(res));
						});

					if (!ok) return;

					// 남은 파티원 갱신(Info만)
					BroadcastPartyInfo(after.partyId);

					// 킥당한 사람: 파티 없음 (세션 Actor에서)
					auto kicked = GameSessionManager::GSessionManager->FindByPlayerId(targetId);
					if (kicked)
					{
						kicked->Post([](PlayerSessionRef self)
							{
								Protocol::S_PARTY_INFO_NTF off = MakeNoPartyInfoNtf();
								self->Send(ClientPacketHandler::MakeSendBuffer(off));
							});
					}
				});
		});

	return true;
}

bool ClientPacketHandler::Handle_C_PARTY_DISBAND_REQ(PacketSessionRef& session, Protocol::C_PARTY_DISBAND_REQ& pkt)
{
	PlayerSessionRef ps = static_pointer_cast<PlayerSession>(session);
	if (!ps) return true;

	if (ps->IsMapChanging()) return true;

	ps->PostPlayer([](PlayerSessionRef ps, PlayerRef player)
		{
			const uint64 leaderId = player->GetPlayerId();

			PartyActor::Instance().Push([ps, leaderId]()
				{
					auto& core = PartyActor::Instance().Core();

					PartyManagerCore::Party disbanded;
					const bool ok = core.Disband(leaderId, disbanded);

					const uint64 partyId = ok ? disbanded.partyId : 0;
					const uint32 version = ok ? disbanded.version : 0;

					// 요청자 결과
					ps->Post([ok, partyId, version](PlayerSessionRef self)
						{
							Protocol::S_PARTY_RESULT res;
							res.set_op((int32)Protocol::PARTY_OP_DISBAND);
							res.set_success(ok);
							res.set_reason(ok ? (int32)Protocol::PARTY_REASON_OK : (int32)Protocol::PARTY_REASON_INTERNAL_ERROR);
							res.set_partyid(partyId);
							res.set_version(version);
							self->Send(ClientPacketHandler::MakeSendBuffer(res));
						});

					if (!ok) return;

					// 전원에게 "파티 없음" (각 세션 Actor에서)
					for (uint64 id : disbanded.members)
					{
						auto ms = GameSessionManager::GSessionManager->FindByPlayerId(id);
						if (!ms) continue;

						ms->Post([](PlayerSessionRef self)
							{
								Protocol::S_PARTY_INFO_NTF off = MakeNoPartyInfoNtf();
								self->Send(ClientPacketHandler::MakeSendBuffer(off));
							});
					}
				});
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

	// 네트워크 스레드에서 빠른 컷(atomic read만)
	if (ps->IsMapChanging())
		return true;

	// 내 세션 상태는 무조건 세션 Actor로
	ps->PostPlayer([](PlayerSessionRef ps, PlayerRef me)
		{
			const uint64 myId = me->GetPlayerId();

			// 파티 스냅샷은 PartyActor에서만
			PartyActor::Instance().Push([ps, myId]()
				{
					auto& core = PartyActor::Instance().Core();

					const uint64 partyId = core.GetPartyIdByPlayerId(myId);
					if (partyId == 0)
					{
						// 응답은 "내 세션 Actor"에서만 Send
						ps->Post([](PlayerSessionRef self)
							{
								Protocol::S_PARTY_STATUS_NTF ntf;
								ntf.set_partyid(0);
								ntf.set_version(0);
								self->Send(ClientPacketHandler::MakeSendBuffer(ntf));
							});
						return;
					}

					PartyManagerCore::Party snap = core.GetSnapshot(partyId);
					const uint32 version = snap.version;

					std::vector<uint64> memberIds;
					memberIds.reserve(snap.members.size());
					for (uint64 id : snap.members)
						memberIds.push_back(id);

					// 이제부터는 요청자 세션 Actor에서 fan-out(각 세션에게 "자기 상태" 요청)
					ps->Post([partyId, version, memberIds = std::move(memberIds)](PlayerSessionRef requester) mutable
						{
							// 1) 온라인 세션만 추려서 요청할 대상 확정
							std::vector<PlayerSessionRef> targets;
							targets.reserve(memberIds.size());

							for (uint64 id : memberIds)
							{
								auto ms = GameSessionManager::GSessionManager->FindByPlayerId(id);
								if (ms)
									targets.push_back(ms);
							}

							auto collector = MakeShared<PartyStatusCollector>();
							collector->partyId = partyId;
							collector->version = version;
							collector->remaining = static_cast<int32>(targets.size());
							collector->items.reserve(targets.size());

							// 아무도 없으면(전원 오프라인 등) 빈 리스트로 응답
							if (collector->remaining == 0)
							{
								Protocol::S_PARTY_STATUS_NTF ntf;
								ntf.set_partyid(partyId);
								ntf.set_version(version);
								requester->Send(ClientPacketHandler::MakeSendBuffer(ntf));
								return;
							}

							// 2) 각 멤버 세션에게 "너 상태 네가 채워서 줘" (memberSession Actor에서만 player 접근)
							for (auto& memberSession : targets)
							{
								memberSession->PostPlayer([requester, collector](PlayerSessionRef ms, PlayerRef mp)
									{
										// 멤버 세션 Actor thread에서 "자기 상태"만 뽑는다
										PartyStatusItem item;
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

										// 결과 합치기는 요청자 세션 Actor에서만
										requester->Post([collector, item = std::move(item)](PlayerSessionRef req) mutable
											{
												collector->items.push_back(std::move(item));

												// 마지막 응답이면 최종 패킷 만들어서 Send (요청자 세션 Actor에서만)
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
							}
						});
				});
		});

	return true;
}
