#pragma once
#include "Protocol.pb.h"
#include "Session.h"
#include "Player.h"
#include "JobQueue.h"
#include <map>
#include <memory>
#include <fstream>

class Room;
using RoomRef = std::shared_ptr<Room>;

class Room : public std::enable_shared_from_this<Room>
{
public:
    Room();
    ~Room();

    bool Enter(PlayerRef player);
    void Leave(PlayerRef player);

    void Broadcast(SendBufferRef sendBuffer);
    void BroadcastWithoutSelf(SendBufferRef sendBuffer, uint64 selfId);

    void SetId(int32 roomId);
    void SetName(const std::string& name) { _roomName = name; }
    void SetType(Protocol::RoomType type) { _roomType = type; }

    int32 GetId() { return _roomId; }
    const std::string& GetName() { return _roomName; }
    Protocol::RoomType GetType() { return _roomType; }

    void LogChat(uint64 playerId, const std::string& playerName, const std::string& message);
    void LogChatToDB(uint64 playerId, const std::string& playerName, const std::string& message);

    const std::map<uint64, PlayerRef>& GetPlayers() { return _players; }

    size_t PendingJobs() const { return _jobQueue.Size(); }

private:
    USE_LOCK;
    int32 _roomId = 0;
    std::string _roomName;
    Protocol::RoomType _roomType;
    std::map<uint64, PlayerRef> _players;

    JobQueue _jobQueue;
    std::ofstream _logFile;
};
