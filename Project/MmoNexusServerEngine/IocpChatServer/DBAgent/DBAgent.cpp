#include "pch.h"
#include "ThreadManager.h"
#include "Service.h"
#include "Session.h"
#include "BufferWriter.h"
#include <iostream>
#include "DBConnectionPool.h"
#include "DBAgentPacketHandler.h"
#include "GameSession.h" 
#include "ChatSession.h"
#include "LoginSession.h" // [ADD] 아까 만든 헤더 포함
#include <windows.h>

// [정의] GDBConnectionPool의 실체
DBConnectionPool* GDBConnectionPool = nullptr;

// [ADD] Ctrl Handler
BOOL WINAPI CtrlHandler(DWORD fdwCtrlType)
{
	switch (fdwCtrlType)
	{
	case CTRL_C_EVENT:
	case CTRL_CLOSE_EVENT:
	case CTRL_LOGOFF_EVENT:
	case CTRL_SHUTDOWN_EVENT:
		std::cout << " [DBAgent] Shutdown Initiated..." << std::endl;
		extern std::atomic<bool> GIsRunning;
		GIsRunning = false;
		return TRUE;
	default:
		return FALSE;
	}
}

int main()
{
	// [ADD] 핸들러 등록
	SetConsoleCtrlHandler(CtrlHandler, TRUE);

	// 1. 핸들러 초기화
	DBAgentPacketHandler::Init();

	// 2. DBConnectionPool 생성 및 연결
	GDBConnectionPool = xnew<DBConnectionPool>();
	ASSERT_CRASH(GDBConnectionPool->Connect(
		16,
		GServerConfig.DBConnectionString.c_str()
	));

	std::cout << "[DBAgent] DB Connection established to Azure SQL." << std::endl;

	// [CHANGE] IOCP Core를 공유하기 위해 미리 생성
	IocpCoreRef mainIocpCore = MakeShared<IocpCore>();

	// 3-1. GameServer용 서비스 (Port: 7778)
	ServerServiceRef gameService = MakeShared<ServerService>(
		NetAddress(L"127.0.0.1", 7778),
		mainIocpCore, // Core 공유
		MakeShared<GameSession>,
		100
	);

	// 3-2. [ADD] LoginServer용 서비스 (Port: 7779)
	ServerServiceRef loginService = MakeShared<ServerService>(
		NetAddress(L"127.0.0.1", 7779), // 포트 분리!
		mainIocpCore, // Core 공유
		MakeShared<LoginSession>, // LoginSession 생성
		10 // 로그인 서버는 소수니까 적게 잡아도 됨
	);

	//ServerServiceRef chatService = MakeShared<ServerService>(
	//	NetAddress(L"127.0.0.1", 7781), // 포트 분리!
	//	mainIocpCore, // Core 공유
	//	MakeShared<ChatSession>, 
	//	10 
	//);

	ASSERT_CRASH(gameService->Start());
	ASSERT_CRASH(loginService->Start());
	//ASSERT_CRASH(chatService->Start());

	std::cout << " [DBAgent] Listening on Port 7778(Game) & 7779(Login)..." << std::endl;

	// 4. 스레드 런칭 (Hybrid Architecture)
	int32 threadCount = std::thread::hardware_concurrency();
	if (threadCount < 2) threadCount = 2;

	int32 networkThreadCount = threadCount / 2;
	int32 logicThreadCount = threadCount - networkThreadCount;

	std::cout << " [System] Worker Threads -> Network: " << networkThreadCount
		<< " | Logic(DB): " << logicThreadCount << std::endl;

	// 4-1. 네트워크 스레드
	for (int32 i = 0; i < networkThreadCount; i++)
	{
		GThreadManager->Launch([=]() {
			while (GIsRunning)
			{
				// [Check] Core를 공유했으므로 mainIocpCore만 돌리면
				// gameService, loginService 둘 다 처리된다.
				mainIocpCore->Dispatch(10);
			}
			});
	}

	// 4-2. 로직(DB) 스레드
	for (int32 i = 0; i < logicThreadCount; i++)
	{
		GThreadManager->Launch([=]() {
			ThreadManager::DoGlobalQueueWork();
			});
	}

	// [CHANGE] 메인 스레드: 주기적 Heartbeat 체크
	while (GIsRunning)
	{
		std::this_thread::sleep_for(std::chrono::seconds(3));

		// 두 서비스 모두 체크
		gameService->CheckHeartbeat();
		loginService->CheckHeartbeat();
	}

	// 5. 대기 및 종료
	GThreadManager->Join();

	std::cout << " [DBAgent] Cleaning up resources..." << std::endl;

	gameService->CloseService();
	loginService->CloseService(); // [ADD]
	//chatService->CloseService();
	xdelete(GDBConnectionPool);

		std::cout << "[DBAgent] Bye!" << std::endl;
	return 0;
}
