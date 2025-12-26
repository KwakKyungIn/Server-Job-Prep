#pragma once
#include "GameRoom.h"
#include "LobbyRoom.h"
struct RoomKey
{
    int32 channelId;
    int32 mapId;
    int64 instanceId = 0; // 추가 (0 = world)
    bool operator<(const RoomKey& other) const
    {
        if (channelId != other.channelId) return channelId < other.channelId;
        if (mapId != other.mapId)         return mapId < other.mapId;
        return instanceId < other.instanceId;
    }
};

class RoomManager
{
public:
    RoomManager() = default;
    ~RoomManager() = default;

    std::shared_ptr<GameRoom> GetOrCreateRoom(int32 channelId, int32 mapId, int64 instanceId = 0);
    std::shared_ptr<GameRoom> FindRoom(int32 channelId, int32 mapId, int64 instanceId = 0);

    // ✅ 채널당 1개 로비 (Player staging / EnterGame -> World로 transfer용)
    std::shared_ptr<LobbyRoom> GetOrCreateLobby(int32 channelId);


    // 메인 루프에서 호출
    void UpdateAll();

private:
    USE_LOCK;
    std::map<RoomKey, std::shared_ptr<GameRoom>> _rooms;

    void PurgeInstanceRooms(uint64 nowMs);

    // ✅ channelId -> lobby
    std::unordered_map<int32, std::shared_ptr<LobbyRoom>> _lobbies;

};

// 전역 포인터 (GameServer, 핸들러에서 같이 씀)
extern std::shared_ptr<RoomManager> GRoomManager;
