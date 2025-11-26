#pragma once
#include "Session.h"

class PlayerSession : public PacketSession
{
public:
	PlayerSession() {}
	virtual ~PlayerSession() {}

	virtual void OnConnected() override;
	virtual void OnDisconnected() override;
	virtual void OnRecvPacket(BYTE* buffer, int32 len) override;
	virtual void OnSend(int32 len) override;
};