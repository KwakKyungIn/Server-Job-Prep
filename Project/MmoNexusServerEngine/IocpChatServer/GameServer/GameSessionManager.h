#pragma once
#include "Lock.h"
#include <unordered_map>

class PlayerSession;
using PlayerSessionRef = std::shared_ptr<PlayerSession>;

class GameSessionManager
{
public:
    static GameSessionManager* GSessionManager;

    void Add(PlayerSessionRef session);
    void Remove(PlayerSessionRef session);

    void BindPlayerId(PlayerSessionRef session, uint64 playerId);
    void UnbindPlayerId(uint64 playerId);

    PlayerSessionRef FindBySessionId(uint64 sessionId);
    PlayerSessionRef FindByPlayerId(uint64 playerId);

    void Broadcast(SendBufferRef sendBuffer);

    uint64 GetPlayerIdBySessionId(uint64 sessionId);

    void SetPlayerName(uint64 playerId, const std::string& name);
    std::string GetPlayerName(uint64 playerId);

private:
    USE_LOCK;

    HashMap<uint64, PlayerSessionRef> _bySessionId;   // sessionId -> session
    HashMap<uint64, PlayerSessionRef> _byPlayerId;    // playerId  -> session

    //  핵심: 우회 접근 막기용 역인덱스
    HashMap<uint64, uint64> _playerIdBySessionId;     // sessionId -> playerId

    HashMap<uint64, std::string> _nameByPlayerId;     // playerId -> name
};
