#include "pch.h"
#include "GameServerSession.h"
#include "S2SPacketHandler.h"

void GameServerSession::OnConnected()
{
	std::cout << " [LoginServer] GameServer Connected!" << std::endl;
}

void GameServerSession::OnDisconnected()
{
	std::cout << " [LoginServer] GameServer Disconnected" << std::endl;
}

void GameServerSession::OnRecvPacket(BYTE* buffer, int32 len)
{
	PacketSessionRef session = GetPacketSessionRef();
	// GameServer가 보낸 패킷 처리 (S2S 핸들러)
	S2SPacketHandler::HandlePacket(session, buffer, len);
}

void GameServerSession::OnSend(int32 len)
{
}