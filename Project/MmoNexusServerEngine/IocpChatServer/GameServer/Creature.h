#pragma once
#include "Protocol.pb.h"
#include "GameRoom.h"
#include <memory>
#include <map>

// forward
class RoomActor;
class GameRoom;

using RoomActorRef = std::shared_ptr<RoomActor>;

// 게임 내 존재하는 모든 생명체의 기본 클래스
// 플레이어와 몬스터가 이 클래스를 상속받아서 공통된 기능을 수행한다
// 비동기 람다 안에서 안전하게 사용하기 위해 shared_from_this를 상속받음
class Creature : public enable_shared_from_this<Creature>
{
public:
    Creature(Protocol::ObjectType type);
    virtual ~Creature();

    uint64 GetObjectId() const { return _objectId; }
    Protocol::ObjectType GetObjectType() const { return _objectType; }

    // 현재 이 크리처가 속해 있는 방을 설정한다
    // 순환 참조 문제를 방지하기 위해 shared_ptr 대신 weak_ptr로 들고 있게 설계함
    void SetRoom(RoomActorRef room) { _room = room; }
    RoomActorRef GetRoom() const { return _room.lock(); }

    // 기본 RoomActor는 범용적이라 전투 로직이 없으므로 GameRoom으로 캐스팅해서 쓰는 헬퍼 함수
    // 몬스터나 전투 로직은 GameRoom 단위에서만 돌아가기 때문
    std::shared_ptr<GameRoom> GetGameRoom() const
    {
        return std::dynamic_pointer_cast<GameRoom>(_room.lock());
    }

    // 시야 처리(AOI)를 위해 현재 속한 존의 인덱스를 저장
    void SetZoneIndex(int32 index) { _zoneIndex = index; }
    int32 GetZoneIndex() const { return _zoneIndex; }

    // 위치 정보와 스탯 정보는 빈번하게 바뀌므로 포인터로 접근해서 수정 가능하게 함
    Protocol::PositionInfo* GetPosInfo() { return _posInfo; }
    void SetPosInfo(Protocol::PositionInfo* info) { _posInfo = info; }

    Protocol::StatInfo* GetStatInfo() { return _statInfo; }
    void SetStatInfo(Protocol::StatInfo* info) { _statInfo = info; }

    // 피격 및 사망 처리 함수 (가상 함수로 자식에서 오버라이딩 가능)
    virtual void OnDamaged(std::shared_ptr<Creature> attacker, int32 damage);
    virtual void OnDead(std::shared_ptr<Creature> attacker);

    // 스킬 쿨타임 및 상태 이상 체크
    bool CanUseSkill(int32 skillId);
    // 쿨타임 소비 (서버 기준)
    void StartSkillCooldown(int32 skillId, int32 cooldownMs);
    // 실제 스킬 사용 요청 (여기서 쿨타임 돌리고 룸에 처리 위임)
    void UseSkill(int32 skillId);

protected:
    uint64 _objectId = 0;
    Protocol::ObjectType _objectType = Protocol::OBJECT_TYPE_NONE;

    Protocol::PositionInfo* _posInfo = nullptr;
    Protocol::StatInfo* _statInfo = nullptr;

    // 맵이나 방 정보는 소유권이 없으므로 weak_ptr로 관리
    std::weak_ptr<RoomActor> _room;
    int32 _zoneIndex = -1;

    // 스킬 쿨타임 관리 (SkillId -> NextAvailableTime)
    Map<int32, uint64> _cooldowns;

private:
    // 전역적으로 유니크한 ID를 발급하기 위한 원자적 카운터
    static std::atomic<uint64> s_idGenerator;
};
