#include "pch.h"
#include "ThreadManager.h"
#include "Service.h"
#include "Session.h"
#include "GameSession.h" // [Inbound] GameServer 접속용
#include "DBSession.h"   // [Outbound] DB 접속용
#include "ChatServerPacketHandler.h"
#include "S2SPacketHandler.h"
#include <iostream>

int main()
{
	// 1. 핸들러 초기화
	ChatServerPacketHandler::Init(); // GameServer 요청 처리
	S2SPacketHandler::Init();        // DB 응답 처리

	// 2. [공유 심장]
	IocpCoreRef core = MakeShared<IocpCore>();

	// 3. [DB 연결] (ClientService -> DBAgent:7778)
	ClientServiceRef dbService = MakeShared<ClientService>(
		NetAddress(L"127.0.0.1", 7778),
		core,
		MakeShared<DBSession>,
		1
	);

	// 4. [GameServer 리스닝] (ServerService <- GameServer:7779)
	// ChatServer는 내부 서버이므로 포트를 7779로 쓴다.
	ServerServiceRef service = MakeShared<ServerService>(
		NetAddress(L"127.0.0.1", 7779),
		core,
		MakeShared<GameSession>, // GameServer용 세션
		100 // 서버 간 연결이니까 많이 필요 없음
	);

	// 5. 서비스 시작
	ASSERT_CRASH(dbService->Start());
	ASSERT_CRASH(service->Start());

	std::cout << "✅ [ChatServer] Running..." << std::endl;
	std::cout << "   - Listening GameServer on 7779" << std::endl;
	std::cout << "   - Connecting to DB on 7778" << std::endl;

	// 6. 스레드 런칭
	for (int32 i = 0; i < std::thread::hardware_concurrency(); i++)
	{
		GThreadManager->Launch([=]() {
			while (true) service->GetIocpCore()->Dispatch();
			});
	}

	GThreadManager->Join();
	return 0;
}