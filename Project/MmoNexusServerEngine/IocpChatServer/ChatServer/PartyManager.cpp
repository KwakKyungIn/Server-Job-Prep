#include "pch.h"
#include "PartyManager.h"

void PartyManager::Upsert(uint64 partyId, uint64 leaderId, uint32 version, const std::vector<uint64>& memberIds)
{
    std::lock_guard<std::mutex> guard(_lock);

    Party& p = _parties[partyId];
    p.partyId = partyId;
    p.leaderId = leaderId;
    p.version = version;

    p.members.clear();
    for (uint64 id : memberIds)
        p.members.insert(id);
}

bool PartyManager::IsMember(uint64 partyId, uint64 playerId) const
{
    std::lock_guard<std::mutex> guard(_lock);
    auto it = _parties.find(partyId);
    if (it == _parties.end()) return false;

    return it->second.members.find(playerId) != it->second.members.end();
}

PartyManager::Party PartyManager::GetSnapshot(uint64 partyId) const
{
    std::lock_guard<std::mutex> guard(_lock);
    auto it = _parties.find(partyId);
    if (it == _parties.end()) return Party{};
    return it->second;
}
