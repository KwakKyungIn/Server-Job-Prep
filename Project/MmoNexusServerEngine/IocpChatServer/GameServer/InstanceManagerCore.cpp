#include "pch.h"
#include "InstanceManagerCore.h"

bool InstanceManagerCore::GetInstanceByParty(uint64 partyId, InstanceInfo& out) const
{
    auto it = _partyToInstance.find(partyId);
    if (it == _partyToInstance.end()) return false;

    auto it2 = _instances.find(it->second);
    if (it2 == _instances.end()) return false;

    out = it2->second;
    return true;
}

int64 InstanceManagerCore::GenerateInstanceId()
{
    const uint64 now = ::GetTickCount64();
    const uint16_t seq = ++_seq;
    const uint64 id = (now << 16) | (uint64)(seq);
    return (int64)id;
}

bool InstanceManagerCore::GetInstanceById(int64 instanceId, InstanceInfo& out) const
{
    auto it = _instances.find(instanceId);
    if (it == _instances.end()) return false;
    out = it->second;
    return true;
}


bool InstanceManagerCore::CreateOrGetForParty(uint64 partyId, int32 channelId, int32 mapId,
    const std::vector<uint64>& members, InstanceInfo& out)
{
    out = InstanceInfo{};
    if (partyId == 0) return false;

    InstanceInfo exist;
    if (GetInstanceByParty(partyId, exist))
    {
        out = exist;
        return true;
    }

    InstanceInfo inst;
    inst.instanceId = GenerateInstanceId();
    inst.channelId = channelId;
    inst.mapId = mapId;
    inst.partyId = partyId;
    inst.createdTick = ::GetTickCount64();
    inst.members.insert(members.begin(), members.end());

    _partyToInstance[partyId] = inst.instanceId;
    _instances[inst.instanceId] = inst;

    for (uint64 pid : members)
        _playerToInstance[pid] = inst.instanceId;

    out = inst;
    return true;
}

bool InstanceManagerCore::CloseForParty(uint64 partyId, InstanceInfo& outClosed)
{
    outClosed = InstanceInfo{};
    auto it = _partyToInstance.find(partyId);
    if (it == _partyToInstance.end()) return false;

    const int64 instanceId = it->second;
  

    auto it2 = _instances.find(instanceId);
    if (it2 == _instances.end()) return false;

    outClosed = it2->second;

    _partyToInstance.erase(it);
    // 역인덱스 정리
    for (uint64 pid : outClosed.members)
    {
        auto p = _playerToInstance.find(pid);
        if (p != _playerToInstance.end() && p->second == instanceId)
            _playerToInstance.erase(p);
    }

    _instances.erase(it2);
    return true;
}

bool InstanceManagerCore::EjectMember(int64 instanceId, uint64 playerId, bool& outInstanceEmpty)
{
    outInstanceEmpty = false;

    auto it = _instances.find(instanceId);
    if (it == _instances.end()) return false;

    InstanceInfo& inst = it->second;
    if (inst.members.erase(playerId) == 0)
        return false;

    auto p = _playerToInstance.find(playerId);
    if (p != _playerToInstance.end() && p->second == instanceId)
        _playerToInstance.erase(p);

    inst.lastActiveTick = ::GetTickCount64();

    if (inst.members.empty())
        outInstanceEmpty = true;

    return true;
}

bool InstanceManagerCore::OnMemberOffline(uint64 playerId, InstanceInfo& outClosedIfEmpty)
{
    outClosedIfEmpty = InstanceInfo{};

    auto it = _playerToInstance.find(playerId);
    if (it == _playerToInstance.end()) return false;

    const int64 instanceId = it->second;

    bool empty = false;
    if (!EjectMember(instanceId, playerId, empty))
        return false;

    if (empty)
    {
        // ✅ 마지막 멤버가 빠졌으면 자동 Close
        InstanceInfo closed;
        if (GetInstanceById(instanceId, closed))
        {
            InstanceInfo outClosed;
            CloseForParty(closed.partyId, outClosed);
            outClosedIfEmpty = outClosed;
        }
    }

    return true;
}

void InstanceManagerCore::CollectExpired(uint64 nowMs, std::vector<InstanceInfo>& outToClose) const
{
    outToClose.clear();

    for (auto& kv : _instances)
    {
        const InstanceInfo& inst = kv.second;
        if (inst.closing) continue;

        //  “30분 초과 시 종료” (요구사항)
        if (nowMs - inst.createdTick >= kInstanceTimeoutMs)
            outToClose.push_back(inst);
    }
}
bool InstanceManagerCore::CloseByInstanceId(int64 instanceId, InstanceInfo& outClosed)
{
    outClosed = InstanceInfo{};

    auto it = _instances.find(instanceId);
    if (it == _instances.end()) return false;

    outClosed = it->second;

    // party -> instance 매핑 제거
    auto pit = _partyToInstance.find(outClosed.partyId);
    if (pit != _partyToInstance.end() && pit->second == instanceId)
        _partyToInstance.erase(pit);

    // reverse index 정리 (중요)
    for (uint64 pid : outClosed.members)
    {
        auto p = _playerToInstance.find(pid);
        if (p != _playerToInstance.end() && p->second == instanceId)
            _playerToInstance.erase(p);
    }

    _instances.erase(it);
    return true;
}
