#include "pch.h"
#include "GameSessionManager.h"
#include "PlayerSession.h"

GameSessionManager* GameSessionManager::GSessionManager = nullptr;

void GameSessionManager::Add(PlayerSessionRef session)
{
    if (!session) return;

    WRITE_LOCK;
    _bySessionId[session->GetSessionId()] = session;
}

void GameSessionManager::Remove(PlayerSessionRef session)
{
    if (!session) return;

    WRITE_LOCK;

    const uint64 sessionId = session->GetSessionId();

    // 1) sessionId 인덱스 제거
    _bySessionId.erase(sessionId);

    // 2) ✅ session->GetPlayerId() 같은 우회 접근 절대 금지
    auto it = _playerIdBySessionId.find(sessionId);
    if (it != _playerIdBySessionId.end())
    {
        const uint64 playerId = it->second;
        _playerIdBySessionId.erase(it);

        if (playerId != 0)
            _byPlayerId.erase(playerId);
    }
}

void GameSessionManager::BindPlayerId(PlayerSessionRef session, uint64 playerId)
{
    if (!session || playerId == 0) return;

    WRITE_LOCK;

    const uint64 sessionId = session->GetSessionId();

    // ✅ 같은 세션이 예전에 다른 playerId로 바인딩돼 있던 경우 정리
    auto prev = _playerIdBySessionId.find(sessionId);
    if (prev != _playerIdBySessionId.end())
    {
        const uint64 oldPlayerId = prev->second;
        if (oldPlayerId != 0 && oldPlayerId != playerId)
            _byPlayerId.erase(oldPlayerId);

        prev->second = playerId;
    }
    else
    {
        _playerIdBySessionId[sessionId] = playerId;
    }

    // ✅ 같은 playerId가 이미 다른 세션에 붙어있던 경우 덮어쓰기(중복 로그인 정책에 맞게 조정 가능)
    _byPlayerId[playerId] = session;
}

void GameSessionManager::UnbindPlayerId(uint64 playerId)
{
    if (playerId == 0) return;

    WRITE_LOCK;

    auto it = _byPlayerId.find(playerId);
    if (it != _byPlayerId.end())
    {
        // sessionId -> playerId 역인덱스도 같이 정리
        const uint64 sessionId = it->second ? it->second->GetSessionId() : 0;
        if (sessionId != 0)
            _playerIdBySessionId.erase(sessionId);

        _byPlayerId.erase(it);
    }
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