#include "pch.h"
#include "DBSession.h"
#include "S2SPacketHandler.h" // DB 응답 처리

void DBSession::OnConnected()
{
	std::cout << "✅ [ChatServer] Connected To DBAgent!" << std::endl;
}

void DBSession::OnDisconnected()
{
	std::cout << "❌ [ChatServer] Disconnected From DBAgent" << std::endl;
}

void DBSession::OnRecvPacket(BYTE* buffer, int32 len)
{
	PacketSessionRef session = GetPacketSessionRef();
	// [DBAgent -> ChatServer] 응답 처리 (저장 성공/실패 등)
	S2SPacketHandler::HandlePacket(session, buffer, len);
}

void DBSession::OnSend(int32 len)
{
}