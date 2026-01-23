#include "pch.h"
#include "InstanceManagerCore.h"

// 파티 ID로 현재 진행 중인 인스턴스 정보 조회
// 이중 맵 구조라 파티 ID -> 인스턴스 ID -> 정보 순서로 찾음
bool InstanceManagerCore::GetInstanceByParty(uint64 partyId, InstanceInfo& out) const
{
    auto it = _partyToInstance.find(partyId);
    if (it == _partyToInstance.end()) return false;

    auto it2 = _instances.find(it->second);
    if (it2 == _instances.end()) return false;

    out = it2->second;
    return true;
}

// 유니크한 인스턴스 ID 생성기
// 단순히 증가하는 정수만 쓰면 서버 재시작 시 겹칠 수 있어서
// 현재 시간(tick)을 상위 비트에, 시퀀스를 하위 비트에 섞어서 충돌 방지함
int64 InstanceManagerCore::GenerateInstanceId()
{
    const uint64 now = ::GetTickCount64();
    const uint16_t seq = ++_seq;

    // 상위 48비트: 시간, 하위 16비트: 순차 증가값
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

// 인스턴스 생성 혹은 조회 메인 로직
// 파티가 이미 던전에 있으면 기존 정보를 주고, 없으면 새로 만듦
bool InstanceManagerCore::CreateOrGetForParty(uint64 partyId, int32 channelId, int32 mapId,
    const Vector<uint64>& members, InstanceInfo& out)
{
    out = InstanceInfo{};
    if (partyId == 0) return false;

    // 이미 생성된 게 있는지 확인 (중복 생성 방지)
    InstanceInfo exist;
    if (GetInstanceByParty(partyId, exist))
    {
        out = exist;
        return true;
    }

    // 새 인스턴스 정보 세팅
    InstanceInfo inst;
    inst.instanceId = GenerateInstanceId();
    inst.channelId = channelId;
    inst.mapId = mapId;
    inst.partyId = partyId;
    inst.createdTick = ::GetTickCount64();
    inst.members.insert(members.begin(), members.end());

    // 관리용 맵들에 등록
    _partyToInstance[partyId] = inst.instanceId;
    _instances[inst.instanceId] = inst;

    // 플레이어 개개인도 어느 인스턴스에 있는지 역인덱싱 해둠
    // 나중에 플레이어가 튕겼다 재접속할 때 어느 던전이었는지 찾으려고 씀
    for (uint64 pid : members)
        _playerToInstance[pid] = inst.instanceId;

    out = inst;
    return true;
}

// 파티 기준 인스턴스 종료 처리
// 관련된 모든 맵(파티, 인스턴스, 플레이어 역인덱스)을 깔끔하게 정리해야 함
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

    // 역인덱스 정리 (중요)
    // 이거 안 지우면 플레이어는 이미 던전 끝났는데 계속 던전 상태로 남음
    for (uint64 pid : outClosed.members)
    {
        auto p = _playerToInstance.find(pid);
        if (p != _playerToInstance.end() && p->second == instanceId)
            _playerToInstance.erase(p);
    }

    _instances.erase(it2);
    return true;
}

// 특정 멤버만 던전에서 내보낼 때 (탈퇴, 강퇴 등)
bool InstanceManagerCore::EjectMember(int64 instanceId, uint64 playerId, bool& outInstanceEmpty)
{
    outInstanceEmpty = false;

    auto it = _instances.find(instanceId);
    if (it == _instances.end()) return false;

    InstanceInfo& inst = it->second;

    // 멤버 목록에서 제거 시도
    if (inst.members.erase(playerId) == 0)
        return false;

    // 역인덱스에서도 제거
    auto p = _playerToInstance.find(playerId);
    if (p != _playerToInstance.end() && p->second == instanceId)
        _playerToInstance.erase(p);

    inst.lastActiveTick = ::GetTickCount64();

    // 사람이 다 빠지면 방 폭파 신호를 줌
    if (inst.members.empty())
        outInstanceEmpty = true;

    return true;
}

// 멤버가 오프라인 됐을 때 처리
bool InstanceManagerCore::OnMemberOffline(uint64 playerId, InstanceInfo& outClosedIfEmpty)
{
    outClosedIfEmpty = InstanceInfo{};

    auto it = _playerToInstance.find(playerId);
    if (it == _playerToInstance.end()) return false;

    const int64 instanceId = it->second;

    bool empty = false;
    if (!EjectMember(instanceId, playerId, empty))
        return false;

    // 마지막 한 명이었으면 인스턴스 자체를 닫아버림
    if (empty)
    {
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

// 타임아웃 검사
// O(N)으로 전체 순회하는데 인스턴스 개수가 수만 개가 아니면 괜찮음
void InstanceManagerCore::CollectExpired(uint64 nowMs, Vector<InstanceInfo>& outToClose) const
{
    outToClose.clear();

    for (auto& kv : _instances)
    {
        const InstanceInfo& inst = kv.second;
        if (inst.closing) continue;

        // 생성된 지 30분 지났으면 종료 대상으로 분류
        if (nowMs - inst.createdTick >= kInstanceTimeoutMs)
            outToClose.push_back(inst);
    }
}

// 인스턴스 ID로 직접 종료 (관리자 명령이나 타임아웃 처리용)
bool InstanceManagerCore::CloseByInstanceId(int64 instanceId, InstanceInfo& outClosed)
{
    outClosed = InstanceInfo{};

    auto it = _instances.find(instanceId);
    if (it == _instances.end()) return false;

    outClosed = it->second;

    // 파티 매핑 제거
    auto pit = _partyToInstance.find(outClosed.partyId);
    if (pit != _partyToInstance.end() && pit->second == instanceId)
        _partyToInstance.erase(pit);

    // 역인덱스 정리
    for (uint64 pid : outClosed.members)
    {
        auto p = _playerToInstance.find(pid);
        if (p != _playerToInstance.end() && p->second == instanceId)
            _playerToInstance.erase(p);
    }

    _instances.erase(it);
    return true;
}