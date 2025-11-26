#include "pch.h"
#include "GameSession.h"
#include "ChatServerPacketHandler.h" // [REQ 처리] GameServer의 요청 처리

void GameSession::OnConnected()
{
	// GameServer가 우리(ChatServer)에게 접속했다!
	std::cout << "✅ [ChatServer] GameServer Connected!" << std::endl;
}

void GameSession::OnDisconnected()
{
	std::cout << "❌ [ChatServer] GameServer Disconnected" << std::endl;
}

void GameSession::OnRecvPacket(BYTE* buffer, int32 len)
{
	PacketSessionRef session = GetPacketSessionRef();
	// [GameServer -> ChatServer] : "이거 방송해줘" 같은 요청 처리
	ChatServerPacketHandler::HandlePacket(session, buffer, len);
}

void GameSession::OnSend(int32 len)
{
}