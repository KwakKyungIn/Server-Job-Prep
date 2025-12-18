#pragma once
#include "pch.h"

class PartyManager
{
public:
    struct Party
    {
        uint64 partyId = 0;
        uint64 leaderId = 0;
        uint32 version = 0;
        std::unordered_set<uint64> members;
    };

    static PartyManager& Instance()
    {
        static PartyManager inst;
        return inst;
    }

    void Upsert(uint64 partyId, uint64 leaderId, uint32 version, const std::vector<uint64>& memberIds);
    bool IsMember(uint64 partyId, uint64 playerId) const;
    Party GetSnapshot(uint64 partyId) const;

private:
    mutable std::mutex _lock;
    std::unordered_map<uint64, Party> _parties;
};
