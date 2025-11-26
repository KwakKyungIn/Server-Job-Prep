#include "pch.h"
#include "ServerPacketHandler.h"
#include <iostream>

// [Definition] 정적 멤버 메모리 할당
PacketHandlerFunc ServerPacketHandler::GPacketHandler[UINT16_MAX];

// [INVALID]
bool ServerPacketHandler::Handle_INVALID(PacketSessionRef& session, BYTE* buffer, int32 len)
{
	return false;
}

// [LOGIN RES] 로그인 결과
bool ServerPacketHandler::Handle_S_LOGIN_RES(PacketSessionRef& session, Protocol::S_LOGIN_RES& pkt)
{
	if (pkt.success())
	{
		std::cout << "✅ [Client] Login Success! My PlayerID: " << pkt.playerid() << std::endl;

		// [TEST] 로그인 성공하면 바로 채팅 한번 쏴본다.
		Protocol::C_CHAT_REQ chatPkt;
		chatPkt.set_message("Hello World! I am alive!");
		auto sendBuffer = ServerPacketHandler::MakeSendBuffer(chatPkt);
		session->Send(sendBuffer);
	}
	else
	{
		std::cout << "❌ [Client] Login Failed." << std::endl;
	}

	return true;
}

// [CHAT RES] 내 채팅이 서버에 잘 도착했는지 확인
bool ServerPacketHandler::Handle_S_CHAT_RES(PacketSessionRef& session, Protocol::S_CHAT_RES& pkt)
{
	// 성공했으면 굳이 로그 안 찍어도 됨 (도배 방지)
	if (pkt.success() == false)
	{
		std::cout << "⚠️ [Client] Chat Failed." << std::endl;
	}
	return true;
}

// [CHAT NTF] 누군가의 채팅을 수신 (방송)
bool ServerPacketHandler::Handle_S_CHAT_NTF(PacketSessionRef& session, Protocol::S_CHAT_NTF& pkt)
{
	std::cout << "💬 [" << pkt.name() << "]: " << pkt.message() << std::endl;
	return true;
}