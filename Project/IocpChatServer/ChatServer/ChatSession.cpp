#include "pch.h"
#include "ChatSession.h"
#include "ChatSessionManager.h"
#include "ClientPacketHandler.h"

void ChatSession::OnConnected()
{
	GSessionManager.Add(static_pointer_cast<ChatSession>(shared_from_this()));
}

void ChatSession::OnDisconnected()
{
	GSessionManager.Remove(static_pointer_cast<ChatSession>(shared_from_this()));
}

void ChatSession::OnRecvPacket(BYTE* buffer, int32 len)
{
	PacketSessionRef session = GetPacketSessionRef();
	PacketHeader* header = reinterpret_cast<PacketHeader*>(buffer);

	// TODO : packetId 대역 체크
	ClientPacketHandler::HandlePacket(session, buffer, len);
}

void ChatSession::OnSend(int32 len)
{
}