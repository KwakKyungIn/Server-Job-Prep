#include "pch.h"
#include "RoomManager.h"
#include "Metrics.h" // [METRICS]

RoomManager GRoomManager;

int32 RoomManager::AddRoom(RoomRef room)
{
    WRITE_LOCK;
    int32 newId = _nextRoomId++;
    _rooms[newId] = room;

    // [METRICS] 현재 방 수 증가 및 1초 피크 갱신
    uint32_t now = GMetrics.rooms_gauge.fetch_add(1, std::memory_order_relaxed) + 1;
    uint32_t prev = GMetrics.rooms_peak.load(std::memory_order_relaxed);
    while (now > prev && !GMetrics.rooms_peak.compare_exchange_weak(prev, now, std::memory_order_relaxed)) {}

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
    auto erased = _rooms.erase(roomId);
    if (erased > 0) {
        // [METRICS] 현재 방 수 감소 (피크는 Tick에서 리셋 방식이라 여기서 건드릴 필요 없음)
        GMetrics.rooms_gauge.fetch_sub(1, std::memory_order_relaxed);
    }
}
