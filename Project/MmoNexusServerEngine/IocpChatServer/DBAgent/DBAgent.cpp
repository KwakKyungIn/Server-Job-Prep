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
		MakeShared<GameSession>,
		100
	);

	ASSERT_CRASH(service->Start());
	std::cout << "✅ [DBAgent] Listening on Port 7778... (Press Ctrl+C to quit)" << std::endl;

	// 4. 스레드 런칭
	for (int32 i = 0; i < std::thread::hardware_concurrency(); i++)
	{
		GThreadManager->Launch([=]() {
			// [CHANGE] GIsRunning 체크
			while (GIsRunning)
			{
				service->GetIocpCore()->Dispatch(10);
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
		service->CheckHeartbeat();   // 끊기면 재접속 시도!
	}

	// 5. 대기 및 종료
	GThreadManager->Join();

	std::cout << "🛑 [DBAgent] Cleaning up resources..." << std::endl;

	service->CloseService(); // 리스너/세션 정리
	delete GDBConnectionPool; // DB 연결 정리

	std::cout << "👋 [DBAgent] Bye!" << std::endl;
	return 0;
}