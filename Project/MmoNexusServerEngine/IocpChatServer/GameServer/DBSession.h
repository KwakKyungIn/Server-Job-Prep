#pragma once
#include "Session.h"

class DBSession : public PacketSession
{
public:
	DBSession() {}
	virtual ~DBSession() {}

	virtual void OnConnected() override;
	virtual void OnDisconnected() override;
	virtual void OnRecvPacket(BYTE* buffer, int32 len) override;
	virtual void OnSend(int32 len) override;
	virtual void Ping() override;
};