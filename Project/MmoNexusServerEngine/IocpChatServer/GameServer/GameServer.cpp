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

int main()
{
	// 1. 핸들러 초기화
	ClientPacketHandler::Init();
	S2SPacketHandler::Init();

	// 2. [공유 심장] 하나의 IOCP 코어로 모든 네트워크 처리
	IocpCoreRef core = MakeShared<IocpCore>();

	// 3. [DB 연결] (ClientService -> DBAgent:7778)
	ClientServiceRef dbService = MakeShared<ClientService>(
		NetAddress(L"127.0.0.1", 7778),
		core,
		MakeShared<DBSession>,
		1 // 연결 1개
	);

	// 4. [Chat 연결] (ClientService -> ChatServer:7779)
	ClientServiceRef chatService = MakeShared<ClientService>(
		NetAddress(L"127.0.0.1", 7779),
		core,
		MakeShared<ChatSession>, // GameServer -> ChatServer용 세션
		1 // 연결 1개
	);

	// 5. [Client 리스닝] (ServerService <- Clients:7777)
	ServerServiceRef gameService = MakeShared<ServerService>(
		NetAddress(L"127.0.0.1", 7777),
		core,
		MakeShared<PlayerSession>, // 유저용 세션
		1000 // 최대 동접
	);

	// 6. 서비스 시작 (순서: DB -> Chat -> Game 순으로 켜는 게 안전함)
	ASSERT_CRASH(dbService->Start());
	ASSERT_CRASH(chatService->Start());
	ASSERT_CRASH(gameService->Start());

	std::cout << "✅ [GameServer] Running..." << std::endl;
	std::cout << "   - Listening Client on 7777" << std::endl;
	std::cout << "   - Connecting to DB on 7778" << std::endl;
	std::cout << "   - Connecting to Chat on 7779" << std::endl;

	// 7. 스레드 런칭 (통합 처리)
	for (int32 i = 0; i < std::thread::hardware_concurrency(); i++)
	{
		GThreadManager->Launch([=]() {
			while (true)
			{
				core->Dispatch();
			}
			});
	}

	GThreadManager->Join();
	return 0;
}