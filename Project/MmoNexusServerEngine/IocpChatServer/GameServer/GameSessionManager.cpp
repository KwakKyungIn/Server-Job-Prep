#include "pch.h"
#include "GameSessionManager.h"
#include "PlayerSession.h"

GameSessionManager* GameSessionManager::GSessionManager = nullptr;


void GameSessionManager::Add(PlayerSessionRef session)
{
    ASSERT_CRASH(this != nullptr);

    if (!session) return;
    WRITE_LOCK;
    _bySessionId[session->GetSessionId()] = session;
}

void GameSessionManager::Remove(PlayerSessionRef session)
{
    if (!session) return;

    WRITE_LOCK;

    const uint64 sessionId = session->GetSessionId();
    _bySessionId.erase(sessionId);

    // 여기 중요: Remove가 먼저 호출되고 그 다음에 _player = nullptr 되니까
    // 지금 시점엔 GetPlayerId()가 살아있다.
    const uint64 playerId = session->GetPlayerId();
    if (playerId != 0)
        _byPlayerId.erase(playerId);
}

void GameSessionManager::BindPlayerId(PlayerSessionRef session, uint64 playerId)
{
    if (!session || playerId == 0) return;

    WRITE_LOCK;
    _byPlayerId[playerId] = session;
}

void GameSessionManager::UnbindPlayerId(uint64 playerId)
{
    if (playerId == 0) return;
    WRITE_LOCK;
    _byPlayerId.erase(playerId);
}

PlayerSessionRef GameSessionManager::FindBySessionId(uint64 sessionId)
{
    READ_LOCK;
    auto it = _bySessionId.find(sessionId);
    if (it == _bySessionId.end()) return nullptr;
    return it->second;
}

PlayerSessionRef GameSessionManager::FindByPlayerId(uint64 playerId)
{
    READ_LOCK;
    auto it = _byPlayerId.find(playerId);
    if (it == _byPlayerId.end()) return nullptr;
    return it->second;
}

void GameSessionManager::Broadcast(SendBufferRef sendBuffer)
{
    if (!sendBuffer) return;

    READ_LOCK;
    for (auto it = _bySessionId.begin(); it != _bySessionId.end(); ++it)
    {
        PlayerSessionRef s = it->second;
        if (s) s->Send(sendBuffer);
    }
}