// Creature.h

#pragma once
#include "Protocol.pb.h"
#include "GameRoom.h"
#include <memory>
#include <map>

// forward
class RoomActor;
class GameRoom;

using RoomActorRef = std::shared_ptr<RoomActor>;

class Creature : public enable_shared_from_this<Creature>
{
public:
    Creature(Protocol::ObjectType type);
    virtual ~Creature();

    uint64 GetObjectId() const { return _objectId; }
    Protocol::ObjectType GetObjectType() const { return _objectType; }

    // [Room Link] 이제 RoomActor로 통일
    void SetRoom(RoomActorRef room) { _room = room; }
    RoomActorRef GetRoom() const { return _room.lock(); }

    // [Helper] Monster/전투 로직은 GameRoom이 필요하니 캐스팅 헬퍼 제공
    std::shared_ptr<GameRoom> GetGameRoom() const
    {
        return std::dynamic_pointer_cast<GameRoom>(_room.lock());
    }

    void SetZoneIndex(int32 index) { _zoneIndex = index; }
    int32 GetZoneIndex() const { return _zoneIndex; }

    Protocol::PositionInfo* GetPosInfo() { return _posInfo; }
    void SetPosInfo(Protocol::PositionInfo* info) { _posInfo = info; }

    Protocol::StatInfo* GetStatInfo() { return _statInfo; }
    void SetStatInfo(Protocol::StatInfo* info) { _statInfo = info; }

    virtual void OnDamaged(std::shared_ptr<Creature> attacker, int32 damage);
    virtual void OnDead(std::shared_ptr<Creature> attacker);

    bool CanUseSkill(int32 skillId);
    void UseSkill(int32 skillId);

protected:
    uint64 _objectId = 0;
    Protocol::ObjectType _objectType = Protocol::OBJECT_TYPE_NONE;

    Protocol::PositionInfo* _posInfo = nullptr;
    Protocol::StatInfo* _statInfo = nullptr;

    //  RoomActor로 변경
    std::weak_ptr<RoomActor> _room;
    int32 _zoneIndex = -1;

    Map<int32, uint64> _cooldowns;

private:
    static std::atomic<uint64> s_idGenerator;
};
