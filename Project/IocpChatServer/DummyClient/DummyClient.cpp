#include "pch.h"
#include "ThreadManager.h"
#include "Service.h"
#include "Session.h"
#include "BufferReader.h"
#include "ServerPacketHandler.h"
#include "Protocol.pb.h"
#include <iostream>

class ServerSession : public PacketSession
{
public:
	~ServerSession()
	{
		std::cout << "~ServerSession" << std::endl;
	}

	virtual void OnConnected() override
	{
		// 1. Get user input for the player name.
		std::cout << "Enter player name to login: ";
		std::string playerName;
		std::cin >> playerName;

		// 2. Create a login request packet with the entered name.
		Protocol::C_LOGIN_REQ pkt;
		pkt.set_name(playerName);

		// 3. Send the login request to the server.
		auto sendBuffer = ServerPacketHandler::MakeSendBuffer(pkt);
		Send(sendBuffer);

		std::cout << "Sent login request for player '" << playerName << "' to server." << std::endl;
	}

	virtual void OnRecvPacket(BYTE* buffer, int32 len) override
	{
		PacketSessionRef session = GetPacketSessionRef();
		PacketHeader* header = reinterpret_cast<PacketHeader*>(buffer);

		// Handle the received packet.
		ServerPacketHandler::HandlePacket(session, buffer, len);
	}

	virtual void OnSend(int32 len) override
	{
		//std::cout << "OnSend Len = " << len << std::endl;
	}

	virtual void OnDisconnected() override
	{
		//std::cout << "Disconnected" << std::endl;
	}
};

int main()
{
	ServerPacketHandler::Init();

	std::this_thread::sleep_for(std::chrono::seconds(1));

	ClientServiceRef service = MakeShared<ClientService>(
		NetAddress(L"127.0.0.1", 7777),
		MakeShared<IocpCore>(),
		MakeShared<ServerSession>,
		1);

	ASSERT_CRASH(service->Start());

	// Run the client's network loop on a single thread.
	GThreadManager->Launch([=]()
		{
			while (true)
			{
				service->GetIocpCore()->Dispatch();
			}
		});

	// Join the thread to keep the application running.
	GThreadManager->Join();
}
