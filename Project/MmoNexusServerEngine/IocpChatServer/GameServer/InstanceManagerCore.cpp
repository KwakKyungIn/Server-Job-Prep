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
    inst.instanceId = _nextInstanceId++;
    inst.channelId = channelId;
    inst.mapId = mapId;
    inst.partyId = partyId;
    inst.createdTick = ::GetTickCount64();
    inst.members.insert(members.begin(), members.end());

    _partyToInstance[partyId] = inst.instanceId;
    _instances[inst.instanceId] = inst;

    out = inst;
    return true;
}

bool InstanceManagerCore::CloseForParty(uint64 partyId, int64& outClosedInstanceId)
{
    outClosedInstanceId = 0;

    auto it = _partyToInstance.find(partyId);
    if (it == _partyToInstance.end()) return false;

    const int64 instanceId = it->second;
    _partyToInstance.erase(it);

    auto it2 = _instances.find(instanceId);
    if (it2 != _instances.end())
        _instances.erase(it2);

    outClosedInstanceId = instanceId;
    return true;
}
