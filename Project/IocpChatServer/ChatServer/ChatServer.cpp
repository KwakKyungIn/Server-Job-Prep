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

enum
{
	WORKER_TICK = 64
};



int main()
{

	ASSERT_CRASH(GDBConnectionPool->Connect(1, L"Driver={ODBC Driver 18 for SQL Server};Server=localhost;Database=test2;Trusted_Connection=Yes;Encrypt=Yes;TrustServerCertificate=Yes;"));


	// Create Table
	{
		auto query = L"									\
			DROP TABLE IF EXISTS [dbo].[Gold];			\
			CREATE TABLE [dbo].[Gold]					\
			(											\
				[id] INT NOT NULL PRIMARY KEY IDENTITY, \
				[gold] INT NULL							\
			);";

		DBConnection* dbConn = GDBConnectionPool->Pop();
		ASSERT_CRASH(dbConn->Execute(query));
		GDBConnectionPool->Push(dbConn);
	}

	// Add Data
	for (int32 i = 0; i < 3; i++)
	{
		DBConnection* dbConn = GDBConnectionPool->Pop();
		// 기존에 바인딩 된 정보 날림
		dbConn->Unbind();

		// 넘길 인자 바인딩
		int32 gold = 100;
		SQLLEN len = 0;

		// 넘길 인자 바인딩
		ASSERT_CRASH(dbConn->BindParam(1, SQL_C_LONG, SQL_INTEGER, sizeof(gold), &gold, &len));

		// SQL 실행
		ASSERT_CRASH(dbConn->Execute(L"INSERT INTO [dbo].[Gold]([gold]) VALUES(?)"));

		GDBConnectionPool->Push(dbConn);
	}

	// Read
	{
		DBConnection* dbConn = GDBConnectionPool->Pop();
		// 기존에 바인딩 된 정보 날림
		dbConn->Unbind();

		int32 gold = 100;
		SQLLEN len = 0;
		// 넘길 인자 바인딩
		ASSERT_CRASH(dbConn->BindParam(1, SQL_C_LONG, SQL_INTEGER, sizeof(gold), &gold, &len));

		int32 outId = 0;
		SQLLEN outIdLen = 0;
		ASSERT_CRASH(dbConn->BindCol(1, SQL_C_LONG, sizeof(outId), &outId, &outIdLen));

		int32 outGold = 0;
		SQLLEN outGoldLen = 0;
		ASSERT_CRASH(dbConn->BindCol(2, SQL_C_LONG, sizeof(outGold), &outGold, &outGoldLen));

		// SQL 실행
		ASSERT_CRASH(dbConn->Execute(L"SELECT id, gold FROM [dbo].[Gold] WHERE gold = (?)"));

		while (dbConn->Fetch())
		{
			cout << "Id: " << outId << " Gold : " << outGold << endl;
		}

		GDBConnectionPool->Push(dbConn);
	}


	ASSERT_CRASH(service->Start());


	GThreadManager->Join();
}