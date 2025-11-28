#pragma once
#include "Session.h"

class ChatSession : public PacketSession
{
public:
	ChatSession() {}
	virtual ~ChatSession() {}

	virtual void OnConnected() override;
	virtual void OnDisconnected() override;
	virtual void OnRecvPacket(BYTE* buffer, int32 len) override;
	virtual void OnSend(int32 len) override;
	virtual void Ping() override;
};