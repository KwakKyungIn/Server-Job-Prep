#include "pch.h"
#include "DBAgentSession.h"
#include "S2SPacketHandler.h" // [가정] 서버간 통신 핸들러 (혹은 DBAgentPacketHandler)

void DBAgentSession::OnConnected()
{
	printf(" [DBAgentSession] Connected To DBAgent!\n");
}

void DBAgentSession::OnDisconnected()
{
	printf(" [DBAgentSession] Disconnected From DBAgent.\n");
}

void DBAgentSession::OnRecvPacket(BYTE* buffer, int32 len)
{
	PacketSessionRef session = GetPacketSessionRef();

	// [Handler] 서버간 통신 핸들러 연결
	S2SPacketHandler::HandlePacket(session, buffer, len);
}

void DBAgentSession::OnSend(int32 len)
{
	// 전송 완료 로그 등 필요 시 작성
}