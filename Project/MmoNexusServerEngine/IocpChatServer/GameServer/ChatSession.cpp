#include "pch.h"
#include "ChatSession.h"
#include "S2SPacketHandler.h" // [Chat Response]

void ChatSession::OnConnected()
{
	std::cout << "✅ [GameServer] Connected To ChatServer!" << std::endl;
}

void ChatSession::OnDisconnected()
{
	std::cout << "❌ [GameServer] Disconnected From ChatServer" << std::endl;
}

void ChatSession::OnRecvPacket(BYTE* buffer, int32 len)
{
	PacketSessionRef session = GetPacketSessionRef();
	// ChatServer가 보낸 응답 처리 (방송 성공 여부 등)
	// S2S 핸들러를 공유해서 쓴다.
	S2SPacketHandler::HandlePacket(session, buffer, len);
}

void ChatSession::OnSend(int32 len)
{
}