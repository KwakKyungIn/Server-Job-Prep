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
        HashSet<uint64> members;
        uint64 createdTick = 0;
        uint64 lastActiveTick = 0;
        bool closing = false;
    };

public:
    bool GetInstanceByParty(uint64 partyId, InstanceInfo& out) const;

    bool GetInstanceById(int64 instanceId, InstanceInfo& out) const;

    bool CreateOrGetForParty(uint64 partyId, int32 channelId, int32 mapId,
        const Vector<uint64>& members, InstanceInfo& out);

    //  party 해산/전원 leave 시 호출
    bool CloseForParty(uint64 partyId, InstanceInfo& outClosed);

    //  던전 내 파티 탈퇴/킥 시 강제 퇴출용
    bool EjectMember(int64 instanceId, uint64 playerId, bool& outInstanceEmpty);

    //  오프라인 강제 복귀 정책(던전에서 제거)
    bool OnMemberOffline(uint64 playerId, InstanceInfo& outClosedIfEmpty);

    //  30분 타임아웃: 만료 인스턴스들을 outToClose로 뽑아준다
    void CollectExpired(uint64 nowMs, Vector<InstanceInfo>& outToClose) const;

    static constexpr uint64 kInstanceTimeoutMs = 30ull * 60ull * 1000ull; // 30분

    //  instanceId로 닫기 (partyToInstance도 같이 정리)
    bool CloseByInstanceId(int64 instanceId, InstanceInfo& outClosed);
private:
    int64 GenerateInstanceId();

private:
    //  “재사용 방지”: 시간(ms)<<16 | seq
    uint16_t _seq = 0;

    HashMap<uint64, int64> _partyToInstance;   // partyId -> instanceId
    HashMap<int64, InstanceInfo> _instances;   // instanceId -> info
    HashMap<uint64, int64> _playerToInstance;  // playerId -> instanceId
};
