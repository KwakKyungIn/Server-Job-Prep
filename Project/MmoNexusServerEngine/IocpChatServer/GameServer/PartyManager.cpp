#include "pch.h"
#include "PartyManager.h"

void PartyManager::Upsert(uint64 partyId, uint64 leaderId, uint32 version, const std::vector<uint64>& memberIds)
{
    WRITE_LOCK;

    // 기존 파티가 있으면, 기존 멤버들의 역인덱스 먼저 제거
    auto it = _parties.find(partyId);
    if (it != _parties.end())
    {
        for (uint64 oldId : it->second.members)
        {
            auto pit = _playerToParty.find(oldId);
            if (pit != _playerToParty.end() && pit->second == partyId)
                _playerToParty.erase(pit);
        }
    }

    Party& p = _parties[partyId];
    p.partyId = partyId;
    p.leaderId = leaderId;
    p.version = version;

    p.members.clear();
    p.members.reserve(memberIds.size());

    for (uint64 id : memberIds)
    {
        p.members.insert(id);
        _playerToParty[id] = partyId;
    }
}

bool PartyManager::IsMember(uint64 partyId, uint64 playerId) const
{
    READ_LOCK;
    auto it = _parties.find(partyId);
    if (it == _parties.end()) return false;
    return it->second.members.find(playerId) != it->second.members.end();
}

PartyManager::Party PartyManager::GetSnapshot(uint64 partyId) const
{
    READ_LOCK;
    auto it = _parties.find(partyId);
    if (it == _parties.end()) return Party{};
    return it->second; // 복사본 반환
}

uint64 PartyManager::GetPartyIdByPlayerId(uint64 playerId) const
{
    READ_LOCK;
    auto it = _playerToParty.find(playerId);
    if (it == _playerToParty.end()) return 0;
    return it->second;
}

void PartyManager::GetMembers(uint64 partyId, std::vector<uint64>& outMembers) const
{
    outMembers.clear();

    READ_LOCK;
    auto it = _parties.find(partyId);
    if (it == _parties.end()) return;

    outMembers.reserve(it->second.members.size());
    for (uint64 id : it->second.members)
        outMembers.push_back(id);
}

bool PartyManager::RemoveMember(uint64 partyId, uint64 playerId)
{
    WRITE_LOCK;

    auto it = _parties.find(partyId);
    if (it == _parties.end()) return false;

    auto mit = it->second.members.find(playerId);
    if (mit == it->second.members.end()) return false;

    it->second.members.erase(mit);

    auto pit = _playerToParty.find(playerId);
    if (pit != _playerToParty.end() && pit->second == partyId)
        _playerToParty.erase(pit);

    // 멤버 0이면 파티 해산 처리(선택)
    if (it->second.members.empty())
        _parties.erase(it);

    return true;
}

bool PartyManager::Disband(uint64 partyId)
{
    WRITE_LOCK;

    auto it = _parties.find(partyId);
    if (it == _parties.end()) return false;

    for (uint64 id : it->second.members)
    {
        auto pit = _playerToParty.find(id);
        if (pit != _playerToParty.end() && pit->second == partyId)
            _playerToParty.erase(pit);
    }

    _parties.erase(it);
    return true;
}
