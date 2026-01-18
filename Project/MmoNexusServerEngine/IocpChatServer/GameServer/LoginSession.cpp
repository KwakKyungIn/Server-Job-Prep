#include "pch.h"
#include "LoginSession.h"
#include "S2SPacketHandler.h"

// [GIGACHAD] 전역 변수 (필요하면 GameServer.cpp 등에서 extern으로 씀)
shared_ptr<LoginSession> G_LoginSession = nullptr;

void LoginSession::OnConnected()
{
	G_LoginSession = static_pointer_cast<LoginSession>(shared_from_this());
	std::cout << " [GameServer] Connected To LoginServer!" << std::endl;

	// [TODO] 연결되자마자 "나 1채널이고, IP는 뭐고, 포트는 7777이야"라고 신고해야 함.
	// 그래야 LoginServer가 유저한테 "1채널로 가세요"라고 알려줄 수 있음.
}

void LoginSession::OnDisconnected()
{
	if (G_LoginSession == shared_from_this())
		G_LoginSession = nullptr;

	std::cout << " [GameServer] Disconnected From LoginServer" << std::endl;
}

void LoginSession::OnRecvPacket(BYTE* buffer, int32 len)
{
	PacketSessionRef session = GetPacketSessionRef();
	// LoginServer에서 오는 패킷 처리 (S2S 핸들러 공용 사용)
	S2SPacketHandler::HandlePacket(session, buffer, len);
}

void LoginSession::OnSend(int32 len)
{
}