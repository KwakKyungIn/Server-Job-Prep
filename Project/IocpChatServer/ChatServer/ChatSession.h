#pragma once
#include "Session.h"
#include "Player.h"

class ChatSession : public PacketSession
{
public:
	~ChatSession()
	{
		cout << "~ChatSession" << endl;
	}

	virtual void OnConnected() override;
	virtual void OnDisconnected() override;
	virtual void OnRecvPacket(BYTE* buffer, int32 len) override;
	virtual void OnSend(int32 len) override;

public:
	// A session should only have one player.
	PlayerRef GetPlayer() { return _player; }
	void SetPlayer(PlayerRef player) { _player = player; }

private:
	PlayerRef _player;
};
