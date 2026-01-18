#include "pch.h"
#include "ChatSession.h"
#include "DBAgentPacketHandler.h" // 여기도 똑같은 핸들러 쓴다

void ChatSession::OnConnected()
{
	std::cout << " [DBAgent] ChatServer Connected!" << std::endl;
}

void ChatSession::OnDisconnected()
{
	std::cout << " [DBAgent] ChatServer Disconnected" << std::endl;
}

void ChatSession::OnRecvPacket(BYTE* buffer, int32 len)
{
	PacketSessionRef session = GetPacketSessionRef();
	// ChatServer가 보낸 요청(REQ)을 처리한다.
	// (예: 로그 저장 요청 등)
	DBAgentPacketHandler::HandlePacket(session, buffer, len);
}

void ChatSession::OnSend(int32 len)
{
}