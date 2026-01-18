#include "pch.h"
#include "GameSession.h"
#include "DBAgentPacketHandler.h"

void GameSession::OnConnected()
{
	std::cout << " [DBAgent] GameServer Connected!" << std::endl;
}

void GameSession::OnDisconnected()
{
	std::cout << " [DBAgent] GameServer Disconnected" << std::endl;
}

void GameSession::OnRecvPacket(BYTE* buffer, int32 len)
{
	PacketSessionRef session = GetPacketSessionRef();

	// GameServer가 보낸 요청(S2S_REQ_LOGIN 등)을 처리한다.
	DBAgentPacketHandler::HandlePacket(session, buffer, len);
}

void GameSession::OnSend(int32 len)
{
}