#include "pch.h"
#include "ThreadManager.h"
#include "Service.h"
#include "Session.h"
#include "BufferWriter.h"
#include <iostream>
#include "DBConnectionPool.h"

// DBAgent는 GameServer/ChatServer의 접속을 받아주는 '서버' 역할도 해야 한다.
// 포트는 7778로 가정한다. (GameServer가 7777이라면)

int main()
{


    ASSERT_CRASH(GDBConnectionPool->Connect(
        16,
        GServerConfig.DBConnectionString.c_str()
    ));

    std::cout << "[DBAgent] DB Connection established to Azure SQL." << std::endl;

    // 3. 서버 서비스 시작 (GameServer들의 접속을 기다림)
    // S2S(Server to Server) 통신용 세션과 패킷 핸들러가 필요하지만, 
    // 일단은 컴파일/런 되는지 확인하기 위해 기본 Session 사용.
    ServerServiceRef service = MakeShared<ServerService>(
        NetAddress(L"127.0.0.1", 7778), // Port 7778
        MakeShared<IocpCore>(),
        MakeShared<Session>, // 나중에 DBSession으로 교체 예정
        100 // 최대 접속 서버 수 (많지 않음)
    );

    ASSERT_CRASH(service->Start());
    std::cout << "✅ [DBAgent] Listening on Port 7778..." << std::endl;

    // 4. 스레드 런칭 (IOCP Worker Threads)
    for (int32 i = 0; i < std::thread::hardware_concurrency(); i++)
    {
        GThreadManager->Launch([=]() {
            while (true) service->GetIocpCore()->Dispatch();
            });
    }

    // Main Thread는 멈추지 않게 대기
    GThreadManager->Join();

    return 0;
}