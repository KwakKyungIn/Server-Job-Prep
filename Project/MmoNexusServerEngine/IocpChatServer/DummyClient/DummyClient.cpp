#include "pch.h"
#include "ThreadManager.h"
#include "Service.h"
#include "Session.h"
#include "BufferReader.h"
#include "ServerPacketHandler.h"
#include "Protocol.pb.h"
#include <iostream>
#include <string> 
#include <atomic>
#include <limits> 
#include <ios>      

// 전역 플래그 정의
std::atomic<bool> g_isLoggedIn = false;
std::atomic<bool> g_isInRoom = false;

// 연결된 세션
PacketSessionRef g_session = nullptr;

class ServerSession : public PacketSession
{
public:
	~ServerSession()
	{
		std::cout << "~ServerSession" << std::endl;
	}

	virtual void OnConnected() override
	{
		// [수정] 여기선 입력받지 않음! 연결됐다는 깃발만 꽂음.
		g_session = GetPacketSessionRef();
		std::cout << ">> [System] 서버 연결 성공!" << std::endl;
	}

	virtual void OnRecvPacket(BYTE* buffer, int32 len) override
	{
		PacketSessionRef session = GetPacketSessionRef();
		PacketHeader* header = reinterpret_cast<PacketHeader*>(buffer);

		// 패킷 처리는 핸들러에게 위임
		ServerPacketHandler::HandlePacket(session, buffer, len);
	}

	virtual void OnSend(int32 len) override
	{
	}

	virtual void OnDisconnected() override
	{
		g_isLoggedIn = false;
		g_isInRoom = false;
		g_session = nullptr;
		std::cout << ">> [System] 서버와 연결이 끊어졌습니다." << std::endl;
	}
};

int main()
{
	// 1. 패킷 핸들러 초기화
	ServerPacketHandler::Init();

	// 2. 서비스 시작 (접속 시도)
	std::this_thread::sleep_for(std::chrono::seconds(1));

	ClientServiceRef service = MakeShared<ClientService>(
		NetAddress(L"127.0.0.1", 7777),
		MakeShared<IocpCore>(),
		MakeShared<ServerSession>,
		1);

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
	// [메인 스레드 로직] 입력 및 게임 흐름 제어
	// =============================================================

	// Step 1: 접속 대기 (I/O 스레드가 g_session을 채워줄 때까지 대기)
	std::cout << ">> [System] 서버 접속 대기 중...";
	while (g_session == nullptr)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}
	std::cout << " 완료!" << std::endl;


	// Step 2: 로그인 루프
	while (g_isLoggedIn == false)
	{
		if (g_session == nullptr) break; // 연결 끊기면 루프 탈출

		std::cout << "\n로그인할 플레이어 이름을 입력하세요: ";
		std::string playerName;
		std::cin >> playerName;

		// 입력 버퍼 비우기 (개행 문자 제거)
		std::cin.ignore((std::numeric_limits<std::streamsize>::max)(), '\n');

		Protocol::C_LOGIN_REQ pkt;
		pkt.set_name(playerName);
		auto sendBuffer = ServerPacketHandler::MakeSendBuffer(pkt);
		g_session->Send(sendBuffer);

		std::cout << ">> 로그인 요청 전송함. 응답 대기 중..." << std::endl;

		// 응답 올 때까지 잠시 대기 (너무 빠른 재입력 방지)
		for (int i = 0; i < 20; i++)
		{
			if (g_isLoggedIn) break; // 로그인 성공하면 즉시 탈출
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}
	}

	// Step 3: 게임/채팅 루프
	while (true)
	{
		if (g_session == nullptr) break; // 연결 끊기면 종료

		// A. 방 안에 있을 때 (채팅 모드)
		if (g_isInRoom)
		{
			// 프롬프트 없이 대기 (채팅 로그와 섞이는 것 방지)
			std::string message;
			std::getline(std::cin, message);

			if (message.empty()) continue;

			if (message == "/exit")
			{
				Protocol::C_LEAVE_ROOM_REQ reqPkt;
				g_session->Send(ServerPacketHandler::MakeSendBuffer(reqPkt));
				std::cout << ">> 방 나가기 요청 전송." << std::endl;
			}
			else
			{
				Protocol::C_ROOM_CHAT_REQ reqPkt;
				reqPkt.set_message(message);
				g_session->Send(ServerPacketHandler::MakeSendBuffer(reqPkt));
			}
		}
		// B. 로비에 있을 때 (명령어 모드)
		else
		{
			std::cout << "\n[1] 방 생성  [2] 방 입장  >> ";
			int choice = 0;
			std::cin >> choice;

			if (std::cin.fail())
			{
				std::cin.clear();
				std::cin.ignore((std::numeric_limits<std::streamsize>::max)(), '\n');
				continue;
			}
			std::cin.ignore((std::numeric_limits<std::streamsize>::max)(), '\n'); // 버퍼 비우기

			if (choice == 1)
			{
				std::cout << "생성할 방 이름: ";
				std::string roomName;
				std::getline(std::cin, roomName);

				Protocol::C_CREATE_ROOM_REQ reqPkt;
				reqPkt.set_roomname(roomName);
				reqPkt.set_type(Protocol::ROOM_GROUP);
				g_session->Send(ServerPacketHandler::MakeSendBuffer(reqPkt));
			}
			else if (choice == 2)
			{
				std::cout << "입장할 방 ID: ";
				int32 roomId;
				std::cin >> roomId;
				std::cin.ignore((std::numeric_limits<std::streamsize>::max)(), '\n');

				Protocol::C_JOIN_ROOM_REQ reqPkt;
				reqPkt.set_roomid(roomId);
				g_session->Send(ServerPacketHandler::MakeSendBuffer(reqPkt));
			}

			// UI 꼬임 방지용 대기
			std::this_thread::sleep_for(std::chrono::milliseconds(500));
		}
	}

	GThreadManager->Join();
}