#pragma once

class PartyManagerCore
{
public:
    struct Party
    {
        uint64 partyId = 0;
        uint64 leaderId = 0;
        uint32 version = 0;
        std::unordered_set<uint64> members;
    };

    struct PendingInvite
    {
        uint64 partyId = 0;
        uint64 inviterId = 0;
        uint64 targetId = 0;
        uint64 expireTick = 0; // GetTickCount64 ±â¹Ý
    };

public:
    // ===== Query (Actor thread ONLY) =====
    uint64 GetPartyIdByPlayerId(uint64 playerId) const;
    bool   IsMember(uint64 partyId, uint64 playerId) const;
    Party  GetSnapshot(uint64 partyId) const;
    void   GetMembers(uint64 partyId, std::vector<uint64>& outMembers) const;

public:
    // ===== Ops (Actor thread ONLY) =====
    bool Create(uint64 leaderId, uint64& outPartyId);
    bool Invite(uint64 inviterId, uint64 targetId, PendingInvite& outInvite);
    bool AcceptInvite(uint64 targetId, uint64 partyId, bool accept, Party& outPartyAfter);
    bool Leave(uint64 playerId, Party& outPartyAfter, bool& outDisbanded);
    bool Kick(uint64 leaderId, uint64 targetId, Party& outPartyAfter);
    bool Disband(uint64 leaderId, Party& outDisbandedParty);

private:
    uint64 _nextPartyId = 1;

    std::unordered_map<uint64, Party> _parties;                 // partyId -> Party
    std::unordered_map<uint64, uint64> _playerToParty;          // playerId -> partyId
    std::unordered_map<uint64, PendingInvite> _pendingByTarget; // targetId -> invite
};
