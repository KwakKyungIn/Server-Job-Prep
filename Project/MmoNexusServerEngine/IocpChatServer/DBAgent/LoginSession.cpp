#include "pch.h"
#include "LoginSession.h"
#include "DBAgentPacketHandler.h"

void LoginSession::OnConnected()
{
	std::cout << "✅ [DBAgent] LoginServer Connected!" << std::endl;
}

void LoginSession::OnDisconnected()
{
	std::cout << "❌ [DBAgent] LoginServer Disconnected" << std::endl;
}

void LoginSession::OnRecvPacket(BYTE* buffer, int32 len)
{
	PacketSessionRef session = GetPacketSessionRef();

	// 공용 핸들러 사용
	DBAgentPacketHandler::HandlePacket(session, buffer, len);
}

void LoginSession::OnSend(int32 len)
{
}