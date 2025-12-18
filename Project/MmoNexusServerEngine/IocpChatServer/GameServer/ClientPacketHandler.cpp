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
#include "PartyManager.h"
#include <atomic>
#include <chrono>

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
	PlayerSessionRef playerSession = static_pointer_cast<PlayerSession>(session);

	// 1. [Redis Check] 토큰 검증
	std::string token = pkt.token();
	std::string value = GRedisManager->Get(token); // Blocking Call

	if (value.empty())
	{
		printf("❌ [EnterGame] Invalid Token: %s\n", token.c_str());
		playerSession->Disconnect(L"Invalid Token");
		return false;
	}

	uint64 playerId = std::stoull(value);
	printf("✅ [EnterGame] Token Validated! PlayerID: %llu\n", playerId);

	// [NEW] 채널/맵 정보 읽기 (기본값 방어)
	int32 channelId = pkt.channelid();
	if (channelId <= 0) channelId = 1;

	int32 mapId = pkt.mapid();
	DataManager* dm = DataManager::Instance();
	if (dm == nullptr || dm->IsValidMapId(mapId) == false)
		mapId = (dm ? dm->GetDefaultMapId() : 1);

	const MapConfig* cfg = (dm ? dm->GetMapConfig(mapId) : nullptr);

	// 2. [Player Object] 생성
	shared_ptr<Player> player = ObjectPool<Player>::MakeShared();

	// [GIGACHAD FIX] Player::Init(const PlayerInfo& info) 시그니처 맞추기
	{
		Protocol::PlayerInfo tempInfo;
		tempInfo.set_playerid(playerId);
		tempInfo.set_name("Player_" + std::to_string(playerId)); // 임시 이름 (DB 로딩 전)

		// 좌표 등은 Player::Init 내부에서 방어코드(_playerInfo.has_posinfo 체크)가 있으므로 생략 가능
		// 하지만 확실하게 하기 위해 기본값 세팅
		auto pos = tempInfo.mutable_posinfo();
		pos->set_x(cfg ? cfg->spawnX : 50.f);
		pos->set_y(cfg ? cfg->spawnY : 0.f);
		pos->set_z(cfg ? cfg->spawnZ : 50.f);
		player->Init(tempInfo);
	}

	// [NEW] 선택한 채널/맵 저장
	player->SetChannelId(channelId);
	player->SetMapId(mapId);


	// 세션과 플레이어 연결
	playerSession->SetPlayer(player);
	player->SetSession(playerSession);

	GameSessionManager::GSessionManager->BindPlayerId(playerSession, playerId);


	// 3. [DB Loading] 데이터 로딩 요청 (비동기)
	if (G_DBSession)
	{
		// 스탯 정보 로딩
		Protocol::S2S_REQ_LOAD_PLAYER_DATA reqStat;
		reqStat.set_playerid(playerId);
		reqStat.set_gamesessionid(playerSession->GetSessionId());
		G_DBSession->Send(S2SPacketHandler::MakeSendBuffer(reqStat));

		// 아이템 정보 로딩
		Protocol::S2S_REQ_ITEMS_LOAD reqItem;
		reqItem.set_playerid(playerId);
		reqItem.set_gamesessionid(playerSession->GetSessionId());
		G_DBSession->Send(S2SPacketHandler::MakeSendBuffer(reqItem));
	}


	return true;
}



// [MOVE] 이동 요청
bool ClientPacketHandler::Handle_C_MOVE(PacketSessionRef& session, Protocol::C_MOVE& pkt)
{
	PlayerSessionRef playerSession = static_pointer_cast<PlayerSession>(session);

	if (playerSession->IsMapChanging())
		return true;


	auto player = playerSession->GetPlayer();

	if (player == nullptr)
	{
		printf("[C_MOVE] Player is null\n");
		return false;
	}

	auto room = player->GetRoom();
	if (room == nullptr)
	{
		printf("[C_MOVE] Room is null for Player %llu\n", player->GetPlayerId());
		return false;
	}

	printf("[C_MOVE] From Player %llu Pos:(%.1f,%.1f,%.1f)\n",
		player->GetPlayerId(),
		pkt.posinfo().x(), pkt.posinfo().y(), pkt.posinfo().z());

	if (room) room->PushJob(&GameRoom::HandleMove, playerSession, pkt);
	return true;
}

bool ClientPacketHandler::Handle_C_USE_ITEM(PacketSessionRef& session, Protocol::C_USE_ITEM& pkt)
{
	PlayerSessionRef ps = static_pointer_cast<PlayerSession>(session);
	PlayerRef player = ps->GetPlayer();
	if (player == nullptr) return false;

	// 맵 이동 중 입력 차단 (너가 원하던 “이동 중 상태”)
	if (ps->IsMapChanging())
		return false;

	auto room = player->GetRoom();
	if (room == nullptr)
		return false;

	// 로직 스레드에서 처리
	room->PushJob(&GameRoom::HandleUseItem, ps, pkt);
	return true;
}


// [ClientPacketHandler.cpp]

bool ClientPacketHandler::Handle_C_EQUIP_ITEM(PacketSessionRef& session, Protocol::C_EQUIP_ITEM& pkt)
{
	PlayerSessionRef playerSession = static_pointer_cast<PlayerSession>(session);

	if (playerSession->IsMapChanging()) return true;
	// 1. Player 검증
	auto player = playerSession->GetPlayer();
	if (player == nullptr) return false;

	// 2. 아이템 찾기 (메모리에서 검색)
	// Player의 _items 벡터를 순회하며 UID로 찾음
	Protocol::ItemInfo* targetItem = nullptr;

	// 주의: GetItems()는 vector&를 반환해야 수정 가능
	auto& items = player->GetItems();
	for (auto& item : items)
	{
		if (item.itemuid() == pkt.itemuid())
		{
			targetItem = &item;
			break;
		}
	}

	// 아이템이 없거나, 소유권이 없으면 패스
	if (targetItem == nullptr)
		return false;

	// TODO: 장착 가능한 부위인지, 레벨 제한은 없는지 등 검증 로직 필요 (지금은 생략)

	// 3. 상태 변경 (Toggle 혹은 패킷 값 따르기)
	// 패킷에 equip=true/false가 오므로 그걸 따른다.
	targetItem->set_isequipped(pkt.equip());

	// 4. [Core Logic] 스탯 재계산
	// 여기서 DataManager를 통해 공격력/방어력이 갱신됨
	player->RefreshStats();

	// 5. 결과 전송 A (장착 상태 변경 알림)
	{
		Protocol::S_EQUIP_ITEM resPkt;
		resPkt.set_itemuid(pkt.itemuid());
		resPkt.set_equipped(pkt.equip());
		resPkt.set_slotindex(pkt.slotindex()); // 혹시 필요하면

		playerSession->Send(MakeSendBuffer(resPkt));
	}

	// 6. 결과 전송 B (변경된 스탯 알림)
	{
		Protocol::S_CHANGE_STAT statPkt;
		statPkt.mutable_statinfo()->CopyFrom(*player->GetStatInfo());

		playerSession->Send(MakeSendBuffer(statPkt));
	}

	std::cout << "⚔️ [Equip] ItemUID: " << pkt.itemuid() << " Equipped: " << pkt.equip() << std::endl;

	return true;
}

// [LOGIN] 인증 요청

bool ClientPacketHandler::Handle_C_SKILL(PacketSessionRef& session, Protocol::C_SKILL& pkt)
{
	PlayerSessionRef playerSession = static_pointer_cast<PlayerSession>(session);

	if (playerSession->IsMapChanging())
		return true;


	PlayerRef player = playerSession->GetPlayer();
	if (player == nullptr) return false;

	if (player->GetStatInfo()->hp() <= 0) return false;

	auto room = player->GetRoom();
	if (room == nullptr) return false;

	// [Success] 이제 이 람다식은 GameRoom::PushJob(std::function<void()>) 으로 연결된다.
	room->PushJob([room, player, pkt]()
		{
			player->UseSkill(pkt.skillid());
		});

	return true;
}
// [CHAT] 채팅 요청
bool ClientPacketHandler::Handle_C_CHAT_REQ(PacketSessionRef& session, Protocol::C_CHAT_REQ& pkt)
{
	PlayerSessionRef ps = static_pointer_cast<PlayerSession>(session);
	if (!ps) return true;

	// 맵 전환 중이면 입력 차단(원하면 제거)
	if (ps->IsMapChanging())
		return true;

	const std::string msg = pkt.message();

	ps->PushJob(ObjectPool<Job>::MakeShared([ps, msg]()
		{
			auto player = ps->GetPlayer();
			if (!player) return;

			auto room = player->GetRoom();
			if (!room) return;

			Protocol::S_CHAT_NTF ntf;
			ntf.set_playerid(player->GetPlayerId());
			ntf.set_name(player->GetName());
			ntf.set_message(msg);

			// ✅ 여기서 Room이 "버퍼 복제" 해주거나,
			//    room 내부에서 멤버 순회하면서 MakeSendBuffer를 매번 호출해야 안전함.
			room->PushJob([room, ntf]()
				{
					room->BroadcastChat(ntf); // 아래 GameRoom 패치 참고
				});
		}));

	return true;
}


//맵이동
bool ClientPacketHandler::Handle_C_MAP_CHANGE_REQ(PacketSessionRef& session, Protocol::C_MAP_CHANGE_REQ& pkt)
{
	PlayerSessionRef playerSession = static_pointer_cast<PlayerSession>(session);
	PlayerRef player = playerSession->GetPlayer();
	if (player == nullptr)
		return false;

	// 이미 진행 중이면 중복 요청 무시
	if (playerSession->IsMapChanging())
		return true;

	const int32 targetMapId = pkt.targetmapid();

	// 맵 유효성 체크
	DataManager* dm = DataManager::Instance();
	if (dm == nullptr || dm->IsValidMapId(targetMapId) == false)
		return false;

	// 같은 맵이면 무시
	if (player->GetMapId() == targetMapId)
		return true;

	const MapConfig* cfg = dm->GetMapConfig(targetMapId);
	if (cfg == nullptr)
		return false;

	// 목적지 스폰
	Protocol::PositionInfo spawn;
	spawn.set_x(cfg->spawnX);
	spawn.set_y(cfg->spawnY);
	spawn.set_z(cfg->spawnZ);

	// 토큰 생성 + 세션에 pending 저장
	const uint64 token = MakeMapChangeToken(player->GetPlayerId(), playerSession->GetSessionId());
	if (playerSession->TryBeginMapChange(token, targetMapId, spawn) == false)
		return true;

	// BEGIN 전송 (클라: 입력락/로딩 시작)
	Protocol::S_MAP_CHANGE_BEGIN beginPkt;
	beginPkt.set_token(token);
	beginPkt.set_targetmapid(targetMapId);
	beginPkt.mutable_spawn()->CopyFrom(spawn);

	playerSession->Send(MakeSendBuffer(beginPkt));

	printf("🗺️ [MapChange][BEGIN] Player %llu -> Map %d (token=%llu)\n",
		player->GetPlayerId(), targetMapId, token);

	return true;
}


bool ClientPacketHandler::Handle_C_MAP_CHANGE_ACK(PacketSessionRef& session, Protocol::C_MAP_CHANGE_ACK& pkt)
{
	PlayerSessionRef playerSession = static_pointer_cast<PlayerSession>(session);
	PlayerRef player = playerSession->GetPlayer();
	if (player == nullptr)
		return false;

	const uint64 token = pkt.token();

	// 여기 out으로 pending을 꺼내면서 상태를 SWITCHING으로 전환
	int32 targetMapId = 0;
	Protocol::PositionInfo spawn;
	if (playerSession->TryConsumeMapChangeAck(token, targetMapId, spawn) == false)
		return false;

	// 목적지 룸 확보
	const int32 channelId = player->GetChannelId();
	auto newRoom = GRoomManager->GetOrCreateRoom(channelId, targetMapId);
	if (newRoom == nullptr)
	{
		playerSession->CancelMapChange();
		return false;
	}

	auto oldRoom = player->GetRoom();
	if (oldRoom == nullptr)
	{
		// 예외: 아직 룸이 없으면(입장 직후 타이밍) 그냥 새 룸으로 들어간다
		player->SetMapId(targetMapId);
		player->GetPosInfo()->CopyFrom(spawn);

		newRoom->PushJob(&GameRoom::EnterMapChange, playerSession);

		printf("🗺️ [MapChange][ACK] Player %llu oldRoom=null -> Enter newRoom(map=%d)\n",
			player->GetPlayerId(), targetMapId);

		return true;
	}

	// 핵심: Leave는 oldRoom 스레드에서, Enter는 newRoom 스레드에서
	oldRoom->PushJob([playerSession, player, oldRoom, newRoom, targetMapId, spawn]()
		{
			// 1) oldRoom에서 나가기
			oldRoom->Leave(playerSession);

			// 2) 메타/좌표 갱신
			player->SetMapId(targetMapId);
			player->GetPosInfo()->CopyFrom(spawn);

			// 3) newRoom 입장
			// IMPORTANT:
			// - 여기서 newRoom의 Enter가 S_MAP_CHANGE_END를 보내도록 GameRoom.cpp를 조정해야 함.
			// - 그리고 그 시점에 playerSession->EndMapChange() 호출해서 입력락 풀어야 "2-step"이 완성된다.
			newRoom->PushJob(&GameRoom::EnterMapChange, playerSession);
		});

	printf("🗺️ [MapChange][ACK] Player %llu -> Map %d (token=%llu)\n",
		player->GetPlayerId(), targetMapId, token);

	return true;
}


bool ClientPacketHandler::Handle_C_HEART_BEAT_REQ(PacketSessionRef& session, Protocol::C_HEART_BEAT_REQ& pkt)
{
	return true;
}



bool ClientPacketHandler::Handle_C_PARTY_CHAT_REQ(PacketSessionRef& session, Protocol::C_PARTY_CHAT_REQ& pkt)
{
	PlayerSessionRef ps = static_pointer_cast<PlayerSession>(session);
	if (!ps) return true;

	const std::string msg = pkt.message();

	ps->PushJob(ObjectPool<Job>::MakeShared([ps, msg]()
		{
			auto player = ps->GetPlayer();
			if (!player) return;

			const uint64 senderId = player->GetPlayerId();
			const std::string senderName = player->GetName();

			const uint64 partyId = PartyManager::Instance().GetPartyIdByPlayerId(senderId);
			if (partyId == 0)
			{
				// TODO : 파티 없음 -> 무시 or 실패 패킷 보내도 됨(지금은 무시)
				return;
			}

			// 멤버 목록 뽑아서 라우팅
			std::vector<uint64> members;
			PartyManager::Instance().GetMembers(partyId, members);

			for (uint64 memberId : members)
			{
				PlayerSessionRef target =
					GameSessionManager::GSessionManager->FindByPlayerId(memberId);

				if (!target) continue;

				Protocol::S_PARTY_CHAT_NTF ntf;
				ntf.set_partyid(partyId);
				ntf.set_senderid(senderId);
				ntf.set_sendername(senderName);
				ntf.set_message(msg);

				auto sb = ClientPacketHandler::MakeSendBuffer(ntf);
				target->Send(sb);
			}
		}));

	return true;
}


bool ClientPacketHandler::Handle_C_LOGIN(PacketSessionRef& session, Protocol::C_LOGIN& pkt)
{
	return false;
}

//=======================================파티===========================================

// 파티 스냅샷 -> S_PARTY_INFO_NTF
static Protocol::S_PARTY_INFO_NTF MakePartyInfoNtf(const PartyManager::Party& p)
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

// (해산/추방 등) 파티 없음 통지
static Protocol::S_PARTY_INFO_NTF MakeNoPartyInfoNtf()
{
	Protocol::S_PARTY_INFO_NTF ntf;
	ntf.set_hasparty(false);
	ntf.set_partyid(0);
	ntf.set_leaderid(0);
	ntf.set_version(0);
	return ntf;
}

static void SendPartyInfoTo(PlayerSessionRef target, uint64 partyId)
{
	Protocol::S_PARTY_INFO_NTF info;

	if (partyId == 0)
	{
		info.set_hasparty(false);
		target->Send(ClientPacketHandler::MakeSendBuffer(info));
		return;
	}

	auto snap = PartyManager::Instance().GetSnapshot(partyId);
	if (snap.partyId == 0)
	{
		info.set_hasparty(false);
		target->Send(ClientPacketHandler::MakeSendBuffer(info));
		return;
	}

	info.set_hasparty(true);
	info.set_partyid(snap.partyId);
	info.set_leaderid(snap.leaderId);
	info.set_version(snap.version);

	for (uint64 id : snap.members)
		info.add_memberids(id);

	target->Send(ClientPacketHandler::MakeSendBuffer(info));
}

static void BroadcastPartyInfoAndStatus(uint64 partyId)
{
	auto snap = PartyManager::Instance().GetSnapshot(partyId);
	if (snap.partyId == 0) return;

	// 1. Info 패킷 미리 생성
	Protocol::S_PARTY_INFO_NTF infoPkt = MakePartyInfoNtf(snap);

	// 2. Status 패킷 준비
	Protocol::S_PARTY_STATUS_NTF statusPkt;
	statusPkt.set_partyid(partyId);
	statusPkt.set_version(snap.version);

	// 3. 한 번의 순회로 처리
	for (uint64 id : snap.members)
	{
		auto ps = GameSessionManager::GSessionManager->FindByPlayerId(id);
		if (!ps) continue;

		auto p = ps->GetPlayer();
		if (!p) continue;

		// Status 멤버 추가
		auto* m = statusPkt.add_members();
		m->set_playerid(id);
		m->set_name(p->GetName());
		auto st = p->GetStatInfo();
		m->set_level(st ? st->level() : 1);
		m->set_hp(st ? st->hp() : 0);
		m->set_maxhp(st ? st->maxhp() : 0);
	}

	// 4. 전송 (각 세션의 Job 큐 활용)
	for (uint64 id : snap.members)
	{
		auto ps = GameSessionManager::GSessionManager->FindByPlayerId(id);
		if (!ps) continue;

		ps->PushJob(ObjectPool<Job>::MakeShared([ps, infoPkt, statusPkt]() mutable
			{
				ps->Send(ClientPacketHandler::MakeSendBuffer(infoPkt));
				ps->Send(ClientPacketHandler::MakeSendBuffer(statusPkt));
			}));
	}
}


bool ClientPacketHandler::Handle_C_PARTY_CREATE_REQ(PacketSessionRef& session, Protocol::C_PARTY_CREATE_REQ& pkt)
{
	PlayerSessionRef ps = static_pointer_cast<PlayerSession>(session);
	if (!ps) return true;

	if (ps->IsMapChanging())
		return true;

	ps->PushJob(ObjectPool<Job>::MakeShared([ps]()
		{
			auto player = ps->GetPlayer();
			if (!player) return;

			uint64 leaderId = player->GetPlayerId();

			// 이미 파티 있으면 실패
			if (PartyManager::Instance().GetPartyIdByPlayerId(leaderId) != 0)
			{
				Protocol::S_PARTY_RESULT res;
				res.set_op(Protocol::PARTY_OP_CREATE);
				res.set_success(false);
				res.set_reason(Protocol::PARTY_REASON_ALREADY_IN_PARTY);
				ps->Send(ClientPacketHandler::MakeSendBuffer(res));
				return;
			}

			uint64 partyId = 0;
			bool ok = PartyManager::Instance().Create(leaderId, partyId);

			Protocol::S_PARTY_RESULT res;
			res.set_op(Protocol::PARTY_OP_CREATE);
			res.set_success(ok);
			res.set_reason(ok ? Protocol::PARTY_REASON_OK : Protocol::PARTY_REASON_INTERNAL_ERROR);
			res.set_partyid(partyId);
			res.set_version(ok ? PartyManager::Instance().GetSnapshot(partyId).version : 0);
			ps->Send(ClientPacketHandler::MakeSendBuffer(res));

			if (ok)
				BroadcastPartyInfoAndStatus(partyId);
		}));

	return true;
}

bool ClientPacketHandler::Handle_C_PARTY_INVITE_REQ(PacketSessionRef& session, Protocol::C_PARTY_INVITE_REQ& pkt)
{
	PlayerSessionRef ps = static_pointer_cast<PlayerSession>(session);
	if (!ps) return true;

	if (ps->IsMapChanging())
		return true;

	const uint64 targetId = pkt.targetplayerid();

	// Handle_C_PARTY_INVITE_REQ 수정
	ps->PushJob(ObjectPool<Job>::MakeShared([ps, targetId]()
		{
			auto player = ps->GetPlayer();
			if (!player) return;

			const uint64 inviterId = player->GetPlayerId();
			const std::string inviterName = player->GetName(); // ✅ 여기서 미리 복사

			// 기본 검증
			if (targetId == 0 || targetId == inviterId)
			{
				Protocol::S_PARTY_RESULT res;
				res.set_op(Protocol::PARTY_OP_INVITE);
				res.set_success(false);
				res.set_reason(Protocol::PARTY_REASON_SELF_TARGET);
				ps->Send(ClientPacketHandler::MakeSendBuffer(res));
				return;
			}

			uint64 partyId = PartyManager::Instance().GetPartyIdByPlayerId(inviterId);
			if (partyId == 0)
			{
				Protocol::S_PARTY_RESULT res;
				res.set_op(Protocol::PARTY_OP_INVITE);
				res.set_success(false);
				res.set_reason(Protocol::PARTY_REASON_NOT_IN_PARTY);
				ps->Send(ClientPacketHandler::MakeSendBuffer(res));
				return;
			}

			// 대상 온라인 확인
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

			PartyManager::PendingInvite inv;
			bool ok = PartyManager::Instance().Invite(inviterId, targetId, inv);

			Protocol::S_PARTY_RESULT res;
			res.set_op(Protocol::PARTY_OP_INVITE);
			res.set_success(ok);
			res.set_reason(ok ? Protocol::PARTY_REASON_OK : Protocol::PARTY_REASON_ALREADY_IN_PARTY);
			res.set_partyid(partyId);
			res.set_version(PartyManager::Instance().GetSnapshot(partyId).version);
			ps->Send(ClientPacketHandler::MakeSendBuffer(res));

			if (!ok) return;

			// ✅ 대상 세션의 Job 큐를 통해 안전하게 전송
			targetSession->PushJob(ObjectPool<Job>::MakeShared(
				[targetSession, partyId, inviterId, inviterName]() // inviterName이 이제 정의됨!
				{
					Protocol::S_PARTY_INVITE_NTF ntf;
					ntf.set_partyid(partyId);
					ntf.set_inviterid(inviterId);
					ntf.set_invitername(inviterName);
					targetSession->Send(ClientPacketHandler::MakeSendBuffer(ntf));
				}));
		}));

	return true;
}

bool ClientPacketHandler::Handle_C_PARTY_INVITE_ACCEPT_REQ(PacketSessionRef& session, Protocol::C_PARTY_INVITE_ACCEPT_REQ& pkt)
{
	PlayerSessionRef ps = static_pointer_cast<PlayerSession>(session);
	if (!ps) return true;



	const uint64 partyId = pkt.partyid();
	const bool accept = pkt.accept();

	ps->PushJob(ObjectPool<Job>::MakeShared([ps, partyId, accept]()
		{
			auto player = ps->GetPlayer();
			if (!player) return;

			const uint64 targetId = player->GetPlayerId();

			PartyManager::Party after;
			bool ok = PartyManager::Instance().AcceptInvite(targetId, partyId, accept, after);

			Protocol::S_PARTY_RESULT res;
			res.set_op(accept ? Protocol::PARTY_OP_INVITE_ACCEPT : Protocol::PARTY_OP_INVITE_REJECT);
			res.set_success(ok);
			res.set_reason(ok ? (accept ? Protocol::PARTY_REASON_OK : Protocol::PARTY_REASON_REJECTED)
				: Protocol::PARTY_REASON_INVALID_PARTY);
			res.set_partyid(partyId);
			res.set_version(after.partyId ? after.version : 0);

			ps->Send(ClientPacketHandler::MakeSendBuffer(res));

			if (ok && accept && after.partyId != 0)
				BroadcastPartyInfoAndStatus(after.partyId);

			if (ok && !accept)
				SendPartyInfoTo(ps, PartyManager::Instance().GetPartyIdByPlayerId(targetId)); // 보통 0
		}));

	return true;
}

bool ClientPacketHandler::Handle_C_PARTY_LEAVE_REQ(PacketSessionRef& session, Protocol::C_PARTY_LEAVE_REQ& pkt)
{
	PlayerSessionRef ps = static_pointer_cast<PlayerSession>(session);
	if (!ps) return true;

	if (ps->IsMapChanging())
		return true;

	ps->PushJob(ObjectPool<Job>::MakeShared([ps]()
		{
			auto player = ps->GetPlayer();
			if (!player) return;

			const uint64 pid = player->GetPlayerId();
			uint64 partyId = PartyManager::Instance().GetPartyIdByPlayerId(pid);

			PartyManager::Party after;
			bool disbanded = false;
			bool ok = PartyManager::Instance().Leave(pid, after, disbanded);

			Protocol::S_PARTY_RESULT res;
			res.set_op(Protocol::PARTY_OP_LEAVE);
			res.set_success(ok);
			res.set_reason(ok ? Protocol::PARTY_REASON_OK : Protocol::PARTY_REASON_NOT_IN_PARTY);
			res.set_partyid(partyId);
			res.set_version(after.partyId ? after.version : 0);
			ps->Send(ClientPacketHandler::MakeSendBuffer(res));

			// 떠난 사람은 party false로
			SendPartyInfoTo(ps, 0);

			// 남은 파티원 갱신
			if (ok && !disbanded && after.partyId != 0)
				BroadcastPartyInfoAndStatus(after.partyId);
			if (ok && disbanded)
			{
				// 파티가 터졌으면 남은 사람 없음(0)이라 추가 브로드캐스트 필요 없음
			}
		}));

	return true;
}

bool ClientPacketHandler::Handle_C_PARTY_KICK_REQ(PacketSessionRef& session, Protocol::C_PARTY_KICK_REQ& pkt)
{
	PlayerSessionRef ps = static_pointer_cast<PlayerSession>(session);
	if (!ps) return true;
	if (ps->IsMapChanging()) return true;

	const uint64 targetId = pkt.targetplayerid();

	ps->PushJob(ObjectPool<Job>::MakeShared([ps, targetId]()
		{
			auto player = ps->GetPlayer();
			if (!player) return;

			const uint64 leaderId = player->GetPlayerId();
			PartyManager::Party partyAfter;
			const bool ok = PartyManager::Instance().Kick(leaderId, targetId, partyAfter);

			// 결과 응답(요청자)
			{
				Protocol::S_PARTY_RESULT res;
				res.set_op((int32)Protocol::PARTY_OP_KICK);
				res.set_success(ok);
				res.set_reason(ok ? (int32)Protocol::PARTY_REASON_OK : (int32)Protocol::PARTY_REASON_INTERNAL_ERROR);
				res.set_partyid(ok ? partyAfter.partyId : 0);
				res.set_version(ok ? partyAfter.version : 0);
				ps->Send(ClientPacketHandler::MakeSendBuffer(res));
			}

			if (!ok) return;

			// ✅ 남은 파티원: Info + Status 둘 다 갱신
			BroadcastPartyInfoAndStatus(partyAfter.partyId);

			// ✅ 킥당한 사람: 파티 없음만 따로
			{
				auto kicked = GameSessionManager::GSessionManager->FindByPlayerId(targetId);
				if (kicked)
				{
					kicked->PushJob(ObjectPool<Job>::MakeShared([kicked]() {
						Protocol::S_PARTY_INFO_NTF off = MakeNoPartyInfoNtf();
						kicked->Send(ClientPacketHandler::MakeSendBuffer(off));
						}));
				}
			}

		}));

	return true;
}

bool ClientPacketHandler::Handle_C_PARTY_DISBAND_REQ(PacketSessionRef& session, Protocol::C_PARTY_DISBAND_REQ& pkt)
{
	PlayerSessionRef ps = static_pointer_cast<PlayerSession>(session);
	if (!ps) return true;
	if (ps->IsMapChanging()) return true;

	ps->PushJob(ObjectPool<Job>::MakeShared([ps]()
		{
			auto player = ps->GetPlayer();
			if (!player) return;

			const uint64 leaderId = player->GetPlayerId();
			PartyManager::Party disbanded;
			const bool ok = PartyManager::Instance().Disband(leaderId, disbanded);

			// 결과 응답(요청자)
			{
				Protocol::S_PARTY_RESULT res;
				res.set_op((int32)Protocol::PARTY_OP_DISBAND);
				res.set_success(ok);
				res.set_reason(ok ? (int32)Protocol::PARTY_REASON_OK : (int32)Protocol::PARTY_REASON_INTERNAL_ERROR);
				res.set_partyid(ok ? disbanded.partyId : 0);
				res.set_version(ok ? disbanded.version : 0);
				ps->Send(ClientPacketHandler::MakeSendBuffer(res));
			}

			if (!ok) return;

			// 전원에게 "파티 없음"

			for (uint64 id : disbanded.members)
			{
				auto ms = GameSessionManager::GSessionManager->FindByPlayerId(id);
				if (!ms) continue;

				// 패킷은 값으로 캡처하고, SendBuffer는 Job 안에서 "매번 새로" 만든다.
				ms->PushJob(ObjectPool<Job>::MakeShared([ms]() {
					Protocol::S_PARTY_INFO_NTF off = MakeNoPartyInfoNtf();
					ms->Send(ClientPacketHandler::MakeSendBuffer(off));
					}));
			}

		}));

	return true;
}
bool ClientPacketHandler::Handle_C_PARTY_STATUS_REQ(PacketSessionRef& session, Protocol::C_PARTY_STATUS_REQ& pkt)
{
	PlayerSessionRef ps = static_pointer_cast<PlayerSession>(session);
	if (!ps) return true;

	if (ps->IsMapChanging())
		return true;

	ps->PushJob(ObjectPool<Job>::MakeShared([ps]()
		{
			auto me = ps->GetPlayer();
			if (!me) return;

			const uint64 myId = me->GetPlayerId();
			const uint64 partyId = PartyManager::Instance().GetPartyIdByPlayerId(myId);

			// 파티 없음이면 빈 스냅샷
			if (partyId == 0)
			{
				Protocol::S_PARTY_STATUS_NTF ntf;
				ntf.set_partyid(0);
				ntf.set_version(0);
				ps->Send(ClientPacketHandler::MakeSendBuffer(ntf));
				return;
			}

			// 스냅샷 + 멤버별 상태 채우기
			PartyManager::Party snap = PartyManager::Instance().GetSnapshot(partyId);

			Protocol::S_PARTY_STATUS_NTF ntf;
			ntf.set_partyid(snap.partyId);
			ntf.set_version(snap.version);

			for (uint64 memberId : snap.members)
			{
				auto ms = GameSessionManager::GSessionManager->FindByPlayerId(memberId);
				if (!ms) continue;

				auto mp = ms->GetPlayer();
				if (!mp) continue;

				auto* st = ntf.add_members();
				st->set_playerid(memberId);

				// objectId: Creature에 GetObjectId() 있으면 그거, 없으면 playerId로 대체해도 됨
				st->set_objectid(mp->GetObjectId()); // 없으면 컴파일 에러 -> 그때 0 또는 playerId로 바꿔

				st->set_name(mp->GetName());

				// StatInfo: Creature 쪽 GetStatInfo() 쓰는 구조로 너 이미 쓰고 있었지
				st->set_level(mp->GetStatInfo()->level());
				st->set_hp(mp->GetStatInfo()->hp());
				st->set_maxhp(mp->GetStatInfo()->maxhp());

				st->set_mapid(mp->GetMapId());
				st->set_channelid(mp->GetChannelId());

				// 위치 포함
				st->mutable_posinfo()->CopyFrom(*mp->GetPosInfo());
			}

			// 패킷 한 번만 생성
			auto buffer = ClientPacketHandler::MakeSendBuffer(ntf);

			for (uint64 memberId : snap.members)
			{
				auto ms = GameSessionManager::GSessionManager->FindByPlayerId(memberId);
				if (!ms) continue;
				ms->Send(ClientPacketHandler::MakeSendBuffer(ntf)); // ✅ 세션마다 새로 생성
			}
		}));

	return true;
}
