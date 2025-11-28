#include "pch.h"
#include "ThreadManager.h"
#include "Service.h"
#include "Session.h"
#include "GameSession.h" 
#include "DBSession.h"   
#include "ChatServerPacketHandler.h"
#include "S2SPacketHandler.h"
#include <iostream>
#include <windows.h> // [ADD]

// [ADD] Ctrl Handler
BOOL WINAPI CtrlHandler(DWORD fdwCtrlType)
{
	switch (fdwCtrlType)
	{
	case CTRL_C_EVENT:
	case CTRL_CLOSE_EVENT:
	case CTRL_LOGOFF_EVENT:
	case CTRL_SHUTDOWN_EVENT:
		std::cout << "🛑 [ChatServer] Shutdown Initiated..." << std::endl;
		extern std::atomic<bool> GIsRunning;
		GIsRunning = false;
		return TRUE;
	default:
		return FALSE;
	}
}

int main()
{
	SetConsoleCtrlHandler(CtrlHandler, TRUE);

	// 1. 핸들러 초기화
	ChatServerPacketHandler::Init();
	S2SPacketHandler::Init();

	// 2. [공유 심장]
	IocpCoreRef core = MakeShared<IocpCore>();

	// 3. [DB 연결] 
	ClientServiceRef dbService = MakeShared<ClientService>(
		NetAddress(L"127.0.0.1", 7778),
		core,
		MakeShared<DBSession>,
		1
	);

	// 4. [GameServer 리스닝] 
	ServerServiceRef gameService = MakeShared<ServerService>(
		NetAddress(L"127.0.0.1", 7779),
		core,
		MakeShared<GameSession>,
		100
	);

	// 5. 서비스 시작
	ASSERT_CRASH(dbService->Start());
	ASSERT_CRASH(gameService->Start());

	std::cout << "✅ [ChatServer] Running... (Press Ctrl+C to quit)" << std::endl;

	// 6. 스레드 런칭
	for (int32 i = 0; i < std::thread::hardware_concurrency(); i++)
	{
		GThreadManager->Launch([=]() {
			while (GIsRunning)
			{
				gameService->GetIocpCore()->Dispatch(10);
			}
			});
	}

	// [CHANGE] 메인 스레드는 이제 그냥 대기만 하는 게 아니라,
	// 주기적으로 Heartbeat 검사를 한다.

	while (GIsRunning)
	{
		// 1초에 한 번씩 검사 (너무 자주 할 필요 없음)
		std::this_thread::sleep_for(std::chrono::seconds(3));

		// 서비스들의 상태 체크
		dbService->CheckHeartbeat();   // 끊기면 재접속 시도!
		gameService->CheckHeartbeat(); // 유저들 타임아웃 체크!
	}

	GThreadManager->Join();

	std::cout << "🛑 [ChatServer] Cleaning up resources..." << std::endl;
	dbService->CloseService();
	gameService->CloseService();

	std::cout << "👋 [ChatServer] Bye!" << std::endl;
	return 0;
}