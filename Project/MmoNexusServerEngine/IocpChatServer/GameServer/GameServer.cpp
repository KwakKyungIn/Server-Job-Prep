#include "pch.h"
#include "ThreadManager.h"
#include "Service.h"
#include "Session.h"
#include "PlayerSession.h"      // [Inbound] 유저 접속용
#include "BufferWriter.h"
#include "ClientPacketHandler.h"
#include "S2SPacketHandler.h" // DB/Chat 응답용
#include "DBSession.h"        // [Outbound] DB 접속용
#include "ChatSession.h"      // [Outbound] Chat 접속용
#include <iostream>
#include <windows.h> // [ADD] 종료 핸들러용


// [Ctrl+C 핸들러]
BOOL WINAPI CtrlHandler(DWORD fdwCtrlType)
{
	switch (fdwCtrlType)
	{
	case CTRL_C_EVENT:
	case CTRL_CLOSE_EVENT:
		std::cout << "🛑 [System] Server Shutdown Initiated..." << std::endl;
		// GIsRunning 플래그를 내려서 스레드들이 루프를 탈출하게 만듦
		extern std::atomic<bool> GIsRunning;
		GIsRunning = false;
		return TRUE;
	default:
		return FALSE;
	}
}

int main()
{
	// 1. 종료 핸들러 등록
	SetConsoleCtrlHandler(CtrlHandler, TRUE);

	// 2. 핸들러 초기화
	ClientPacketHandler::Init();
	S2SPacketHandler::Init();

	// 3. [공유 심장] IOCP 코어
	IocpCoreRef core = MakeShared<IocpCore>();

	// 4. [DB 연결] (ClientService -> DBAgent:7778)
	ClientServiceRef dbService = MakeShared<ClientService>(
		NetAddress(L"127.0.0.1", 7778),
		core,
		MakeShared<DBSession>,
		1
	);

	// 5. [Chat 연결] (ClientService -> ChatServer:7779)
	ClientServiceRef chatService = MakeShared<ClientService>(
		NetAddress(L"127.0.0.1", 7779),
		core,
		MakeShared<ChatSession>,
		1
	);

	// 6. [Client 리스닝] (ServerService <- Clients:7777)
	ServerServiceRef gameService = MakeShared<ServerService>(
		NetAddress(L"127.0.0.1", 7777),
		core,
		MakeShared<PlayerSession>,
		1000
	);

	// 7. 서비스 시작
	ASSERT_CRASH(dbService->Start());
	ASSERT_CRASH(chatService->Start());
	ASSERT_CRASH(gameService->Start());

	std::cout << "✅ [GameServer] Running... (Press Ctrl+C to quit)" << std::endl;
	std::cout << "   - Listening Client on 7777" << std::endl;
	std::cout << "   - Connecting to DB on 7778" << std::endl;
	std::cout << "   - Connecting to Chat on 7779" << std::endl;

	// 8. 스레드 런칭 (Hybrid Mode)
	// [수정됨] IO 담당 스레드와 로직 담당 스레드를 분리한다.

	int32 threadCount = std::thread::hardware_concurrency();
	if (threadCount < 2) threadCount = 2; // 최소 2개는 보장

	// 전략: 절반은 네트워크(IO), 절반은 로직(Job)
	// (MMO 특성상 로직 스레드가 더 필요할 수 있지만, 일단 5:5로 시작)
	int32 networkThreadCount = threadCount / 2;
	int32 logicThreadCount = threadCount - networkThreadCount;

	std::cout << "🚀 [System] Worker Threads -> Network(IOCP): " << networkThreadCount
		<< " | Logic(Job): " << logicThreadCount << std::endl;

	// 8-1. 네트워크 스레드 (IOCP Dispatch Only)
	for (int32 i = 0; i < networkThreadCount; i++)
	{
		GThreadManager->Launch([=]() {
			while (GIsRunning)
			{
				// IO 처리 (기존 역할)
				core->Dispatch(10);
			}
			});
	}

	// 8-2. 로직 스레드 (JobQueue Execute Only)
	for (int32 i = 0; i < logicThreadCount; i++)
	{
		GThreadManager->Launch([=]() {
			// 로직 처리 (새로운 역할)
			ThreadManager::DoGlobalQueueWork();
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
		chatService->CheckHeartbeat(); // 끊기면 재접속 시도!
		gameService->CheckHeartbeat(); // 유저들 타임아웃 체크!
	}

	// 9. 메인 스레드 대기
	GThreadManager->Join();

	// 10. [Graceful Shutdown] 정리 작업
	std::cout << "🛑 [System] Cleaning up resources..." << std::endl;

	gameService->CloseService();
	dbService->CloseService();
	chatService->CloseService();

	std::cout << "👋 [System] Bye!" << std::endl;
	return 0;
}