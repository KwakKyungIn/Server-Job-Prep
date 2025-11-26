#include "pch.h"
#include "ThreadManager.h"
#include "Service.h"
#include "Session.h"
#include "BufferWriter.h"
#include <iostream>
#include "DBConnectionPool.h"
#include "DBAgentPacketHandler.h"
#include "GameSession.h" // [Default] 게임서버 접속용 세션

// [정의] GDBConnectionPool의 실체
DBConnectionPool* GDBConnectionPool = nullptr;

int main()
{
	// 1. 핸들러 초기화
	DBAgentPacketHandler::Init();

	// 2. DBConnectionPool 생성 및 연결
	GDBConnectionPool = new DBConnectionPool();
	ASSERT_CRASH(GDBConnectionPool->Connect(
		16,
		GServerConfig.DBConnectionString.c_str()
	));

	std::cout << "[DBAgent] DB Connection established to Azure SQL." << std::endl;

	// 3. 서버 서비스 시작 (Port 7778)
	// GameServer와 ChatServer 모두 여기로 접속한다.
	// 일단 GameSession으로 통일해서 받는다.
	ServerServiceRef service = MakeShared<ServerService>(
		NetAddress(L"127.0.0.1", 7778),
		MakeShared<IocpCore>(),
		MakeShared<GameSession>, // 들어오는 놈들은 다 GameSession 취급
		100
	);

	ASSERT_CRASH(service->Start());
	std::cout << "✅ [DBAgent] Listening on Port 7778..." << std::endl;

	// 4. 스레드 런칭
	for (int32 i = 0; i < std::thread::hardware_concurrency(); i++)
	{
		GThreadManager->Launch([=]() {
			while (true) service->GetIocpCore()->Dispatch();
			});
	}

	GThreadManager->Join();
	delete GDBConnectionPool;
	return 0;
}