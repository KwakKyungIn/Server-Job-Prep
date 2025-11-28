#include "pch.h"
#include "PlayerSession.h"
#include "ClientPacketHandler.h"
#include "GameSessionManager.h" // 매니저 등록용



void PlayerSession::OnConnected()
{
	// [Manager] 입장 신고
	GameSessionManager::GSessionManager->Add(static_pointer_cast<PlayerSession>(shared_from_this()));

	// std::cout << "[Player] Client Connected (ID: " << _sessionId << ")" << std::endl;
}

void PlayerSession::OnDisconnected()
{
	// [Manager] 퇴장 신고
	GameSessionManager::GSessionManager->Remove(static_pointer_cast<PlayerSession>(shared_from_this()));

	// std::cout << "[Player] Client Disconnected (ID: " << _sessionId << ")" << std::endl;
}

void PlayerSession::OnRecvPacket(BYTE* buffer, int32 len)
{
	PacketSessionRef session = GetPacketSessionRef();

	// 패킷 핸들러에게 넘길 때, 이 세션이 누군지 알 수 있게 됨
	ClientPacketHandler::HandlePacket(session, buffer, len);
}

void PlayerSession::OnSend(int32 len)
{
}