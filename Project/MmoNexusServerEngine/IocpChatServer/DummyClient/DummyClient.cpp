#include "pch.h"
#include "ThreadManager.h"
#include "Service.h"
#include "Session.h"
#include "BufferReader.h"
#include "ServerPacketHandler.h"
#include "ServerSession.h" // [필수] 네가 만든 세션 헤더
#include "Protocol.pb.h"
#include <iostream>
#include <string> 
#include <atomic>
#include <limits> 
#include <ios>       

// 전역 플래그 (extern으로 핸들러에서 씀)
std::atomic<bool> g_isLoggedIn = false;

// 연결된 세션 (핸들러나 세션에서 설정해줘야 함)
PacketSessionRef g_session = nullptr;

int main()
{
	// 1. 패킷 핸들러 초기화
	ServerPacketHandler::Init();

	// 2. 서비스 시작 (GameServer: 7777로 접속)
	std::this_thread::sleep_for(std::chrono::seconds(1)); // 서버 켜질 시간 조금 줌

	ClientServiceRef service = MakeShared<ClientService>(
		NetAddress(L"127.0.0.1", 7777),
		MakeShared<IocpCore>(),
		MakeShared<ServerSession>, // 세션 팩토리
		1
	);

	ASSERT_CRASH(service->Start());

	// 3. 네트워크 I/O 스레드 실행
	GThreadManager->Launch([=]()
		{
			while (true)
			{
				service->GetIocpCore()->Dispatch();
				std::this_thread::sleep_for(std::chrono::milliseconds(10));
			}
		});

	// =============================================================
	// [메인 스레드] 입력 및 흐름 제어
	// =============================================================

	// [Phase 1] 접속 대기
	std::cout << ">> [System] 서버 접속 대기 중...";
	while (g_session == nullptr)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}
	std::cout << " 완료!" << std::endl;

	// [Phase 2] 로그인 (로그인 성공할 때까지 반복)
	while (g_isLoggedIn == false)
	{
		if (g_session == nullptr) break;

		std::cout << "\n>> 로그인할 이름을 입력하세요: ";
		std::string playerName;
		std::cin >> playerName;

		// 버퍼 비우기 (엔터 키 잔재 제거)
		std::cin.ignore((std::numeric_limits<std::streamsize>::max)(), '\n');

		// 패킷 전송
		Protocol::C_LOGIN_REQ pkt;
		pkt.set_name(playerName);
		auto sendBuffer = ServerPacketHandler::MakeSendBuffer(pkt);
		g_session->Send(sendBuffer);

		std::cout << ">> 로그인 요청 전송. 응답 대기 중..." << std::endl;

		// 응답 대기 (최대 2초)
		for (int i = 0; i < 20; i++)
		{
			if (g_isLoggedIn) break;
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}

		if (g_isLoggedIn == false)
			std::cout << ">> 응답 없음. 다시 시도하세요." << std::endl;
	}

	// [Phase 3] 채팅 (무한 루프)
	std::cout << "\n==================================================" << std::endl;
	std::cout << "       🔥 WELCOME TO NEXUS CHAT 🔥" << std::endl;
	std::cout << "==================================================" << std::endl;

	while (true)
	{
		if (g_session == nullptr) break;

		// 채팅 입력
		std::string message;
		std::getline(std::cin, message);

		if (message.empty()) continue;

		// 종료 커맨드
		if (message == "/quit" || message == "/exit")
		{
			break;
		}

		// 일반 채팅 전송
		Protocol::C_CHAT_REQ reqPkt;
		reqPkt.set_message(message);
		auto sendBuffer = ServerPacketHandler::MakeSendBuffer(reqPkt);
		g_session->Send(sendBuffer);
	}

	std::cout << ">> 클라이언트를 종료합니다." << std::endl;
	GThreadManager->Join();
	return 0;
}