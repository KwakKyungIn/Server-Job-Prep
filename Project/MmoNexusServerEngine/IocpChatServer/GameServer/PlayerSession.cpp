#include "pch.h"
#include "PlayerSession.h"
#include "ClientPacketHandler.h" // [User Request]

void PlayerSession::OnConnected()
{
	// std::cout << "[Player] Client Connected" << std::endl;
}

void PlayerSession::OnDisconnected()
{
	// std::cout << "[Player] Client Disconnected" << std::endl;
}

void PlayerSession::OnRecvPacket(BYTE* buffer, int32 len)
{
	PacketSessionRef session = GetPacketSessionRef();
	// 유저가 보낸 패킷 처리 (C_LOGIN, C_CHAT 등)
	ClientPacketHandler::HandlePacket(session, buffer, len);
}

void PlayerSession::OnSend(int32 len)
{
}