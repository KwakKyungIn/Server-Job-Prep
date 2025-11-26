#include "pch.h"
#include "ServerSession.h"
#include "ServerPacketHandler.h"

void ServerSession::OnConnected()
{
	std::cout << "✅ [Client] Connected To GameServer!" << std::endl;

	// [AUTO LOGIN] 접속하자마자 로그인 시도
	Protocol::C_LOGIN_REQ pkt;
	pkt.set_name("KwakPpiPpi"); // 테스트용 이름

	auto sendBuffer = ServerPacketHandler::MakeSendBuffer(pkt);
	Send(sendBuffer);
}

void ServerSession::OnDisconnected()
{
	std::cout << "❌ [Client] Disconnected From GameServer" << std::endl;
}

void ServerSession::OnRecvPacket(BYTE* buffer, int32 len)
{
	PacketSessionRef session = GetPacketSessionRef();
	PacketHeader* header = reinterpret_cast<PacketHeader*>(buffer);

	// 서버에서 온 패킷 처리
	ServerPacketHandler::HandlePacket(session, buffer, len);
}

void ServerSession::OnSend(int32 len)
{
}