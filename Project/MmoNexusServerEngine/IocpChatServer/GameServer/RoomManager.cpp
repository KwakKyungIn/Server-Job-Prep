#include "pch.h"
#include "RoomManager.h"

std::shared_ptr<RoomManager> GRoomManager = nullptr;

std::shared_ptr<GameRoom> RoomManager::GetOrCreateRoom(int32 channelId, int32 mapId)
{
    RoomKey key{ channelId, mapId };

    // 1차: 읽기 락으로 존재 여부 확인
    {
        READ_LOCK;
        auto it = _rooms.find(key);
        if (it != _rooms.end())
            return it->second;
    }

    // 없으면 쓰기 락으로 생성
    WRITE_LOCK;
    auto it = _rooms.find(key);
    if (it != _rooms.end())
        return it->second;

    auto room = MakeShared<GameRoom>();

    // TODO: 나중에 mapId별로 sizeX/sizeY/zoneSize를 DataManager/Config에서 가져가도 됨
    const int32 sizeX = 100;
    const int32 sizeY = 100;
    const int32 zoneSize = 10;

    room->Init(channelId, mapId, sizeX, sizeY, zoneSize);
    _rooms[key] = room;
    return room;
}

void RoomManager::UpdateAll()
{
    Vector<std::shared_ptr<GameRoom>> roomsCopy;

    {
        READ_LOCK;
        for (auto& kv : _rooms)
            roomsCopy.push_back(kv.second);
    }

    for (auto& room : roomsCopy)
    {
        room->Update();
    }
}
