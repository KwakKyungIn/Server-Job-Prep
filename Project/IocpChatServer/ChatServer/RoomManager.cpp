#include "pch.h"
#include "RoomManager.h"

RoomManager GRoomManager;

int32 RoomManager::AddRoom(RoomRef room)
{
	WRITE_LOCK;
	int32 newId = _nextRoomId++;
	_rooms[newId] = room;
	return newId;
}

RoomRef RoomManager::FindRoom(int32 roomId)
{
	READ_LOCK;
	auto it = _rooms.find(roomId);
	if (it != _rooms.end())
	{
		return it->second;
	}
	return nullptr;
}

void RoomManager::RemoveRoom(int32 roomId)
{
	WRITE_LOCK;
	_rooms.erase(roomId);
}
