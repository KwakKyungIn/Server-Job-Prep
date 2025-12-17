#include "pch.h"
#include "RoomManager.h"
#include "DataManager.h"

std::shared_ptr<RoomManager> GRoomManager = nullptr;

std::shared_ptr<GameRoom> RoomManager::GetOrCreateRoom(int32 channelId, int32 mapId)
{

    DataManager* dm = DataManager::Instance();
    if (dm && dm->IsValidMapId(mapId) == false)
        mapId = dm->GetDefaultMapId();


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


    const MapConfig* cfg = (dm ? dm->GetMapConfig(mapId) : nullptr);

    const int32 sizeX = cfg ? cfg->sizeX : 100;
    const int32 sizeY = cfg ? cfg->sizeY : 100;
    const int32 zoneSize = cfg ? cfg->zoneSize : 10;

    auto room = MakeShared<GameRoom>();

    

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
