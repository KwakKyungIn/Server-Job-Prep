#pragma once
#include "Protocol.pb.h"
#include "Session.h"
#include "Room.h"

class Player
{
public:
	uint64					playerId = 0;
	string					name;
	ChatSessionRef			ownerSession; // Cycle

public:
	// A room the player is currently in.
	// Circular reference is handled in pch.h using class Room;
	RoomRef _room = nullptr;
};

using PlayerRef = shared_ptr<Player>;
