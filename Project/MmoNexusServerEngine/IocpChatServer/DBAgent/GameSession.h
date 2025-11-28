#pragma once
#include "Session.h"

// GameServer와의 연결을 관리하는 세션 (DBAgent 입장에선 GameServer가 클라이언트임)
class GameSession : public PacketSession
{
public:
	GameSession() {}
	virtual ~GameSession() {}

	virtual void OnConnected() override;
	virtual void OnDisconnected() override;
	virtual void OnRecvPacket(BYTE* buffer, int32 len) override;
	virtual void OnSend(int32 len) override;
};