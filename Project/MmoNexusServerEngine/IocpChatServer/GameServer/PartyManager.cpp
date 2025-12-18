#include "pch.h"
#include "PartyManager.h"

uint64 PartyManager::GetPartyIdByPlayerId(uint64 playerId) const
{
    READ_LOCK;
    auto it = _playerToParty.find(playerId);
    return (it == _playerToParty.end()) ? 0 : it->second;
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

bool PartyManager::Create(uint64 leaderId, uint64& outPartyId)
{
    outPartyId = 0;
    if (leaderId == 0) return false;

    WRITE_LOCK;

    if (_playerToParty.find(leaderId) != _playerToParty.end())
        return false; // 이미 파티 있음

    const uint64 partyId = _nextPartyId++;
    Party p;
    p.partyId = partyId;
    p.leaderId = leaderId;
    p.version = 1;
    p.members.insert(leaderId);

    _parties[partyId] = std::move(p);
    _playerToParty[leaderId] = partyId;

    outPartyId = partyId;
    return true;
}

bool PartyManager::Invite(uint64 inviterId, uint64 targetId, PendingInvite& outInvite)
{
    outInvite = PendingInvite{};
    if (inviterId == 0 || targetId == 0) return false;
    if (inviterId == targetId) return false;

    WRITE_LOCK;

    uint64 partyId = 0;
    {
        auto it = _playerToParty.find(inviterId);
        if (it == _playerToParty.end()) return false;
        partyId = it->second;
    }

    // 대상이 이미 파티 있으면 실패
    if (_playerToParty.find(targetId) != _playerToParty.end())
        return false;

    // 초대 중복 방지(타겟 기준 1개만)
    _pendingByTarget.erase(targetId);

    PendingInvite inv;
    inv.partyId = partyId;
    inv.inviterId = inviterId;
    inv.targetId = targetId;
    inv.expireTick = ::GetTickCount64() + 60'000; // 60초

    _pendingByTarget[targetId] = inv;
    outInvite = inv;
    return true;
}

bool PartyManager::AcceptInvite(uint64 targetId, uint64 partyId, bool accept, Party& outPartyAfter)
{
    outPartyAfter = Party{};
    if (targetId == 0 || partyId == 0) return false;

    WRITE_LOCK;

    auto pit = _pendingByTarget.find(targetId);
    if (pit == _pendingByTarget.end()) return false;

    PendingInvite inv = pit->second;
    if (inv.partyId != partyId) return false;
    if (::GetTickCount64() > inv.expireTick) { _pendingByTarget.erase(pit); return false; }

    // 거절이면 그냥 삭제
    if (!accept)
    {
        _pendingByTarget.erase(pit);
        return true;
    }

    // 이미 파티 생겼으면 실패
    if (_playerToParty.find(targetId) != _playerToParty.end())
    {
        _pendingByTarget.erase(pit);
        return false;
    }

    auto it = _parties.find(partyId);
    if (it == _parties.end()) { _pendingByTarget.erase(pit); return false; }

    it->second.members.insert(targetId);
    it->second.version++;

    _playerToParty[targetId] = partyId;

    _pendingByTarget.erase(pit);

    outPartyAfter = it->second;
    return true;
}

bool PartyManager::Leave(uint64 playerId, Party& outPartyAfter, bool& outDisbanded)
{
    outPartyAfter = Party{};
    outDisbanded = false;
    if (playerId == 0) return false;

    WRITE_LOCK;

    auto pit = _playerToParty.find(playerId);
    if (pit == _playerToParty.end()) return false;

    uint64 partyId = pit->second;

    auto it = _parties.find(partyId);
    if (it == _parties.end()) { _playerToParty.erase(pit); return false; }

    it->second.members.erase(playerId);
    _playerToParty.erase(pit);

    // 리더가 나가면 새 리더 선출(최소 id)
    if (it->second.leaderId == playerId && !it->second.members.empty())
    {
        uint64 newLeader = *it->second.members.begin();
        for (uint64 m : it->second.members) newLeader = min(newLeader, m);
        it->second.leaderId = newLeader;
    }

    it->second.version++;

    // 멤버가 0이면 해산
    if (it->second.members.empty())
    {
        _parties.erase(it);
        outDisbanded = true;
        return true;
    }

    outPartyAfter = it->second;
    return true;
}

bool PartyManager::Kick(uint64 leaderId, uint64 targetId, Party& outPartyAfter)
{
    outPartyAfter = Party{};
    if (leaderId == 0 || targetId == 0) return false;

    WRITE_LOCK;

    auto lp = _playerToParty.find(leaderId);
    if (lp == _playerToParty.end()) return false;

    uint64 partyId = lp->second;

    auto it = _parties.find(partyId);
    if (it == _parties.end()) return false;

    if (it->second.leaderId != leaderId) return false; // 리더만 킥
    if (it->second.members.find(targetId) == it->second.members.end()) return false;

    it->second.members.erase(targetId);
    it->second.version++;

    auto tp = _playerToParty.find(targetId);
    if (tp != _playerToParty.end() && tp->second == partyId)
        _playerToParty.erase(tp);

    outPartyAfter = it->second;
    return true;
}

bool PartyManager::Disband(uint64 leaderId, Party& outDisbandedParty)
{
    outDisbandedParty = Party{};
    if (leaderId == 0) return false;

    WRITE_LOCK;

    auto lp = _playerToParty.find(leaderId);
    if (lp == _playerToParty.end()) return false;

    uint64 partyId = lp->second;

    auto it = _parties.find(partyId);
    if (it == _parties.end()) return false;

    if (it->second.leaderId != leaderId) return false;

    outDisbandedParty = it->second;

    // 역인덱스 제거
    for (uint64 id : it->second.members)
    {
        auto p = _playerToParty.find(id);
        if (p != _playerToParty.end() && p->second == partyId)
            _playerToParty.erase(p);
    }

    _parties.erase(it);
    return true;
}
