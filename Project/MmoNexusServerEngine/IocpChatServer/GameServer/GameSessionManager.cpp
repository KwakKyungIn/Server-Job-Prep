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
            _nameByPlayerId.erase(playerId); // 이름 캐시도 정리
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
    _nameByPlayerId.erase(playerId);
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
    _nameByPlayerId[playerId] = name;
}

std::string GameSessionManager::GetPlayerName(uint64 playerId)
{
    READ_LOCK;
    auto it = _nameByPlayerId.find(playerId);
    if (it == _nameByPlayerId.end())
        return std::string();
    return it->second;
}