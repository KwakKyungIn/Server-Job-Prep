#include "pch.h"
#include "LoginSession.h"
#include "S2SPacketHandler.h"

// GameServer 어디서든 로그인 서버와의 연결 상태를 확인하거나 패킷을 보낼 수 있게 전역으로 둠
shared_ptr<LoginSession> G_LoginSession = nullptr;

void LoginSession::OnConnected()
{
	G_LoginSession = static_pointer_cast<LoginSession>(shared_from_this());
	std::cout << " [GameServer] Connected To LoginServer!" << std::endl;

	// 연결 성공 시점에 우리 서버(GameServer)의 정보를 LoginServer에 등록해야 함
	// 그래야 유저가 로그인했을 때 부하가 적은 채널로 안내해줄 수 있음
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
	// 서버 간 통신(S2S) 패킷은 별도의 핸들러에서 처리함
	// 클라이언트 패킷과 섞이지 않게 구조를 분리했음
	S2SPacketHandler::HandlePacket(session, buffer, len);
}

void LoginSession::OnSend(int32 len)
{
}

void LoginSession::Ping()
{
	// LoginServer로 S2S 하트비트 요청
	Protocol::S2S_REQ_HEART_BEAT pkt;
	auto sendBuffer = S2SPacketHandler::MakeSendBuffer(pkt);
	Send(sendBuffer);
}
