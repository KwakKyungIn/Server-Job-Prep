#include "pch.h"
#include "DBSession.h"
#include "S2SPacketHandler.h"

// [Gigachad] Global Instance Definition
shared_ptr<PacketSession> G_DBSession = nullptr;

void DBSession::OnConnected()
{
	G_DBSession = static_pointer_cast<PacketSession>(shared_from_this());
	std::cout << "✅ [GameServer] Connected To DBAgent!" << std::endl;
}

void DBSession::OnDisconnected()
{
	if (G_DBSession == shared_from_this())
		G_DBSession = nullptr;
	std::cout << "❌ [GameServer] Disconnected From DBAgent" << std::endl;
}

void DBSession::OnRecvPacket(BYTE* buffer, int32 len)
{
	// [DEBUG] 패킷이 진짜 들어오는지 확인
	PacketHeader* header = reinterpret_cast<PacketHeader*>(buffer);
	std::cout << "[DBSession] Packet Received! ID: " << header->id << " Size: " << header->size << std::endl;

	PacketSessionRef session = GetPacketSessionRef();
	S2SPacketHandler::HandlePacket(session, buffer, len);
}

void DBSession::OnSend(int32 len)
{
}

void DBSession::Ping()
{
	Protocol::S2S_REQ_HEART_BEAT pkt;
	auto sendBuffer = S2SPacketHandler::MakeSendBuffer(pkt);
	Send(sendBuffer);
}