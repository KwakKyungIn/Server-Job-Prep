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

    room->SetInstanceId(instanceId);

    _rooms[key] = room;
    return room;
}

std::shared_ptr<GameRoom> RoomManager::FindRoom(int32 channelId, int32 mapId, int64 instanceId)
{
    DataManager* dm = DataManager::Instance();
    if (dm && dm->IsValidMapId(mapId) == false)
        mapId = dm->GetDefaultMapId();

    RoomKey key{ channelId, mapId, instanceId };

    READ_LOCK;
    auto it = _rooms.find(key);
    if (it == _rooms.end()) return nullptr;
    return it->second;
}

#include <Windows.h>

void RoomManager::UpdateAll()
{
    Vector<std::shared_ptr<GameRoom>> roomsCopy;

    {
        READ_LOCK;
        for (auto& kv : _rooms)
            roomsCopy.push_back(kv.second);
    }

    // ✅ Update는 Room Actor에서 돌려라
    for (auto& room : roomsCopy)
    {
        if (!room) continue;
        room->PushJob([room]()
            {
                room->Update();
            });
    }

    // ✅ purge는 레지스트리에서 처리
    const uint64 nowMs = ::GetTickCount64();
    PurgeInstanceRooms(nowMs);
}

void RoomManager::PurgeInstanceRooms(uint64 nowMs)
{
    Vector<RoomKey> eraseKeys;

    {
        READ_LOCK;
        for (auto& kv : _rooms)
        {
            const RoomKey& key = kv.first;
            const auto& room = kv.second;
            if (!room) continue;

            // instance 룸만 후보
            if (key.instanceId == 0)
                continue;

            if (room->ShouldPurge(nowMs))
                eraseKeys.push_back(key);
        }
    }

    if (eraseKeys.empty())
        return;

    WRITE_LOCK;
    for (const RoomKey& key : eraseKeys)
    {
        auto it = _rooms.find(key);
        if (it == _rooms.end())
            continue;

        auto room = it->second;
        if (room && room->ShouldPurge(nowMs))
        {
            _rooms.erase(it);
            printf("[RoomManager] Purged instance room: ch=%d map=%d inst=%lld\n",
                key.channelId, key.mapId, key.instanceId);
        }
    }
}
