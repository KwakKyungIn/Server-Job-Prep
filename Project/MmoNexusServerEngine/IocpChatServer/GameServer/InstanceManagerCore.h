#pragma once
#include <unordered_map>
#include <unordered_set>
#include <vector>

// 인스턴스 던전의 논리적 데이터를 관리하는 코어 클래스
// Actor 내부에서만 쓰이며 순수 데이터 조작만 담당함 (스레드 처리 X)
class InstanceManagerCore
{
public:
    // 인스턴스 하나에 대한 메타 데이터
    struct InstanceInfo
    {
        int64 instanceId = 0;
        int32 channelId = 0;
        int32 mapId = 0;
        uint64 partyId = 0;
        HashSet<uint64> members; // 현재 입장 중인 멤버들
        uint64 createdTick = 0;  // 생성 시간 (타임아웃 체크용)
        uint64 lastActiveTick = 0;
        bool closing = false;    // 종료 진행 중 플래그
    };

public:
    bool GetInstanceByParty(uint64 partyId, InstanceInfo& out) const;

    bool GetInstanceById(int64 instanceId, InstanceInfo& out) const;

    // 파티가 입장할 인스턴스를 찾거나 새로 생성
    bool CreateOrGetForParty(uint64 partyId, int32 channelId, int32 mapId,
        const Vector<uint64>& members, InstanceInfo& out);

    // 파티 해산이나 던전 클리어 시 호출하여 정리
    bool CloseForParty(uint64 partyId, InstanceInfo& outClosed);

    // 특정 멤버만 강퇴하거나 나갈 때
    bool EjectMember(int64 instanceId, uint64 playerId, bool& outInstanceEmpty);

    // 유저가 접속 끊었을 때 처리 (던전에서 튕겨내기 등)
    bool OnMemberOffline(uint64 playerId, InstanceInfo& outClosedIfEmpty);

    // 30분 제한 시간 지난 인스턴스 수집
    void CollectExpired(uint64 nowMs, Vector<InstanceInfo>& outToClose) const;

    static constexpr uint64 kInstanceTimeoutMs = 30ull * 60ull * 1000ull; // 제한시간 30분

    // ID로 강제 종료
    bool CloseByInstanceId(int64 instanceId, InstanceInfo& outClosed);
private:
    int64 GenerateInstanceId();

private:
    // ID 생성용 시퀀스. 시간값과 조합해서 유니크 ID 만듦
    uint16_t _seq = 0;

    // 빠른 조회를 위한 3종 해시맵
    // O(1) 접근을 위해 용도별로 인덱싱을 따로 둠
    HashMap<uint64, int64> _partyToInstance;   // 파티 ID -> 인스턴스 ID
    HashMap<int64, InstanceInfo> _instances;   // 인스턴스 ID -> 상세 정보
    HashMap<uint64, int64> _playerToInstance;  // 플레이어 ID -> 인스턴스 ID (역인덱스)
};