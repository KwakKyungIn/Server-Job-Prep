#pragma once

class Room
{
public:
	void Enter(PlayerRef player);
	void Leave(PlayerRef player);
	void Broadcast(SendBufferRef sendBuffer);

	void SetId(int32 roomId) { _roomId = roomId; }
	void SetName(const std::string& name) { _roomName = name; }
	void SetType(Protocol::RoomType type) { _roomType = type; }

	int32 GetId() { return _roomId; }
	const std::string& GetName() { return _roomName; }
	Protocol::RoomType GetType() { return _roomType; }

private:
	USE_LOCK;
	int32 _roomId = 0;
	std::string _roomName;
	Protocol::RoomType _roomType;
	std::map<uint64, PlayerRef> _players;
};

extern Room GRoom;
