#include "pch.h"
#include "ThreadManager.h"
#include "Service.h"
#include "Session.h"
#include "LoginSession.h"
#include "BufferWriter.h"
#include <iostream>
#include <windows.h>
#include <RedisManager.h>
// [GIGACHAD] CoreGlobal은 ServerCore.lib 안에 이미 전역으로 있으니
// 여기서는 별도로 extern 선언 안 해도 됨 (ServerCore 헤더에 포함됨).
// 하지만 명시적으로 초기화 순서가 필요하다면 CoreGlobal.h를 인클루드.
#include "CoreGlobal.h"

enum
{
	WORKER_TICK = 64
};


BOOL WINAPI CtrlHandler(DWORD fdwCtrlType)
{
	switch (fdwCtrlType)
	{
	case CTRL_C_EVENT:
	case CTRL_CLOSE_EVENT:
		std::cout << "🛑 [LoginServer] Shutdown Initiated..." << std::endl;
		GIsRunning = false;
		return TRUE;
	default:
		return FALSE;
	}
}

int main()
{
	SetConsoleCtrlHandler(CtrlHandler, TRUE);

	// 1. 패킷 핸들러 초기화 (나중에 구현)
	// LoginPacketHandler::Init();
	if (GRedisManager->Set("TestToken", "1234", 60)) // 60초 뒤 삭제
	{
		std::cout << "Redis Set Success!" << std::endl;
		std::string val = GRedisManager->Get("TestToken");
		std::cout << "Redis Get Result: " << val << std::endl;
	}
	// 2. IOCP 코어 생성
	IocpCoreRef core = MakeShared<IocpCore>();

	// 3. Login Service 생성 (Port: 7776)
	// LoginSession을 생성하는 람다 팩토리 전달
	ServerServiceRef loginService = MakeShared<ServerService>(
		NetAddress(L"127.0.0.1", 7776),
		core,
		MakeShared<LoginSession>, // Factory: 새로운 세션 필요 시 LoginSession 생성
		1000 // Max User
	);

	ASSERT_CRASH(loginService->Start());

	std::cout << "✅ [LoginServer] Listening on Port 7776..." << std::endl;

	// 4. 스레드 풀 가동
	int32 threadCount = std::thread::hardware_concurrency();
	if (threadCount < 2) threadCount = 2;

	for (int32 i = 0; i < threadCount; i++)
	{
		GThreadManager->Launch([=]() {
			while (GIsRunning)
			{
				core->Dispatch(10); // 10ms 타임아웃
			}
			});
	}

	// 5. 메인 루프 (Heartbeat 및 글로벌 작업)
	// 로그인 서버는 게임 로직(AI, 이동 등)이 없으므로 부하가 적다.
	uint64 lastHeartbeatTick = 0;

	while (GIsRunning)
	{
		// 글로벌 큐 작업 (혹시 모를 예약 작업 처리)
		ThreadManager::DoGlobalQueueWork();

		// CPU 휴식
		std::this_thread::sleep_for(std::chrono::milliseconds(100));

		// 하트비트 체크 (세션 정리 등)
		uint64 now = ::GetTickCount64();
		if (now - lastHeartbeatTick > 5000) // 5초마다
		{
			lastHeartbeatTick = now;
			loginService->CheckHeartbeat();
			// cout << "[Heartbeat] LoginServer Alive..." << endl;
		}
	}

	// 종료 처리
	GThreadManager->Join();
	loginService->CloseService();

	std::cout << "👋 [LoginServer] Terminated." << std::endl;

	return 0;
}