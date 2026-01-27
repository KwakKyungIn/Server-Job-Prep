#include "pch.h"
#include "PartyManagerCore.h"

uint64 PartyManagerCore::GetPartyIdByPlayerId(uint64 playerId) const
{
    // O(1) 조회용 맵 사용
    auto it = _playerToParty.find(playerId);
    return (it == _playerToParty.end()) ? 0 : it->second;
}

// 특정 유저가 파티 멤버인지 확인
bool PartyManagerCore::IsMember(uint64 partyId, uint64 playerId) const
{
    auto it = _parties.find(partyId);
    if (it == _parties.end()) return false;
    // HashSet이라 검색 빠름
    return it->second.members.find(playerId) != it->second.members.end();
}

// 파티 정보를 복사해서 리턴 (스레드 안전한 읽기를 위함)
PartyManagerCore::Party PartyManagerCore::GetSnapshot(uint64 partyId) const
{
    auto it = _parties.find(partyId);
    if (it == _parties.end()) return Party{};
    return it->second;
}

void PartyManagerCore::GetMembers(uint64 partyId, Vector<uint64>& outMembers) const
{
    outMembers.clear();

    auto it = _parties.find(partyId);
    if (it == _parties.end()) return;

    outMembers.reserve(it->second.members.size());
    for (uint64 id : it->second.members)
        outMembers.push_back(id);
}

// 파티 생성
bool PartyManagerCore::Create(uint64 leaderId, uint64& outPartyId)
{
    outPartyId = 0;
    if (leaderId == 0) return false;

    // 이미 파티에 속해있으면 생성 불가
    if (_playerToParty.find(leaderId) != _playerToParty.end())
        return false;

    const uint64 partyId = _nextPartyId++;
    Party p;
    p.partyId = partyId;
    p.leaderId = leaderId;
    p.version = 1;
    p.members.insert(leaderId);

    // map은 복사 비용이 비싸니 move로 소유권 이전
    _parties[partyId] = std::move(p);
    _playerToParty[leaderId] = partyId;

    outPartyId = partyId;
    return true;
}

// 파티 초대 요청
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

    auto pit = _parties.find(partyId);
    if (pit == _parties.end()) return false;

    // [중요] 던전 입장/퇴장 중일 때 초대 금지
    // 위치 동기화나 인스턴스 생성 시점이 꼬일 수 있음
    if (pit->second.inDungeonTransition) return false;

    // 이미 던전 안에 있을 때도 초대 금지 (난입 불가 컨셉)
    if (pit->second.instanceId != 0) return false;

    // 대상이 이미 다른 파티에 있으면 실패
    if (_playerToParty.find(targetId) != _playerToParty.end())
        return false;

    // 기존 초대장이 있다면 제거하고 새로 갱신 (Spam 방지 겸 최신화)
    _pendingByTarget.erase(targetId);

    PendingInvite inv;
    inv.partyId = partyId;
    inv.inviterId = inviterId;
    inv.targetId = targetId;
    inv.expireTick = ::GetTickCount64() + 60'000; // 60초 유효

    _pendingByTarget[targetId] = inv;
    outInvite = inv;
    return true;
}

// 초대 수락/거절 처리
bool PartyManagerCore::AcceptInvite(uint64 targetId, uint64 partyId, bool accept, Party& outPartyAfter)
{
    outPartyAfter = Party{};
    if (targetId == 0 || partyId == 0) return false;

    auto pit = _pendingByTarget.find(targetId);
    if (pit == _pendingByTarget.end()) return false;

    PendingInvite inv = pit->second;
    // 초대장이 다른 파티 거거나 만료됐으면 실패
    if (inv.partyId != partyId) return false;
    if (::GetTickCount64() > inv.expireTick) { _pendingByTarget.erase(pit); return false; }

    // 거절인 경우 초대장만 지우고 끝
    if (!accept)
    {
        _pendingByTarget.erase(pit);
        return true;
    }

    // 그 사이에 다른 파티에 들어갔으면 실패
    if (_playerToParty.find(targetId) != _playerToParty.end())
    {
        _pendingByTarget.erase(pit);
        return false;
    }

    auto it = _parties.find(partyId);
    if (it == _parties.end()) { _pendingByTarget.erase(pit); return false; }

    // 수락하는 순간에도 파티 상태 체크 (던전 진입 중이면 합류 불가)
    if (it->second.inDungeonTransition) { _pendingByTarget.erase(pit); return false; }
    if (it->second.instanceId != 0) { _pendingByTarget.erase(pit); return false; }

    // 멤버 추가
    it->second.members.insert(targetId);
    it->second.version++;

    _playerToParty[targetId] = partyId;
    _pendingByTarget.erase(pit);

    outPartyAfter = it->second;
    return true;
}

// 파티 탈퇴
bool PartyManagerCore::Leave(uint64 playerId, Party& outPartyAfter, bool& outDisbanded)
{
    outPartyAfter = Party{};
    outDisbanded = false;
    if (playerId == 0) return false;

    auto pit = _playerToParty.find(playerId);
    if (pit == _playerToParty.end()) return false;

    uint64 partyId = pit->second;
    auto it = _parties.find(partyId);

    if (it == _parties.end()) { _playerToParty.erase(pit); return false; }

    // 트랜지션 중 탈퇴 금지 (서버 이동 중에 나가면 미아 됨)
    if (it->second.inDungeonTransition) return false;

    it->second.members.erase(playerId);
    _playerToParty.erase(pit);

    // 파티장이 나갔으면 권한 위임
    // 단순하게 ID가 가장 낮은 사람(보통 먼저 가입한 사람)에게 넘김
    if (it->second.leaderId == playerId && !it->second.members.empty())
    {
        uint64 newLeader = *it->second.members.begin();
        for (uint64 m : it->second.members) newLeader = min(newLeader, m);
        it->second.leaderId = newLeader;
    }

    it->second.version++;

    // 멤버가 한 명도 없으면 파티 폭파
    if (it->second.members.empty())
    {
        _parties.erase(it);
        outDisbanded = true;
        return true;
    }

    outPartyAfter = it->second;
    return true;
}

// 강제 퇴장
bool PartyManagerCore::Kick(uint64 leaderId, uint64 targetId, Party& outPartyAfter)
{
    outPartyAfter = Party{};
    if (leaderId == 0 || targetId == 0) return false;

    auto lp = _playerToParty.find(leaderId);
    if (lp == _playerToParty.end()) return false;

    uint64 partyId = lp->second;
    auto it = _parties.find(partyId);

    if (it == _parties.end()) return false;

    // 유효성 체크들
    if (it->second.inDungeonTransition) return false;
    if (it->second.leaderId != leaderId) return false; // 리더만 킥 가능
    if (it->second.members.find(targetId) == it->second.members.end()) return false; // 없는 사람 킥 불가

    it->second.members.erase(targetId);
    it->second.version++;

    auto tp = _playerToParty.find(targetId);
    if (tp != _playerToParty.end() && tp->second == partyId)
        _playerToParty.erase(tp);

    outPartyAfter = it->second;
    return true;
}

// 파티 해산
bool PartyManagerCore::Disband(uint64 leaderId, Party& outDisbandedParty)
{
    outDisbandedParty = Party{};
    if (leaderId == 0) return false;

    auto lp = _playerToParty.find(leaderId);
    if (lp == _playerToParty.end()) return false;

    uint64 partyId = lp->second;
    auto it = _parties.find(partyId);

    if (it == _parties.end()) return false;

    if (it->second.inDungeonTransition) return false;
    if (it->second.leaderId != leaderId) return false;

    outDisbandedParty = it->second;

    // 모든 멤버의 파티 정보 제거
    for (uint64 id : it->second.members)
    {
        auto p = _playerToParty.find(id);
        if (p != _playerToParty.end() && p->second == partyId)
            _playerToParty.erase(p);
    }

    _parties.erase(it);
    return true;
}

// 던전 정보 조회용
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

// 던전 진입 시작 (상태 잠금)
bool PartyManagerCore::TryBeginDungeonTransition(uint64 partyId, DungeonState nextState)
{
    auto it = _parties.find(partyId);
    if (it == _parties.end()) return false;

    Party& p = it->second;
    if (p.inDungeonTransition) return false; // 이미 진행 중

    p.inDungeonTransition = true;
    p.dungeonState = nextState;
    p.version++;
    return true;
}

// 던전 진입/퇴장 완료 (상태 잠금 해제)
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

// 파티의 인스턴스 ID 강제 설정 (디버그나 복구용)
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

// 파티의 던전 정보 초기화 (필드로 복귀했을 때)
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

// 강제 귀환 플래그 설정
void PartyManagerCore::MarkForceReturn(uint64 playerId)
{
    if (playerId == 0) return;
    _forceReturn.insert(playerId);
}

bool PartyManagerCore::ConsumeForceReturn(uint64 playerId)
{
    auto it = _forceReturn.find(playerId);
    if (it == _forceReturn.end()) return false;
    _forceReturn.erase(it);
    return true;
}