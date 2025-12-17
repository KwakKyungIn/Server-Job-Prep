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
	PlayerSessionRef playerSession = static_pointer_cast<PlayerSession>(session);

	// 채팅도 순차 처리를 위해 JobQueue 사용
	playerSession->PushJob(ObjectPool<Job>::MakeShared([playerSession, pkt]()
		{
			// TODO: ChatServer 연결 확인 및 전송 로직
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

bool ClientPacketHandler::Handle_C_LOGIN(PacketSessionRef& session, Protocol::C_LOGIN& pkt)
{
	return false;
}