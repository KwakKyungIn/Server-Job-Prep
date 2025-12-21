#include "pch.h"
#include "RoomManager.h"
#include "DataManager.h"

std::shared_ptr<RoomManager> GRoomManager = nullptr;

std::shared_ptr<GameRoom> RoomManager::GetOrCreateRoom(int32 channelId, int32 mapId, int64 instanceId)
{
    DataManager* dm = DataManager::Instance();
    if (dm && dm->IsValidMapId(mapId) == false)
        mapId = dm->GetDefaultMapId();

    RoomKey key{ channelId, mapId, instanceId }; //instanceId 포함

    {
        READ_LOCK;
        auto it = _rooms.find(key);
        if (it != _rooms.end())
            return it->second;
    }

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

    // ✅ GameRoom이 instanceId를 내부에 저장하도록(추가 예정)
    //room->SetInstanceId(instanceId);

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
