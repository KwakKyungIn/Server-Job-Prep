#include "pch.h"
#include "ServerSession.h"
#include "ServerPacketHandler.h"

extern PacketSessionRef g_session;

void ServerSession::OnConnected()
{
	std::cout << "\n [DummyClient] Connected To GameServer!" << std::endl;

	// [KEY POINT] 연결 성공 시, 전역 변수에 '나(this)'를 할당한다.
	// 이 한 줄이 실행되어야 Main의 '접속 대기' 루프가 깨진다.
	g_session = static_pointer_cast<PacketSession>(shared_from_this());
}

void ServerSession::OnDisconnected()
{
	std::cout << "\n [DummyClient] Disconnected" << std::endl;

	// 연결 끊기면 전역 변수도 비워주는 센스
	if (g_session == shared_from_this())
		g_session = nullptr;
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