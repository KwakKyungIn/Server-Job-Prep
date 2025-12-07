#include "pch.h"
#include "ThreadManager.h"
#include "Service.h"
#include "Session.h"
#include "BufferWriter.h"
#include <iostream>
#include "DBConnectionPool.h"
#include "DBAgentPacketHandler.h"
#include "GameSession.h" 
#include <windows.h> // [ADD]

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
		std::cout << "🛑 [DBAgent] Shutdown Initiated..." << std::endl;
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
	GDBConnectionPool = new DBConnectionPool();
	ASSERT_CRASH(GDBConnectionPool->Connect(
		16,
		GServerConfig.DBConnectionString.c_str()
	));

	std::cout << "[DBAgent] DB Connection established to Azure SQL." << std::endl;

	// 3. 서버 서비스 시작
	ServerServiceRef service = MakeShared<ServerService>(
		NetAddress(L"127.0.0.1", 7778),
		MakeShared<IocpCore>(),
		MakeShared<GameSession>, // 잡큐가 장착된 세션
		100
	);

	ASSERT_CRASH(service->Start());
	std::cout << "✅ [DBAgent] Listening on Port 7778... (Press Ctrl+C to quit)" << std::endl;

	// 4. 스레드 런칭 (Hybrid Architecture)
	// [CRITICAL Change] IOCP만 돌리면 안 되고, JobQueue도 돌려야 한다!
	int32 threadCount = std::thread::hardware_concurrency();
	if (threadCount < 2) threadCount = 2;

	// 전략: 반반 나눈다. 
	// (DB 쿼리가 느리기 때문에 로직 스레드가 넉넉해야 병목이 안 생긴다)
	int32 networkThreadCount = threadCount / 2;
	int32 logicThreadCount = threadCount - networkThreadCount;

	std::cout << "🚀 [System] Worker Threads -> Network: " << networkThreadCount
		<< " | Logic(DB): " << logicThreadCount << std::endl;

	// 4-1. 네트워크 스레드 (패킷 수신 -> Job 생성)
	for (int32 i = 0; i < networkThreadCount; i++)
	{
		GThreadManager->Launch([=]() {
			while (GIsRunning)
			{
				service->GetIocpCore()->Dispatch(10);
			}
			});
	}

	// 4-2. 로직(DB) 스레드 (Job 꺼내서 SQL 실행)
	// 이 친구들이 없으면 DB 처리가 영원히 안 된다.
	for (int32 i = 0; i < logicThreadCount; i++)
	{
		GThreadManager->Launch([=]() {
			ThreadManager::DoGlobalQueueWork();
			});
	}

	// [CHANGE] 메인 스레드: 주기적 Heartbeat 체크
	while (GIsRunning)
	{
		// 1초에 한 번씩 검사
		std::this_thread::sleep_for(std::chrono::seconds(3));

		// 서비스들의 상태 체크
		service->CheckHeartbeat();
	}

	// 5. 대기 및 종료
	GThreadManager->Join();

	std::cout << "🛑 [DBAgent] Cleaning up resources..." << std::endl;

	service->CloseService(); // 리스너/세션 정리
	delete GDBConnectionPool; // DB 연결 정리

	std::cout << "👋 [DBAgent] Bye!" << std::endl;
	return 0;
}