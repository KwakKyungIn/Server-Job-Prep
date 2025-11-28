#include "pch.h"
#include "ServerPacketHandler.h"

// Main.cpp의 전역 플래그 가져오기
extern std::atomic<bool> g_isLoggedIn;

PacketHandlerFunc ServerPacketHandler::GPacketHandler[UINT16_MAX];

bool ServerPacketHandler::Handle_INVALID(PacketSessionRef& session, BYTE* buffer, int32 len)
{
	return false;
}

bool ServerPacketHandler::Handle_S_LOGIN_RES(PacketSessionRef& session, Protocol::S_LOGIN_RES& pkt)
{
	if (pkt.success())
	{
		std::cout << "🎉 [Login Success] Welcome! PlayerID: " << pkt.playerid() << std::endl;
		// [KEY] 여기서 플래그를 올려줘야 Main의 로그인 루프가 끝난다.
		g_isLoggedIn = true;
	}
	else
	{
		std::cout << "💀 [Login Failed] Check your ID." << std::endl;
		g_isLoggedIn = false;
	}
	return true;
}

bool ServerPacketHandler::Handle_S_CHAT_RES(PacketSessionRef& session, Protocol::S_CHAT_RES& pkt)
{
	return true;
}

bool ServerPacketHandler::Handle_S_CHAT_NTF(PacketSessionRef& session, Protocol::S_CHAT_NTF& pkt)
{
	std::cout << "[" << pkt.name() << "] " << pkt.message() << std::endl;
	return true;
}

bool ServerPacketHandler::Handle_S_HEART_BEAT_RES(PacketSessionRef& session, Protocol::S_HEART_BEAT_RES& pkt)
{
	return true;
}