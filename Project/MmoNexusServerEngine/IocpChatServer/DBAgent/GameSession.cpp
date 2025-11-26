#include "pch.h"
#include "GameSession.h"
#include "DBAgentPacketHandler.h" // 핸들러는 이거 하나 쓴다

void GameSession::OnConnected()
{
	std::cout << "✅ [DBAgent] GameServer Connected!" << std::endl;
}

void GameSession::OnDisconnected()
{
	std::cout << "❌ [DBAgent] GameServer Disconnected" << std::endl;
}

void GameSession::OnRecvPacket(BYTE* buffer, int32 len)
{
	PacketSessionRef session = GetPacketSessionRef();
	// GameServer가 보낸 요청(REQ)을 처리한다.
	DBAgentPacketHandler::HandlePacket(session, buffer, len);
}

void GameSession::OnSend(int32 len)
{
}