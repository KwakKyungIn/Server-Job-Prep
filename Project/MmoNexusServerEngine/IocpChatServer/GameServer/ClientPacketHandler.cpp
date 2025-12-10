#include "pch.h"
#include "ClientPacketHandler.h"
#include "S2SPacketHandler.h" 
#include "PlayerSession.h"
#include "Player.h"
#include "GameSessionManager.h"
#include "Job.h" 
#include "GameRoom.h" 
#include "RedisManager.h"

// Global DB Session Reference
extern shared_ptr<PacketSession> G_DBSession;

PacketHandlerFunc ClientPacketHandler::GPacketHandler[UINT16_MAX];

// [Test] 임시 테스트용 1번방 (Lazy Initialization용 nullptr 초기화)
shared_ptr<GameRoom> GTestRoom = nullptr;

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
		pos->set_x(0); pos->set_y(0); pos->set_z(0);

		player->Init(tempInfo);
	}

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

	// 4. [Lazy Init] 룸 생성 (테스트용)
	if (GTestRoom == nullptr)
	{
		GTestRoom = MakeShared<GameRoom>();
		GTestRoom->Init(1, 100, 100, 10);
	}

	// 주의: 아직 방에 넣지 않음. 
	// DB에서 데이터(좌표 등)가 로딩된 후(S2S_RES_LOAD_PLAYER_DATA)에 방에 넣는 것이 정석.
	// 하지만 지금 구조상 바로 넣고 싶다면 여기서 GTestRoom->PushJob(&GameRoom::Enter, playerSession); 호출 가능.
	// 일단 DB 로딩 완료 패킷 핸들러(S2SPacketHandler)에서 Enter 처리하는 것을 권장.

	return true;
}



// [MOVE] 이동 요청
bool ClientPacketHandler::Handle_C_MOVE(PacketSessionRef& session, Protocol::C_MOVE& pkt)
{
	PlayerSessionRef playerSession = static_pointer_cast<PlayerSession>(session);
	auto player = playerSession->GetPlayer();
	if (player == nullptr) return false;

	auto room = player->GetRoom();
	if (room) room->PushJob(&GameRoom::HandleMove, playerSession, pkt);
	return true;
}

bool ClientPacketHandler::Handle_C_USE_ITEM(PacketSessionRef& session, Protocol::C_USE_ITEM& pkt)
{
	return false;
}

// [ClientPacketHandler.cpp]

bool ClientPacketHandler::Handle_C_EQUIP_ITEM(PacketSessionRef& session, Protocol::C_EQUIP_ITEM& pkt)
{
	PlayerSessionRef playerSession = static_pointer_cast<PlayerSession>(session);

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

bool ClientPacketHandler::Handle_C_HEART_BEAT_REQ(PacketSessionRef& session, Protocol::C_HEART_BEAT_REQ& pkt)
{
	return true;
}

bool ClientPacketHandler::Handle_C_LOGIN(PacketSessionRef& session, Protocol::C_LOGIN& pkt)
{
	return false;
}