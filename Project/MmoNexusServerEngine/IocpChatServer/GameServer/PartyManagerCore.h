#pragma once

class PartyManagerCore
{
public:
    enum class DungeonState : uint8_t
    {
        NONE = 0,
        ENTERING = 1,
        IN_DUNGEON = 2,
        EXITING = 3,
    };

    struct Party
    {
        uint64 partyId = 0;
        uint64 leaderId = 0;
        uint32 version = 0;
        std::unordered_set<uint64> members;

           //  던전 메타
        bool inDungeonTransition = false;     // ENTERING/EXITING 동안 true
        int64 instanceId = 0;                 // 0=필드, >0=던전 인스턴스
        DungeonState dungeonState = DungeonState::NONE;
    };

    struct PendingInvite
    {
        uint64 partyId = 0;
        uint64 inviterId = 0;
        uint64 targetId = 0;
        uint64 expireTick = 0; // GetTickCount64 기반
    };

public:
    // ===== Query (Actor thread ONLY) =====
    uint64 GetPartyIdByPlayerId(uint64 playerId) const;
    bool   IsMember(uint64 partyId, uint64 playerId) const;
    Party  GetSnapshot(uint64 partyId) const;
    void   GetMembers(uint64 partyId, std::vector<uint64>& outMembers) const;

    //  던전 상태 조회
    bool GetDungeonInfo(uint64 partyId, int64& outInstanceId, DungeonState& outState, bool& outTransition) const;
public:
    // ===== Ops (Actor thread ONLY) =====
    bool Create(uint64 leaderId, uint64& outPartyId);
    bool Invite(uint64 inviterId, uint64 targetId, PendingInvite& outInvite);
    bool AcceptInvite(uint64 targetId, uint64 partyId, bool accept, Party& outPartyAfter);
    bool Leave(uint64 playerId, Party& outPartyAfter, bool& outDisbanded);
    bool Kick(uint64 leaderId, uint64 targetId, Party& outPartyAfter);
    bool Disband(uint64 leaderId, Party& outDisbandedParty);

    //  transition / instance 메타 제어 (PartyActor에서만 호출)
    bool TryBeginDungeonTransition(uint64 partyId, DungeonState nextState);
    bool EndDungeonTransition(uint64 partyId, DungeonState finalState);
    bool SetPartyInstance(uint64 partyId, int64 instanceId, DungeonState state);
    bool ClearPartyInstance(uint64 partyId);

    void MarkForceReturn(uint64 playerId);
    bool ConsumeForceReturn(uint64 playerId);
private:
    uint64 _nextPartyId = 1;

    std::unordered_map<uint64, Party> _parties;                 // partyId -> Party
    std::unordered_map<uint64, uint64> _playerToParty;          // playerId -> partyId
    std::unordered_map<uint64, PendingInvite> _pendingByTarget; // targetId -> invite

    std::unordered_set<uint64> _forceReturn;
};
