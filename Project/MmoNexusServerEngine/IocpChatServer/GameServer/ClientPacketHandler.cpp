#include "pch.h"
#include "ClientPacketHandler.h"
#include "S2SPacketHandler.h" 
#include "PlayerSession.h"
#include "Player.h"
#include "GameSessionManager.h"
#include "Job.h" 
#include "GameRoom.h" 

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
bool ClientPacketHandler::Handle_C_ENTER_GAME_REQ(PacketSessionRef& session, Protocol::C_ENTER_GAME_REQ& pkt)
{
	PlayerSessionRef playerSession = static_pointer_cast<PlayerSession>(session);

	// 1. [Lazy Init] 룸 생성
	if (GTestRoom == nullptr)
	{
		GTestRoom = MakeShared<GameRoom>();
		GTestRoom->Init(1, 100, 100, 10);
		printf("[SERVER] GTestRoom Initialized (100x100 Grid, ZoneSize: 10).\n");
	}

	// 2. [Refactoring] Player 객체 가져오기 (로그인 시 생성됨)
	auto player = playerSession->GetPlayer();
	if (player == nullptr)
	{
		// 플레이어 객체가 없으면 진행 불가 (로그인 실패 상태)
		return true;
	}

	// 3. [Data Setup]
	{
		Protocol::S_ENTER_GAME_RES resPkt;
		resPkt.set_success(true);

		// [중요] 세션이 아니라 'Player 객체'의 데이터를 수정해야 함
		Protocol::PlayerInfo* myInfo = player->GetPlayerInfo();

		// DB에서 로드된 정보가 없다면 임시값 세팅 (있다면 이 부분은 생략 가능)
		// 현재는 테스트를 위해 좌표와 이름을 강제로 덮어씌움
		uint64 assignedId = playerSession->GetSessionId();
		if (myInfo->playerid() == 0) myInfo->set_playerid(assignedId);
		if (myInfo->name().empty()) myInfo->set_name("TestPlayer_" + std::to_string(assignedId));

		// 스폰 좌표 설정 (50, 0, 50)
		auto posInfo = myInfo->mutable_posinfo();
		posInfo->set_x(50.0f);
		posInfo->set_y(0.0f);
		posInfo->set_z(50.0f);

		// 클라이언트에게 보낼 정보 복사
		resPkt.mutable_myplayer()->CopyFrom(*myInfo);

		playerSession->Send(MakeSendBuffer(resPkt));
		printf("[SERVER] Player %llu Enter Game Success.\n", myInfo->playerid());
	}

	// 4. [Core Logic] 룸 입장 (Async Job)
	GTestRoom->PushJob(&GameRoom::Enter, playerSession);

	return true;
}
// [MOVE] 이동 요청
bool ClientPacketHandler::Handle_C_MOVE(PacketSessionRef& session, Protocol::C_MOVE& pkt)
{
	PlayerSessionRef playerSession = static_pointer_cast<PlayerSession>(session);

	// 1. 현재 유저가 있는 방 확인
	shared_ptr<GameRoom> room = playerSession->GetRoom();
	if (room == nullptr)
		return false;

	// 2. [Async Job] 룸에게 이동 처리 위임
	room->PushJob(&GameRoom::HandleMove, playerSession, pkt);

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
bool ClientPacketHandler::Handle_C_LOGIN_REQ(PacketSessionRef& session, Protocol::C_LOGIN_REQ& pkt)
{
	PlayerSessionRef playerSession = static_pointer_cast<PlayerSession>(session);

	// DB 처리는 오래 걸리므로 별도 Job으로 분리하여 실행
	playerSession->PushJob(ObjectPool<Job>::MakeShared([playerSession, pkt]()
		{
			uint64 mySessionId = playerSession->GetSessionId();

			// S2S 패킷 생성 -> DBAgent로 전송
			Protocol::S2S_REQ_LOGIN s2sPkt;
			s2sPkt.set_playersessionid(mySessionId);
			s2sPkt.set_name(pkt.name());

			if (G_DBSession && G_DBSession->IsConnected())
			{
				auto sendBuffer = S2SPacketHandler::MakeSendBuffer(s2sPkt);
				G_DBSession->Send(sendBuffer);
			}
		}));

	return true;
}

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