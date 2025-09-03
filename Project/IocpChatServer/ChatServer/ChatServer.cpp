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
#include <iostream>

int main()
{
    ASSERT_CRASH(GDBConnectionPool->Connect(1, L"Driver={ODBC Driver 18 for SQL Server};Server=localhost;Database=test2;Trusted_Connection=Yes;Encrypt=Yes;TrustServerCertificate=Yes;"));

    // 1. Create a dummy Players table for the login test.
    std::cout << "--- Creating Players Table and adding a test user ---" << std::endl;
    DBConnection* dbConn = GDBConnectionPool->Pop();
    ASSERT_CRASH(dbConn);
    auto createTableQuery = L"DROP TABLE IF EXISTS [dbo].[Players];"
        L"CREATE TABLE [dbo].[Players] ("
        L"[playerId] BIGINT NOT NULL PRIMARY KEY IDENTITY,"
        L"[name] NVARCHAR(50) NULL);";
    ASSERT_CRASH(dbConn->Execute(createTableQuery));

    auto insertUserQuery = L"INSERT INTO [dbo].[Players] ([name]) VALUES('TestUser')";
    ASSERT_CRASH(dbConn->Execute(insertUserQuery));
    GDBConnectionPool->Push(dbConn);
    std::cout << "Table created and 'TestUser' inserted." << std::endl;

    // 2. Start the server service to listen for client connections.

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
