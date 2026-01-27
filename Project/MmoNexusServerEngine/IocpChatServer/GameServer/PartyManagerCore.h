#pragma once

// 파티 시스템의 순수 로직 및 데이터 관리 클래스
// 스레드 동기화(Lock)는 Actor Layer에서 처리하므로 여기선 데이터 무결성에만 집중함
class PartyManagerCore
{
public:
    // 던전 진입 단계 관리용 상태값
    enum class DungeonState : uint8_t
    {
        NONE = 0,         // 일반 필드
        ENTERING = 1,     // 입장 시도 중 (매칭 완료 등)
        IN_DUNGEON = 2,   // 던전 플레이 중
        EXITING = 3,      // 퇴장 중
    };

    struct Party
    {
        uint64 partyId = 0;
        uint64 leaderId = 0;
        uint32 version = 0; // 변경사항 추적용 버전
        HashSet<uint64> members; // 중복 방지 및 빠른 검색을 위해 Set 사용

        // 던전 메타데이터
        bool inDungeonTransition = false;     // true면 멤버 추가/제거 잠금
        int64 instanceId = 0;                 // 0=필드, >0=할당된 인스턴스 ID
        DungeonState dungeonState = DungeonState::NONE;
    };

    struct PendingInvite
    {
        uint64 partyId = 0;
        uint64 inviterId = 0;
        uint64 targetId = 0;
        uint64 expireTick = 0; // GetTickCount64 기준 만료 시간
    };

public:
    // ===== Query (Actor thread ONLY) =====
    // 단순 조회 함수들
    uint64 GetPartyIdByPlayerId(uint64 playerId) const;
    bool   IsMember(uint64 partyId, uint64 playerId) const;
    Party  GetSnapshot(uint64 partyId) const;
    void   GetMembers(uint64 partyId, Vector<uint64>& outMembers) const;

    // 던전 관련 상태 조회
    bool GetDungeonInfo(uint64 partyId, int64& outInstanceId, DungeonState& outState, bool& outTransition) const;
public:
    // ===== Ops (Actor thread ONLY) =====
    // 상태 변경 함수들 (성공/실패 반환)
    bool Create(uint64 leaderId, uint64& outPartyId);
    bool Invite(uint64 inviterId, uint64 targetId, PendingInvite& outInvite);
    bool AcceptInvite(uint64 targetId, uint64 partyId, bool accept, Party& outPartyAfter);
    bool Leave(uint64 playerId, Party& outPartyAfter, bool& outDisbanded);
    bool Kick(uint64 leaderId, uint64 targetId, Party& outPartyAfter);
    bool Disband(uint64 leaderId, Party& outDisbandedParty);

    // 트랜지션 / 인스턴스 메타 제어 (PartyActor의 오케스트레이션 로직에서만 호출)
    bool TryBeginDungeonTransition(uint64 partyId, DungeonState nextState);
    bool EndDungeonTransition(uint64 partyId, DungeonState finalState);
    bool SetPartyInstance(uint64 partyId, int64 instanceId, DungeonState state);
    bool ClearPartyInstance(uint64 partyId);

    // 귀환 처리가 필요한 유저 마킹
    void MarkForceReturn(uint64 playerId);
    bool ConsumeForceReturn(uint64 playerId);
private:
    uint64 _nextPartyId = 1;

    HashMap<uint64, Party> _parties;                 // Party ID로 검색
    HashMap<uint64, uint64> _playerToParty;          // Player ID로 검색 (역참조)
    HashMap<uint64, PendingInvite> _pendingByTarget; // 초대받은 사람 기준으로 초대장 관리

    HashSet<uint64> _forceReturn;
};