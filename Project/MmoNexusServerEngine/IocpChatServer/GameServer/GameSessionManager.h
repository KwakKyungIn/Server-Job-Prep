#pragma once
#include <unordered_map>
#include <memory>

class PlayerSession;
using PlayerSessionRef = std::shared_ptr<PlayerSession>;

class GameSessionManager
{
public:
    static GameSessionManager* GSessionManager;

    // 접속/해제
    void Add(PlayerSessionRef session);      // sessionId 등록
    void Remove(PlayerSessionRef session);   // sessionId, playerId 둘 다 해제

    // 전체 브로드캐스트(월드 공지 같은 용도)
    void Broadcast(SendBufferRef sendBuffer);

    // 조회
    PlayerSessionRef FindBySessionId(uint64 sessionId); // DB 응답 gameSessionId로 찾을 때
    PlayerSessionRef FindByPlayerId(uint64 playerId);   // 파티/귓속말/인스턴스 라우팅

    // 바인딩: EnterGame 성공 후 playerId가 결정되면 호출
    void BindPlayerId(PlayerSessionRef session, uint64 playerId);
    void UnbindPlayerId(uint64 playerId);

    // 기존 호환(기존 코드가 Find를 썼다면 playerId 기준으로 의미를 고정)
    PlayerSessionRef Find(uint64 playerId) { return FindByPlayerId(playerId); }

private:
    USE_LOCK;

    std::unordered_map<uint64, PlayerSessionRef> _bySessionId;
    std::unordered_map<uint64, PlayerSessionRef> _byPlayerId;
};

extern GameSessionManager* GSessionManager;
