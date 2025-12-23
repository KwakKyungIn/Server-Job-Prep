#include "pch.h"
#include "PartyManagerCore.h"

uint64 PartyManagerCore::GetPartyIdByPlayerId(uint64 playerId) const
{
    auto it = _playerToParty.find(playerId);
    return (it == _playerToParty.end()) ? 0 : it->second;
}

bool PartyManagerCore::IsMember(uint64 partyId, uint64 playerId) const
{
    auto it = _parties.find(partyId);
    if (it == _parties.end()) return false;
    return it->second.members.find(playerId) != it->second.members.end();
}

PartyManagerCore::Party PartyManagerCore::GetSnapshot(uint64 partyId) const
{
    auto it = _parties.find(partyId);
    if (it == _parties.end()) return Party{};
    return it->second;
}

void PartyManagerCore::GetMembers(uint64 partyId, std::vector<uint64>& outMembers) const
{
    outMembers.clear();

    auto it = _parties.find(partyId);
    if (it == _parties.end()) return;

    outMembers.reserve(it->second.members.size());
    for (uint64 id : it->second.members)
        outMembers.push_back(id);
}

bool PartyManagerCore::Create(uint64 leaderId, uint64& outPartyId)
{
    outPartyId = 0;
    if (leaderId == 0) return false;

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

bool PartyManagerCore::Invite(uint64 inviterId, uint64 targetId, PendingInvite& outInvite)
{
    outInvite = PendingInvite{};
    if (inviterId == 0 || targetId == 0) return false;
    if (inviterId == targetId) return false;

    uint64 partyId = 0;
    {
        auto it = _playerToParty.find(inviterId);
        if (it == _playerToParty.end()) return false;
        partyId = it->second;
    }

    // 파티 상태 확인
    auto pit = _parties.find(partyId);
    if (pit == _parties.end()) return false;

    // ENTERING/EXITING 중 초대 금지
    if (pit->second.inDungeonTransition) return false;

    // 던전(인스턴스) 안에서는 초대 금지 (멤버십 꼬임 방지)
    if (pit->second.instanceId != 0) return false;

    // 대상이 이미 파티 있으면 실패
    if (_playerToParty.find(targetId) != _playerToParty.end())
        return false;

    // 초대 중복 방지(타겟 기준 1개만)
    _pendingByTarget.erase(targetId);

    PendingInvite inv;
    inv.partyId = partyId;
    inv.inviterId = inviterId;
    inv.targetId = targetId;
    inv.expireTick = ::GetTickCount64() + 60'000;

    _pendingByTarget[targetId] = inv;
    outInvite = inv;
    return true;
}

bool PartyManagerCore::AcceptInvite(uint64 targetId, uint64 partyId, bool accept, Party& outPartyAfter)
{
    outPartyAfter = Party{};
    if (targetId == 0 || partyId == 0) return false;

    auto pit = _pendingByTarget.find(targetId);
    if (pit == _pendingByTarget.end()) return false;

    PendingInvite inv = pit->second;
    if (inv.partyId != partyId) return false;
    if (::GetTickCount64() > inv.expireTick) { _pendingByTarget.erase(pit); return false; }

    if (!accept)
    {
        _pendingByTarget.erase(pit);
        return true;
    }

    if (_playerToParty.find(targetId) != _playerToParty.end())
    {
        _pendingByTarget.erase(pit);
        return false;
    }

    auto it = _parties.find(partyId);
    if (it == _parties.end()) { _pendingByTarget.erase(pit); return false; }

    //ENTERING/EXITING 중 합류 금지
    if (it->second.inDungeonTransition) { _pendingByTarget.erase(pit); return false; }

    // 던전 중 합류 금지
    if (it->second.instanceId != 0) { _pendingByTarget.erase(pit); return false; }

    it->second.members.insert(targetId);
    it->second.version++;

    _playerToParty[targetId] = partyId;
    _pendingByTarget.erase(pit);

    outPartyAfter = it->second;
    return true;
}

bool PartyManagerCore::Leave(uint64 playerId, Party& outPartyAfter, bool& outDisbanded)
{
    outPartyAfter = Party{};
    outDisbanded = false;
    if (playerId == 0) return false;

    auto pit = _playerToParty.find(playerId);
    if (pit == _playerToParty.end()) return false;

    uint64 partyId = pit->second;

    auto it = _parties.find(partyId);

    if (it->second.inDungeonTransition) return false;

    if (it == _parties.end()) { _playerToParty.erase(pit); return false; }

    it->second.members.erase(playerId);
    _playerToParty.erase(pit);

    if (it->second.leaderId == playerId && !it->second.members.empty())
    {
        uint64 newLeader = *it->second.members.begin();
        for (uint64 m : it->second.members) newLeader = min(newLeader, m);
        it->second.leaderId = newLeader;
    }

    it->second.version++;

    if (it->second.members.empty())
    {
        _parties.erase(it);
        outDisbanded = true;
        return true;
    }

    outPartyAfter = it->second;
    return true;
}

bool PartyManagerCore::Kick(uint64 leaderId, uint64 targetId, Party& outPartyAfter)
{
    outPartyAfter = Party{};
    if (leaderId == 0 || targetId == 0) return false;

    auto lp = _playerToParty.find(leaderId);
    if (lp == _playerToParty.end()) return false;

    uint64 partyId = lp->second;

    auto it = _parties.find(partyId);

    if (it->second.inDungeonTransition) return false;

    if (it == _parties.end()) return false;

    if (it->second.leaderId != leaderId) return false;
    if (it->second.members.find(targetId) == it->second.members.end()) return false;

    it->second.members.erase(targetId);
    it->second.version++;

    auto tp = _playerToParty.find(targetId);
    if (tp != _playerToParty.end() && tp->second == partyId)
        _playerToParty.erase(tp);

    outPartyAfter = it->second;
    return true;
}

bool PartyManagerCore::Disband(uint64 leaderId, Party& outDisbandedParty)
{
    outDisbandedParty = Party{};
    if (leaderId == 0) return false;

    auto lp = _playerToParty.find(leaderId);
    if (lp == _playerToParty.end()) return false;

    uint64 partyId = lp->second;

    auto it = _parties.find(partyId);

    if (it->second.inDungeonTransition) return false;

    if (it == _parties.end()) return false;

    if (it->second.leaderId != leaderId) return false;

    outDisbandedParty = it->second;

    for (uint64 id : it->second.members)
    {
        auto p = _playerToParty.find(id);
        if (p != _playerToParty.end() && p->second == partyId)
            _playerToParty.erase(p);
    }

    _parties.erase(it);
    return true;
}

bool PartyManagerCore::GetDungeonInfo(uint64 partyId, int64& outInstanceId, DungeonState& outState, bool& outTransition) const
{
    outInstanceId = 0;
    outState = DungeonState::NONE;
    outTransition = false;

    auto it = _parties.find(partyId);
    if (it == _parties.end()) return false;

    outInstanceId = it->second.instanceId;
    outState = it->second.dungeonState;
    outTransition = it->second.inDungeonTransition;
    return true;
}

bool PartyManagerCore::TryBeginDungeonTransition(uint64 partyId, DungeonState nextState)
{
    auto it = _parties.find(partyId);
    if (it == _parties.end()) return false;

    Party& p = it->second;
    if (p.inDungeonTransition) return false;

    p.inDungeonTransition = true;
    p.dungeonState = nextState;
    p.version++;
    return true;
}

bool PartyManagerCore::EndDungeonTransition(uint64 partyId, DungeonState finalState)
{
    auto it = _parties.find(partyId);
    if (it == _parties.end()) return false;

    Party& p = it->second;
    p.inDungeonTransition = false;
    p.dungeonState = finalState;
    p.version++;
    return true;
}

bool PartyManagerCore::SetPartyInstance(uint64 partyId, int64 instanceId, DungeonState state)
{
    auto it = _parties.find(partyId);
    if (it == _parties.end()) return false;

    Party& p = it->second;
    p.instanceId = instanceId;
    p.dungeonState = state;
    p.version++;
    return true;
}

bool PartyManagerCore::ClearPartyInstance(uint64 partyId)
{
    auto it = _parties.find(partyId);
    if (it == _parties.end()) return false;

    Party& p = it->second;
    p.instanceId = 0;
    p.dungeonState = DungeonState::NONE;
    p.inDungeonTransition = false;
    p.version++;
    return true;
}
