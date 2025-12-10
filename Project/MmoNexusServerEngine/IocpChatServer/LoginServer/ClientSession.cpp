#include "pch.h"
#include "ClientSession.h"
#include "LoginSessionManager.h"
#include "ClientPacketHandler.h" // [가정] 나중에 이 이름으로 핸들러 만들거임

void ClientSession::OnConnected()
{

	printf("[ClientSession] Client Connected\n");
	GSessionManager->Add(static_pointer_cast<ClientSession>(shared_from_this()));
}

void ClientSession::OnDisconnected()
{
	printf("[ClientSession] Client Disconnected\n");
	GSessionManager->Remove(static_pointer_cast<ClientSession>(shared_from_this()));
}
void ClientSession::OnRecvPacket(BYTE* buffer, int32 len)
{
	PacketSessionRef session = GetPacketSessionRef();

	// [Handler] 나중에 ClientPacketHandler 만들어서 연결할 것임
	// 지금은 헤더만 인클루드 해놓고 주석 처리하거나, 
	// 핸들러가 준비되면 아래 주석 풀 것.
	ClientPacketHandler::HandlePacket(session, buffer, len);
}

void ClientSession::OnSend(int32 len)
{
	// 보낼 때 별도 처리 필요하면 작성
}