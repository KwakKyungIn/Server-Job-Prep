#include "pch.h"
#include "ThreadManager.h"
#include "Service.h"
#include "Session.h"
#include "PlayerSession.h"
#include "BufferWriter.h"
#include "ClientPacketHandler.h"
#include "S2SPacketHandler.h"
#include "DBSession.h"
#include "GameRoom.h" // [GIGACHAD FIX] 이거 없으면 GameRoom 모름
#include <iostream>
#include <windows.h>
#include "LoginSession.h"
#include "RoomManager.h" // [NEW]
#include "DataManager.h"
#include "LobbyRoom.h"
#include <fstream> 
#include "PersistenceService.h"
#include "AutoCommitService.h"




// [External Reference] ClientPacketHandler.cpp에 있는 그 놈 가져오기
//extern shared_ptr<GameRoom> GTestRoom;
extern shared_ptr<LoginSession> G_LoginSession;
extern std::shared_ptr<PacketSession> G_DBSession;
extern std::atomic<bool> GIsRunning; // 전역 변수 참조
extern RedisManager* GRedisManager;


// [Ctrl+C 핸들러]
BOOL WINAPI CtrlHandler(DWORD fdwCtrlType)
{
	switch (fdwCtrlType)
	{
	case CTRL_C_EVENT:
	case CTRL_CLOSE_EVENT:
		std::cout << "🛑 [System] Server Shutdown Initiated..." << std::endl;
		GIsRunning = false;
		return TRUE;
	default:
		return FALSE;
	}
}


//=========================================임시 콘솔 테스트==========================================
#include <sstream>
#include <string>
#include "GameSessionManager.h"


void ConsoleThread()
{
	while (GIsRunning)
	{
		std::string line;
		std::getline(std::cin, line);
		if (line.empty()) continue;

		std::istringstream iss(line);
		std::string cmd;
		iss >> cmd;

		// 1. 세션 찾기 유틸리티 (명령어 첫 번째 인자는 무조건 playerId라고 가정)
		auto GetSession = [&](uint64 pid) -> PlayerSessionRef {
			auto session = GameSessionManager::GSessionManager->FindByPlayerId(pid);
			if (session == nullptr) {
				std::cout << "❌ [Test] Player " << pid << " not found in SessionManager!" << std::endl;
			}
			return session;
			};

		if (cmd == "/dummy") // 예: /dummy 1 (1번 유저를 서버에 강제로 앉힘)
		{
			uint64 pid; iss >> pid;
			// 실제 세션은 없지만, 테스트를 위해 빈 세션을 하나 만들어서 등록
			auto dummySession = MakeShared<PlayerSession>(/* 필요한 인자 */);
			// 주의: 실제 소켓이 없으므로 Send 시 터질 수 있음. 
			// 가장 좋은 건 클라 1~2개는 띄워놓고 해당 ID로 테스트하는 거다.
		}

		// 2. 명령어 분기
		if (cmd == "/p_create") // 예: /p_create 1 (1번 플레이어가 파티 생성)
		{
			uint64 pid; iss >> pid;
			auto session = GetSession(pid);
			if (session) {
				Protocol::C_PARTY_CREATE_REQ pkt;
				PacketSessionRef ps = static_pointer_cast<PacketSession>(session);
				ClientPacketHandler::Handle_C_PARTY_CREATE_REQ(ps, pkt);
			}
		}
		else if (cmd == "/p_invite") // 예: /p_invite 1 2 (1번이 2번을 초대)
		{
			uint64 inviterId, targetId;
			iss >> inviterId >> targetId;
			auto session = GetSession(inviterId);
			if (session) {
				Protocol::C_PARTY_INVITE_REQ pkt;
				pkt.set_targetplayerid(targetId);
				PacketSessionRef ps = static_pointer_cast<PacketSession>(session);
				ClientPacketHandler::Handle_C_PARTY_INVITE_REQ(ps, pkt);
			}
		}
		else if (cmd == "/p_accept") // 예: /p_accept 2 1 1 (2번이 1번파티 수락. 마지막1은 true)
		{
			uint64 pid, partyId; bool accept;
			iss >> pid >> partyId >> accept;
			auto session = GetSession(pid);
			if (session) {
				Protocol::C_PARTY_INVITE_ACCEPT_REQ pkt;
				pkt.set_partyid(partyId);
				pkt.set_accept(accept);
				PacketSessionRef ps = static_pointer_cast<PacketSession>(session);
				ClientPacketHandler::Handle_C_PARTY_INVITE_ACCEPT_REQ(ps, pkt);
			}
		}
		else if (cmd == "/p_status") // 예: /p_status 1 (1번이 속한 파티 상태 갱신/조회)
		{
			uint64 pid; iss >> pid;
			auto session = GetSession(pid);
			if (session) {
				Protocol::C_PARTY_STATUS_REQ pkt;
				PacketSessionRef ps = static_pointer_cast<PacketSession>(session);
				ClientPacketHandler::Handle_C_PARTY_STATUS_REQ(ps, pkt);
			}
		}
		else if (cmd == "/p_leave") // 예: /p_leave 2 (2번 유저가 파티 탈퇴)
		{
			uint64 pid; iss >> pid;
			auto session = GetSession(pid);
			if (session) {
				Protocol::C_PARTY_LEAVE_REQ pkt;
				PacketSessionRef ps = static_pointer_cast<PacketSession>(session);
				ClientPacketHandler::Handle_C_PARTY_LEAVE_REQ(ps, pkt);
			}
		}
		else if (cmd == "/p_kick") // 예: /p_kick 1 2 (리더 1번이 2번을 추방)
		{
			uint64 leaderId, targetId;
			iss >> leaderId >> targetId;
			auto session = GetSession(leaderId);
			if (session) {
				Protocol::C_PARTY_KICK_REQ pkt;
				pkt.set_targetplayerid(targetId);
				PacketSessionRef ps = static_pointer_cast<PacketSession>(session);
				ClientPacketHandler::Handle_C_PARTY_KICK_REQ(ps, pkt);
			}
		}
		else if (cmd == "/p_disband") // 예: /p_disband 1 (리더 1번이 파티 해산)
		{
			uint64 pid; iss >> pid;
			auto session = GetSession(pid);
			if (session) {
				Protocol::C_PARTY_DISBAND_REQ pkt;
				PacketSessionRef ps = static_pointer_cast<PacketSession>(session);
				ClientPacketHandler::Handle_C_PARTY_DISBAND_REQ(ps, pkt);
			}
		}
		// 필요에 따라 /p_leave, /p_kick 등 추가 가능
	}
}

int main()
{
	SetConsoleCtrlHandler(CtrlHandler, TRUE);

	ClientPacketHandler::Init();
	S2SPacketHandler::Init();

	{
		// ✅ 1) exe 옆 "Maps.json" 존재/오픈 검증 (CWD 기준)
		std::ifstream ifs("Maps.json");
		if (!ifs.is_open())
		{
			std::cout << "❌ [GameServer] Maps.json not found (expected next to exe). "
				"Fallback InitMapRegistry() will be used.\n";
		}
		else
		{
			std::cout << "✅ [GameServer] Maps.json found.\n";
		}
		// 파일 핸들은 여기서 닫아도 되고(스코프 종료), 명시적으로 닫아도 됨
		// ifs.close();

		// ✅ 2) 실제 로드 (DataManager 내부에서 다시 열어서 파싱)
		DataManager* dm = DataManager::Instance();
		if (!dm->LoadMapConfigsFromJson("Maps.json"))
		{
			std::cout << "⚠️ [GameServer] Maps.json load failed. fallback InitMapRegistry() will be used.\n";
		}
		else
		{
			std::cout << "✅ [GameServer] Maps.json loaded.\n";
		}
	}




	IocpCoreRef core = MakeShared<IocpCore>();

	GameSessionManager::GSessionManager = new GameSessionManager();


	ClientServiceRef dbService = MakeShared<ClientService>(
		NetAddress(L"127.0.0.1", 7778),
		core,
		MakeShared<DBSession>,
		1
	);

	/*ClientServiceRef chatService = MakeShared<ClientService>(
		NetAddress(L"127.0.0.1", 7776),
		core,
		MakeShared<ChatSession>,
		1
	);*/

	ServerServiceRef gameService = MakeShared<ServerService>(
		NetAddress(L"127.0.0.1", 7777),
		core,
		MakeShared<PlayerSession>,
		1000
	);

	ClientServiceRef loginService = MakeShared<ClientService>(
		NetAddress(L"127.0.0.1", 7780), // LoginServer의 내부 통신 포트
		core,
		MakeShared<LoginSession>,
		1
	);

	ASSERT_CRASH(dbService->Start());
	//ASSERT_CRASH(chatService->Start());
	ASSERT_CRASH(loginService->Start());
	ASSERT_CRASH(gameService->Start());

	GRoomManager = MakeShared<RoomManager>();

	Persistence::PersistenceService::I().Init(GRedisManager);

	Persistence::AutoCommitService::I().Init(
		GRedisManager,
		[](const Protocol::S2S_REQ_SAVE_PLAYER_CORE& pkt)
		{
			if (!G_DBSession) return;
			auto tmp = pkt;
			auto sb = S2SPacketHandler::MakeSendBuffer(tmp);
			G_DBSession->Send(sb);
		},
		[](const Protocol::S2S_REQ_SAVE_INVENTORY& pkt)
		{
			if (!G_DBSession) return;
			auto tmp = pkt;
			auto sb = S2SPacketHandler::MakeSendBuffer(tmp);
			G_DBSession->Send(sb);
		}
	);


	Persistence::AutoCommitService::I().Start();

	GThreadManager->Launch([=]() { ConsoleThread(); });

	std::cout << "✅ [GameServer] Running... (Press Ctrl+C to quit)" << std::endl;
	std::cout << "💬 [Command] /p_create [pid], /p_invite [inviter] [target], /p_accept [pid] [partyId] [1/0]" << std::endl;

	// 스레드 런칭 로직 (기존 유지)
	int32 threadCount = std::thread::hardware_concurrency();
	if (threadCount < 2) threadCount = 2;

	int32 networkThreadCount = threadCount / 2;
	int32 logicThreadCount = threadCount - networkThreadCount;

	for (int32 i = 0; i < networkThreadCount; i++)
	{
		GThreadManager->Launch([=]() {
			while (GIsRunning) { core->Dispatch(10); }
			});
	}

	for (int32 i = 0; i < logicThreadCount; i++)
	{
		GThreadManager->Launch([=]() {
			ThreadManager::DoGlobalQueueWork();
			});
	}

	// [GIGACHAD FIX] 메인 루프 (심장 박동기)
	// 이전: 3초에 한번 (Too Slow)
	// 변경: 50ms (0.05초)에 한번 -> 초당 20프레임 (서버 틱)

	// 하트비트 체크용 타이머
	uint64 lastHeartbeatTick = 0;

	while (GIsRunning)
	{
		// 1. [CRITICAL] 룸 로직 업데이트 (몬스터 AI는 여기서 돌아간다)
		// [CHANGED] 모든 룸 업데이트
		if (GRoomManager)
			GRoomManager->UpdateAll();

		// 2. CPU 휴식 (프레임 제한)
		std::this_thread::sleep_for(std::chrono::milliseconds(50));

		// 3. 서비스 하트비트 (너무 자주 할 필요 없으니 3초마다 체크)
		uint64 now = ::GetTickCount64();
		if (now - lastHeartbeatTick > 3000)
		{
			lastHeartbeatTick = now;
			//dbService->CheckHeartbeat();
			//chatService->CheckHeartbeat();
			//gameService->CheckHeartbeat();
			//loginService->CheckHeartbeat();
		}
	}

	GThreadManager->Join();

	delete GameSessionManager::GSessionManager;
	GameSessionManager::GSessionManager = nullptr;


	gameService->CloseService();
	dbService->CloseService();
	//chatService->CloseService();
	loginService->CloseService();

	return 0;
}