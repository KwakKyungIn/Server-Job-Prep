#pragma once
#include "Lock.h"
#include <string>
#include <unordered_map>

class PlayerSession;
using PlayerSessionRef = std::shared_ptr<PlayerSession>;

// 전체 접속 중인 플레이어 세션을 관리하는 싱글톤 클래스
// 네트워크 ID(SessionId)와 게임 로직 ID(PlayerId) 두 가지 키로
// 세션을 빠르게(O(1)) 찾을 수 있도록 이중 맵 구조를 사용함
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
    bool TryGetPlayerIdByName(const std::string& name, uint64& outPlayerId, bool& ambiguous);

private:
    USE_LOCK;

    HashMap<uint64, PlayerSessionRef> _bySessionId;   // SessionId를 키로 세션 관리 (네트워크 처리용)
    HashMap<uint64, PlayerSessionRef> _byPlayerId;    // PlayerId를 키로 세션 관리 (게임 로직용)

    // 핵심: 세션 종료 시 Player 객체 없이도 ID를 역추적하기 위한 맵
    // 이걸 안 쓰면 Disconnect 시점에 PlayerId를 알 방법이 없어서 맵 정리가 안 됨
    HashMap<uint64, uint64> _playerIdBySessionId;     // SessionId -> PlayerId 매핑

    HashMap<uint64, std::string> _nameByPlayerId;     // 닉네임 캐싱 (채팅/파티용)
    HashMap<std::string, uint64> _playerIdByName;     // 이름 -> PlayerId (중복이면 비움)
    HashSet<std::string> _ambiguousNames;             // 중복 이름

    void RebuildNameIndex_Locked(const std::string& name);
};
