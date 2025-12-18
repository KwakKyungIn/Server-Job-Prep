#pragma once

class PartyManager
{
public:
    struct Party
    {
        uint64 partyId = 0;
        uint64 leaderId = 0;
        uint32 version = 0; // 단일 GS라도 UI Sync/디버그에 유용
        std::unordered_set<uint64> members;
    };

    static PartyManager& Instance()
    {
        static PartyManager inst;
        return inst;
    }

public:
    // 스냅샷 통째로 반영 (테스트/GM 커맨드/초기화에 좋음)
    void Upsert(uint64 partyId, uint64 leaderId, uint32 version, const std::vector<uint64>& memberIds);

    // 자주 쓰는 조회
    bool   IsMember(uint64 partyId, uint64 playerId) const;
    Party  GetSnapshot(uint64 partyId) const;

    // 라우팅 핵심: playerId -> partyId
    uint64 GetPartyIdByPlayerId(uint64 playerId) const;

    // 멤버 리스트를 vector로 뽑아 쓰기 편하게
    void GetMembers(uint64 partyId, std::vector<uint64>& outMembers) const;

    // 최소 기능(있어야 나중에 던전/탈퇴 처리 안 꼬임)
    bool RemoveMember(uint64 partyId, uint64 playerId);
    bool Disband(uint64 partyId);

private:
    PartyManager() = default;

private:

    USE_LOCK;

    std::unordered_map<uint64, Party> _parties;       // partyId -> Party
    std::unordered_map<uint64, uint64> _playerToParty; // playerId -> partyId
};
