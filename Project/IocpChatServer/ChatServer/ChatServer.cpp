#include "pch.h"
#include "ThreadManager.h"
#include "Service.h"
#include "Session.h"
#include "ChatSession.h"
#include "ChatSessionManager.h"
#include "BufferWriter.h"
#include "ClientPacketHandler.h"
#include <tchar.h>
#include "Protocol.pb.h"
#include "Room.h"
#include "Player.h"
#include "DBConnectionPool.h"
#include "RoomManager.h" // 방 관리를 위해 추가
#include <iostream>

int main()
{
	ASSERT_CRASH(GDBConnectionPool->Connect(1, L"Driver={ODBC Driver 18 for SQL Server};Server=localhost;Database=test2;Trusted_Connection=Yes;Encrypt=Yes;TrustServerCertificate=Yes;"));

	// 1. 로그인 테스트를 위해 더미 Players 테이블을 생성합니다.
	std::cout << "--- Creating Players Table and adding a test user ---" << std::endl;
	DBConnection* dbConn = GDBConnectionPool->Pop();
	ASSERT_CRASH(dbConn);
	auto createTableQuery = L"DROP TABLE IF EXISTS [dbo].[Players];"
		L"CREATE TABLE [dbo].[Players] ("
		L"[playerId] BIGINT NOT NULL PRIMARY KEY IDENTITY,"
		L"[name] NVARCHAR(50) NULL);";
	ASSERT_CRASH(dbConn->Execute(createTableQuery));

	// [수정] Execute 함수를 각각의 INSERT 문에 대해 호출합니다.
	ASSERT_CRASH(dbConn->Execute(L"INSERT INTO [dbo].[Players] ([name]) VALUES('TestUser')"));
	ASSERT_CRASH(dbConn->Execute(L"INSERT INTO [dbo].[Players] ([name]) VALUES('TestUser2')"));
	ASSERT_CRASH(dbConn->Execute(L"INSERT INTO [dbo].[Players] ([name]) VALUES('TestUser3')"));
	ASSERT_CRASH(dbConn->Execute(L"INSERT INTO [dbo].[Players] ([name]) VALUES('TestUser4')"));
	
	GDBConnectionPool->Push(dbConn);
	std::cout << "Table created and 'TestUser' inserted." << std::endl;

	// 2. 클라이언트 연결을 수신하기 위해 서버 서비스를 시작합니다.

	ClientPacketHandler::Init();

	ServerServiceRef service = MakeShared<ServerService>(
		NetAddress(L"127.0.0.1", 7777),
		MakeShared<IocpCore>(),
		MakeShared<ChatSession>,
		100);

	ASSERT_CRASH(service->Start());

	for (int32 i = 0; i < 2; i++)
	{
		GThreadManager->Launch([=]()
			{
				while (true)
				{
					service->GetIocpCore()->Dispatch();
				}
			});
	}

	GThreadManager->Join();

	return 0;
}
