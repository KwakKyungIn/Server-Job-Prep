#pragma once
#include <map>
#include <memory>
#include "Room.h"

// Forward declaration
class Room;

using RoomRef = std::shared_ptr<Room>;

class RoomManager
{
public:
	int32 AddRoom(RoomRef room);
	RoomRef FindRoom(int32 roomId);
	void RemoveRoom(int32 roomId);

private:
	USE_LOCK;
	int32 _nextRoomId = 1;
	std::map<int32, RoomRef> _rooms;
};

extern RoomManager GRoomManager;
