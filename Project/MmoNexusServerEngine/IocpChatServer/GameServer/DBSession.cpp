#include "pch.h"
#include "DBSession.h"
#include "S2SPacketHandler.h"

// [Gigachad] Global Instance Definition
shared_ptr<PacketSession> G_DBSession = nullptr;

void DBSession::OnConnected()
{
	G_DBSession = static_pointer_cast<PacketSession>(shared_from_this());
	std::cout << "✅ [GameServer] Connected To DBAgent!" << std::endl;

	// [GIGACHAD TRIGGER] 접속하자마자 기획 데이터 로딩 요청 발사
	Protocol::S2S_REQ_LOAD_GAME_DATA pkt;
	auto sendBuffer = S2SPacketHandler::MakeSendBuffer(pkt);
	Send(sendBuffer);

	std::cout << "🚀 [GameServer] Request Loading Game Data..." << std::endl;
}

void DBSession::OnDisconnected()
{
	if (G_DBSession == shared_from_this())
		G_DBSession = nullptr;
	std::cout << "❌ [GameServer] Disconnected From DBAgent" << std::endl;
}

void DBSession::OnRecvPacket(BYTE* buffer, int32 len)
{
	PacketSessionRef session = GetPacketSessionRef();
	S2SPacketHandler::HandlePacket(session, buffer, len);
}

void DBSession::OnSend(int32 len)
{
}

void DBSession::Ping()
{
	std::cout << "GAME -> DB" << std::endl;
	Protocol::S2S_REQ_HEART_BEAT pkt;
	auto sendBuffer = S2SPacketHandler::MakeSendBuffer(pkt);
	Send(sendBuffer);
}