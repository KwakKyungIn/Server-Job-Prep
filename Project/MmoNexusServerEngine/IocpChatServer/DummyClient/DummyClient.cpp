#include "pch.h"
#include "ThreadManager.h"
#include "Service.h"
#include "Session.h"
#include "BufferReader.h"
#include "ServerPacketHandler.h"
#include "ServerSession.h" 
#include "Protocol.pb.h"
#include <iostream>
#include <string> 
#include <atomic>
#include <limits> 
#include <ios>
#include <windows.h> // [ADD]

// 전역 플래그
std::atomic<bool> g_isLoggedIn = false;
// [ADD] 프로그램 실행 플래그 (Ctrl+C 대응)
std::atomic<bool> g_isRunning = true;

// 연결된 세션
PacketSessionRef g_session = nullptr;

// [ADD] 종료 핸들러
BOOL WINAPI CtrlHandler(DWORD fdwCtrlType)
{
	switch (fdwCtrlType)
	{
	case CTRL_C_EVENT:
	case CTRL_CLOSE_EVENT:
	case CTRL_LOGOFF_EVENT:
	case CTRL_SHUTDOWN_EVENT:
		std::cout << "\n🛑 [Client] Shutdown Initiated..." << std::endl;
		g_isRunning = false; // 루프 탈출 신호
		return TRUE;
	default:
		return FALSE;
	}
}

int main()
{
	// 1. 핸들러 등록
	SetConsoleCtrlHandler(CtrlHandler, TRUE);
	ServerPacketHandler::Init();

	// 2. 서비스 시작
	std::this_thread::sleep_for(std::chrono::seconds(1));

	ClientServiceRef service = MakeShared<ClientService>(
		NetAddress(L"127.0.0.1", 7777),
		MakeShared<IocpCore>(),
		MakeShared<ServerSession>,
		1
	);

	ASSERT_CRASH(service->Start());

	// 3. 네트워크 스레드
	GThreadManager->Launch([=]()
		{
			// [CHANGE] g_isRunning 체크
			while (g_isRunning)
			{
				service->GetIocpCore()->Dispatch(10);
			}
		});

	// =============================================================
	// [메인 스레드] 입력 및 흐름 제어
	// =============================================================

	// [Phase 1] 접속 대기
	std::cout << ">> [System] 서버 접속 대기 중...";
	while (g_session == nullptr && g_isRunning)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}

	if (g_isRunning)
		std::cout << " 완료!" << std::endl;

	// [Phase 2] 로그인
	while (g_isLoggedIn == false && g_isRunning)
	{
		if (g_session == nullptr) break;

		std::cout << "\n>> 로그인할 이름을 입력하세요: ";

		// [Non-blocking Input Trick]
		// C++ 표준 cin은 블로킹 함수라 g_isRunning이 false가 되어도 
		// 엔터를 칠 때까지 멈춰있다. 
		// 완벽하게 하려면 비동기 입력을 써야 하지만, Dummy니까 일단 cin 씀.
		// (Ctrl+C 누르면 바로 종료되게 하려면 CtrlHandler에서 exit(0) 때리는 게 가장 빠르긴 함)

		std::string playerName;
		std::cin >> playerName;

		if (!g_isRunning) break; // 입력 중 종료 신호

		std::cin.ignore((std::numeric_limits<std::streamsize>::max)(), '\n');

		Protocol::C_LOGIN_REQ pkt;
		pkt.set_name(playerName);
		auto sendBuffer = ServerPacketHandler::MakeSendBuffer(pkt);
		g_session->Send(sendBuffer);

		std::cout << ">> 로그인 요청 전송. 응답 대기 중..." << std::endl;

		for (int i = 0; i < 20; i++)
		{
			if (g_isLoggedIn || !g_isRunning) break;
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}
	}

	// [Phase 3] 채팅
	if (g_isRunning)
	{
		std::cout << "\n==================================================" << std::endl;
		std::cout << "       🔥 WELCOME TO NEXUS CHAT 🔥" << std::endl;
		std::cout << "==================================================" << std::endl;
	}

	while (g_isRunning)
	{
		if (g_session == nullptr) break;

		std::string message;
		std::getline(std::cin, message);

		if (!g_isRunning) break;
		if (message.empty()) continue;

		if (message == "/quit" || message == "/exit")
		{
			g_isRunning = false;
			break;
		}

		Protocol::C_CHAT_REQ reqPkt;
		reqPkt.set_message(message);
		auto sendBuffer = ServerPacketHandler::MakeSendBuffer(reqPkt);
		g_session->Send(sendBuffer);
	}

	// [Graceful Exit]
	std::cout << "🛑 [Client] Closing connection..." << std::endl;

	if (g_session)
	{
		// 서버에게 "나 간다"라고 명시적으로 끊기 (TCP FIN)
		g_session->Disconnect(L"Client Quit");
	}

	service->CloseService(); // 서비스 정리

	GThreadManager->Join();
	std::cout << "👋 [Client] Bye!" << std::endl;
	return 0;
}