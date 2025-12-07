#include "pch.h"
#include "ClientPacketHandler.h"
#include "S2SPacketHandler.h" 
#include "PlayerSession.h"
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

	// 1. [Lazy Init] 룸이 없으면 생성 (서버 켜지고 최초 1회만 실행됨)
	if (GTestRoom == nullptr)
	{
		GTestRoom = MakeShared<GameRoom>();
		GTestRoom->Init(1, 100, 100);
		printf("[SERVER] GTestRoom Initialized (100x100 Grid).\n");
	}

	// 2. [Validation] (생략: DB 정보 대조 등)

	// 3. [Data Setup] 응답 패킷 및 세션 데이터 구성
	{
		Protocol::S_ENTER_GAME_RES resPkt;
		resPkt.set_success(true);

		// 플레이어 정보 생성
		Protocol::PlayerInfo* myInfo = resPkt.mutable_myplayer();
		uint64 assignedId = playerSession->GetSessionId(); // 임시 ID

		myInfo->set_playerid(assignedId);
		myInfo->set_name("TestPlayer_" + std::to_string(assignedId));

		// 스폰 좌표 설정 (벽이 없는 (50, 0, 50) 위치)
		myInfo->mutable_posinfo()->set_x(50.0f);
		myInfo->mutable_posinfo()->set_y(0.0f);
		myInfo->mutable_posinfo()->set_z(50.0f);

		// [CRITICAL] 패킷 보내기 전에 서버 세션에도 정보를 저장해야 함!
		// 그래야 GameRoom에서 "누가(ID)" 들어왔는지 식별 가능.
		playerSession->SetPlayerInfo(*myInfo);

		playerSession->Send(MakeSendBuffer(resPkt));
		printf("[SERVER] Player %llu Enter Game Success.\n", assignedId);
	}

	// 4. [Core Logic] 룸 입장 (Async Job)
	// 로직 스레드(JobQueue)에게 입장 처리를 위임
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
	// 좌표 검증 및 브로드캐스팅은 룸 스레드에서 순차적으로 실행됨
	room->PushJob(&GameRoom::HandleMove, playerSession, pkt);

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