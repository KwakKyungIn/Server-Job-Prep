#include "pch.h"
#include "GameSessionManager.h"
#include "PlayerSession.h"

GameSessionManager* GameSessionManager::GSessionManager = nullptr;

// 새로운 세션이 연결되면 관리 목록에 추가
// 아직 로그인은 안 한 상태라 SessionID만 가지고 있음
void GameSessionManager::Add(PlayerSessionRef session)
{
    if (!session) return;

    WRITE_LOCK;
    _bySessionId[session->GetSessionId()] = session;
}

// 세션 종료 시 관리 목록에서 제거
// 여기서 중요한 건 단순히 Map에서 빼는 게 아니라, 
// PlayerID와의 연결 관계도 깔끔하게 정리해줘야 한다는 점임
void GameSessionManager::Remove(PlayerSessionRef session)
{
    if (!session) return;

    WRITE_LOCK;

    const uint64 sessionId = session->GetSessionId();

    // 1. SessionID 맵에서 제거
    _bySessionId.erase(sessionId);

    // 2. PlayerID 매핑 정보도 제거
    // 주의: session->GetPlayerId()로 접근하면 안 됨. 
    // 세션이 파괴되는 시점에는 Player 객체가 이미 날아갔을 수도 있어서
    // 별도로 관리하던 역인덱스(_playerIdBySessionId)를 통해 찾아야 안전함
    auto it = _playerIdBySessionId.find(sessionId);
    if (it != _playerIdBySessionId.end())
    {
        const uint64 playerId = it->second;
        _playerIdBySessionId.erase(it);

        if (playerId != 0)
        {
            _byPlayerId.erase(playerId);
            auto nameIt = _nameByPlayerId.find(playerId);
            if (nameIt != _nameByPlayerId.end())
            {
                const std::string name = nameIt->second;
                _nameByPlayerId.erase(nameIt); // 이름 캐시도 정리
                RebuildNameIndex_Locked(name);
            }
        }
    }
}

// 로그인 성공 시 세션과 PlayerID를 바인딩함
// 이 시점부터는 FindByPlayerId로 해당 유저의 세션을 찾을 수 있게 됨
void GameSessionManager::BindPlayerId(PlayerSessionRef session, uint64 playerId)
{
    if (!session || playerId == 0) return;

    WRITE_LOCK;

    const uint64 sessionId = session->GetSessionId();

    // 기존에 다른 ID로 바인딩되어 있었다면 정리 (재로그인 등)
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

    // 중복 로그인 처리 정책: 나중 접속이 우선권을 가짐
    _byPlayerId[playerId] = session;
}

// 로그아웃 시 바인딩 해제
void GameSessionManager::UnbindPlayerId(uint64 playerId)
{
    if (playerId == 0) return;

    WRITE_LOCK;

    auto it = _byPlayerId.find(playerId);
    if (it != _byPlayerId.end())
    {
        // SessionID -> PlayerID 역인덱스도 같이 정리
        const uint64 sessionId = it->second ? it->second->GetSessionId() : 0;
        if (sessionId != 0)
            _playerIdBySessionId.erase(sessionId);

        _byPlayerId.erase(it);
    }
    auto nameIt = _nameByPlayerId.find(playerId);
    if (nameIt != _nameByPlayerId.end())
    {
        const std::string name = nameIt->second;
        _nameByPlayerId.erase(nameIt);
        RebuildNameIndex_Locked(name);
    }
}

// 네트워크용: SessionID로 세션 찾기
PlayerSessionRef GameSessionManager::FindBySessionId(uint64 sessionId)
{
    READ_LOCK;
    auto it = _bySessionId.find(sessionId);
    if (it == _bySessionId.end()) return nullptr;
    return it->second;
}

// 컨텐츠용: PlayerID로 세션 찾기 (귓속말, 파티 초대 등에 사용)
PlayerSessionRef GameSessionManager::FindByPlayerId(uint64 playerId)
{
    READ_LOCK;
    auto it = _byPlayerId.find(playerId);
    if (it == _byPlayerId.end()) return nullptr;
    return it->second;
}

// 전체 공지 패킷 전송 (브로드캐스팅)
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

// 역인덱스를 이용한 ID 조회 유틸리티
uint64 GameSessionManager::GetPlayerIdBySessionId(uint64 sessionId)
{
    READ_LOCK;
    auto it = _playerIdBySessionId.find(sessionId);
    if (it == _playerIdBySessionId.end())
        return 0;
    return it->second;
}

// 플레이어 이름 캐싱 (채팅 등에서 매번 DB 조회 안 하려고 씀)
void GameSessionManager::SetPlayerName(uint64 playerId, const std::string& name)
{
    if (playerId == 0) return;

    WRITE_LOCK;
    std::string prev;
    auto it = _nameByPlayerId.find(playerId);
    if (it != _nameByPlayerId.end())
        prev = it->second;

    if (name.empty())
    {
        if (it != _nameByPlayerId.end())
            _nameByPlayerId.erase(it);

        if (!prev.empty())
            RebuildNameIndex_Locked(prev);
        return;
    }

    _nameByPlayerId[playerId] = name;

    if (!prev.empty() && prev != name)
        RebuildNameIndex_Locked(prev);

    RebuildNameIndex_Locked(name);
}

std::string GameSessionManager::GetPlayerName(uint64 playerId)
{
    READ_LOCK;
    auto it = _nameByPlayerId.find(playerId);
    if (it == _nameByPlayerId.end())
        return std::string();
    return it->second;
}

bool GameSessionManager::TryGetPlayerIdByName(const std::string& name, uint64& outPlayerId, bool& ambiguous)
{
    outPlayerId = 0;
    ambiguous = false;

    if (name.empty())
        return false;

    READ_LOCK;
    if (_ambiguousNames.find(name) != _ambiguousNames.end())
    {
        ambiguous = true;
        return false;
    }

    auto it = _playerIdByName.find(name);
    if (it == _playerIdByName.end())
        return false;

    outPlayerId = it->second;
    return true;
}

void GameSessionManager::RebuildNameIndex_Locked(const std::string& name)
{
    if (name.empty()) return;

    uint64 foundId = 0;
    int count = 0;

    for (auto& kv : _nameByPlayerId)
    {
        if (kv.second == name)
        {
            count++;
            foundId = kv.first;
            if (count > 1) break;
        }
    }

    if (count == 0)
    {
        _playerIdByName.erase(name);
        _ambiguousNames.erase(name);
    }
    else if (count == 1)
    {
        _playerIdByName[name] = foundId;
        _ambiguousNames.erase(name);
    }
    else
    {
        _playerIdByName.erase(name);
        _ambiguousNames.insert(name);
    }
}
