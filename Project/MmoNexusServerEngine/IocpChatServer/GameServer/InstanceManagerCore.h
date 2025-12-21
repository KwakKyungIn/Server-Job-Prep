#pragma once
#include <unordered_map>
#include <unordered_set>
#include <vector>

class InstanceManagerCore
{
public:
    struct InstanceInfo
    {
        int64 instanceId = 0;
        int32 channelId = 0;
        int32 mapId = 0;
        uint64 partyId = 0;
        std::unordered_set<uint64> members;
        uint64 createdTick = 0;
    };

public:
    bool GetInstanceByParty(uint64 partyId, InstanceInfo& out) const;

    bool CreateOrGetForParty(uint64 partyId, int32 channelId, int32 mapId,
        const std::vector<uint64>& members, InstanceInfo& out);

    // ✅ 종료(Exit) 시: party -> instance 매핑 제거 + instance 삭제
    bool CloseForParty(uint64 partyId, int64& outClosedInstanceId);

private:
    int64 _nextInstanceId = 1;
    std::unordered_map<uint64, int64> _partyToInstance;
    std::unordered_map<int64, InstanceInfo> _instances;
};
