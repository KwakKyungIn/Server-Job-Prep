#include "pch.h"
#include "LoginSession.h"
//#include "LoginPacketHandler.h" // 나중에 만들 패킷 핸들러

void LoginSession::OnConnected()
{
	cout << "[LoginSession] Client Connected: " << GetSessionId() << endl;
}

void LoginSession::OnDisconnected()
{
	cout << "[LoginSession] Client Disconnected: " << GetSessionId() << endl;
}

void LoginSession::OnRecvPacket(BYTE* buffer, int32 len)
{
	//PacketSession::OnRecvPacket(buffer, len);

	// 나중에 패킷 핸들러 연결
	// PacketHeader* header = reinterpret_cast<PacketHeader*>(buffer);
	// LoginPacketHandler::HandlePacket(GetSessionRef(), buffer, len);
}

void LoginSession::OnSend(int32 len)
{
	// 전송 완료 시 호출
}