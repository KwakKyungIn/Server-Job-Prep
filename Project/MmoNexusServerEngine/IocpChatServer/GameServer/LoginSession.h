#pragma once
#include "Session.h"

// GameServer 입장에서는 LoginServer에게 연결을 요청하는 클라이언트 포지션임
// 따라서 PacketSession을 상속받아 비동기 I/O를 처리함
class LoginSession : public PacketSession
{
public:
	LoginSession() {}
	virtual ~LoginSession() {}

	virtual void OnConnected() override;
	virtual void OnDisconnected() override;
	virtual void OnRecvPacket(BYTE* buffer, int32 len) override;
	virtual void OnSend(int32 len) override;
	virtual void Ping() override;
};
