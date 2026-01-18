#include "pch.h"
#include "ThreadManager.h"
#include "Service.h"
#include "Session.h"
#include "BufferWriter.h"
#include "ClientPacketHandler.h"
#include "S2SPacketHandler.h"
#include "ClientSession.h"
#include "DBAgentSession.h"
#include <iostream>
#include <windows.h>
#include "RedisManager.h"
#include "GameServerSession.h"
#include "LoginSessionManager.h"
// [GIGACHAD] CoreGlobal은 PCH나 라이브러리 링크로 해결되지만, 
// 명시적으로 전역 변수 초기화 순서를 위해 포함.
#include "CoreGlobal.h"

enum
{
	WORKER_TICK = 64
};

// [GIGACHAD] DB 세션을 전역으로 관리 (로그인 핸들러에서 써야 하니까)
// 나중엔 SessionManager가 관리하겠지만, 지금은 단일 연결이니 전역이 편함.
shared_ptr<DBAgentSession> GDBAgentSession = nullptr;

BOOL WINAPI CtrlHandler(DWORD fdwCtrlType)
{
	switch (fdwCtrlType)
	{
	case CTRL_C_EVENT:
	case CTRL_CLOSE_EVENT:
		std::cout << " [LoginServer] Shutdown Initiated..." << std::endl;
		GIsRunning = false;
		return TRUE;
	default:
		return FALSE;
	}
}

int main()
{
	SetConsoleCtrlHandler(CtrlHandler, TRUE);

	// 1. 패킷 핸들러 초기화 (필수!)
	ClientPacketHandler::Init();
	S2SPacketHandler::Init();

	GSessionManager = new LoginSessionManager();
	LoginSessionManager::GSessionManager = GSessionManager;

	// [Redis Test] 서버 켜질 때 Redis 살아있는지 신고식
	if (GRedisManager->Set("LoginServer_Alive", "OK", 60))
	{
		std::cout << " [Redis] Connection Verified." << std::endl;
	}

	// 2. IOCP 코어 생성
	IocpCoreRef core = MakeShared<IocpCore>();

	// ============================================================
	// [Service 1] 클라이언트 접속용 (유저 받는 문)
	// ============================================================
	// Port: 7776
	// Session: ClientSession (가벼운 버전)
	// ============================================================
	// [Service 1] 유저 접속용 (Port: 7775)
	// ============================================================
	ServerServiceRef clientService = MakeShared<ServerService>(
		NetAddress(L"127.0.0.1", 7775),
		core,
		MakeShared<ClientSession>,
		1000
	);

	// ============================================================
	// [Service 2] GameServer 접속용 (Port: 7780) [NEW]
	// ============================================================
	// GameServer들이 여기에 붙어서 상태를 보고한다.
	ServerServiceRef gameServerService = MakeShared<ServerService>(
		NetAddress(L"127.0.0.1", 7780),
		core,
		MakeShared<GameServerSession>,
		10 // 게임 서버 개수는 많지 않음
	);

	// ============================================================
	// [Service 3] DBAgent 접속용 (Port: 7779)
	// ============================================================
	ClientServiceRef dbService = MakeShared<ClientService>(
		NetAddress(L"127.0.0.1", 7779),
		core,
		MakeShared<DBAgentSession>,
		1
	);
	// [중요] DB 세션 생성 콜백 낚아채기
	// 연결 성공하면 그 세션을 전역 변수(GDBAgentSession)에 저장해둔다.
	// 그래야 ClientPacketHandler에서 DB로 패킷을 보내지.
	dbService->SetConnectCallback([](SessionRef session)
		{
			// SessionRef를 DBAgentSession으로 변환 (static_pointer_cast 사용)
			GDBAgentSession = static_pointer_cast<DBAgentSession>(session);
			std::cout << "🔗 [Main] Capture DBAgent Session!" << std::endl;
		});

	// 3. 서비스 시작
	ASSERT_CRASH(clientService->Start());
	ASSERT_CRASH(dbService->Start());
	ASSERT_CRASH(gameServerService->Start());

	std::cout << " [LoginServer] Listening on 7776 & Connecting to DB..." << std::endl;

	// 4. 스레드 풀 가동
	int32 threadCount = std::thread::hardware_concurrency();
	if (threadCount < 2) threadCount = 2;

	for (int32 i = 0; i < threadCount; i++)
	{
		GThreadManager->Launch([=]() {
			while (GIsRunning)
			{
				core->Dispatch(10);
			}
			});
	}

	// 5. 메인 루프
	uint64 lastHeartbeatTick = 0;

	while (GIsRunning)
	{
		ThreadManager::DoGlobalQueueWork();
		std::this_thread::sleep_for(std::chrono::milliseconds(100));

		uint64 now = ::GetTickCount64();
		if (now - lastHeartbeatTick > 5000)
		{
			lastHeartbeatTick = now;
			// 하트비트 체크
			 //clientService->CheckHeartbeat(); 
			 //dbService->CheckHeartbeat(); 
		}
	}

	// 종료
	GThreadManager->Join();
	clientService->CloseService();
	dbService->CloseService();
	gameServerService->CloseService();
	std::cout << "👋 [LoginServer] Terminated." << std::endl;

	return 0;
}