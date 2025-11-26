#include "pch.h"
#include "DBSession.h"
#include "S2SPacketHandler.h" // [DB Response]

void DBSession::OnConnected()
{
	std::cout << "✅ [GameServer] Connected To DBAgent!" << std::endl;
}

void DBSession::OnDisconnected()
{
	std::cout << "❌ [GameServer] Disconnected From DBAgent" << std::endl;
}

void DBSession::OnRecvPacket(BYTE* buffer, int32 len)
{
	PacketSessionRef session = GetPacketSessionRef();
	// DB가 보낸 응답 처리 (로그인 결과 등)
	S2SPacketHandler::HandlePacket(session, buffer, len);
}

void DBSession::OnSend(int32 len)
{
}