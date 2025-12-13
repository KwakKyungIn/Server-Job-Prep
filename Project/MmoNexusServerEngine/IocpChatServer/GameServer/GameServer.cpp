#include "pch.h"
#include "ThreadManager.h"
#include "Service.h"
#include "Session.h"
#include "PlayerSession.h"
#include "BufferWriter.h"
#include "ClientPacketHandler.h"
#include "S2SPacketHandler.h"
#include "DBSession.h"
#include "ChatSession.h"
#include "GameRoom.h" // [GIGACHAD FIX] 이거 없으면 GameRoom 모름
#include <iostream>
#include <windows.h>
#include "LoginSession.h"
#include "RoomManager.h" // [NEW]

// [External Reference] ClientPacketHandler.cpp에 있는 그 놈 가져오기
//extern shared_ptr<GameRoom> GTestRoom;
extern shared_ptr<LoginSession> G_LoginSession;
extern std::atomic<bool> GIsRunning; // 전역 변수 참조

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

int main()
{
	SetConsoleCtrlHandler(CtrlHandler, TRUE);

	ClientPacketHandler::Init();
	S2SPacketHandler::Init();

	IocpCoreRef core = MakeShared<IocpCore>();

	ClientServiceRef dbService = MakeShared<ClientService>(
		NetAddress(L"127.0.0.1", 7778),
		core,
		MakeShared<DBSession>,
		1
	);

	ClientServiceRef chatService = MakeShared<ClientService>(
		NetAddress(L"127.0.0.1", 7779),
		core,
		MakeShared<ChatSession>,
		1
	);

	ServerServiceRef gameService = MakeShared<ServerService>(
		NetAddress(L"127.0.0.1", 7777),
		core,
		MakeShared<PlayerSession>,
		1000
	);

	ClientServiceRef loginService = MakeShared<ClientService>(
		NetAddress(L"127.0.0.1", 7776), // LoginServer의 내부 통신 포트
		core,
		MakeShared<LoginSession>,
		1
	);

	ASSERT_CRASH(dbService->Start());
	ASSERT_CRASH(chatService->Start());
	ASSERT_CRASH(loginService->Start());
	ASSERT_CRASH(gameService->Start());

	GRoomManager = MakeShared<RoomManager>();

	std::cout << "✅ [GameServer] Running... (Press Ctrl+C to quit)" << std::endl;

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
			dbService->CheckHeartbeat();
			chatService->CheckHeartbeat();
			gameService->CheckHeartbeat();
			loginService->CheckHeartbeat();
		}
	}

	GThreadManager->Join();

	gameService->CloseService();
	dbService->CloseService();
	chatService->CloseService();
	loginService->CloseService();

	return 0;
}