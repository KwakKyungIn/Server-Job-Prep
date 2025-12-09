#pragma once
#include "Protocol.pb.h" // ObjectType 쓰려면 필요

class GameRoom;

class Creature : public enable_shared_from_this<Creature>
{
public:
	// 생성자에서 타입(Player/Monster)을 받는다.
	Creature(Protocol::ObjectType type);
	virtual ~Creature();

	// [Identity]
	// 이제 가상함수가 아니라 멤버 변수 반환
	uint64					GetObjectId() const { return _objectId; }
	Protocol::ObjectType	GetObjectType() const { return _objectType; }

	// [Movement]
	void					SetRoom(std::shared_ptr<GameRoom> room) { _room = room; }
	std::shared_ptr<GameRoom> GetRoom() { return _room.lock(); }

	void					SetZoneIndex(int32 index) { _zoneIndex = index; }
	int32					GetZoneIndex() const { return _zoneIndex; }

	Protocol::PositionInfo* GetPosInfo() { return _posInfo; }
	void					SetPosInfo(Protocol::PositionInfo* info) { _posInfo = info; }

	// [Stat]
	Protocol::StatInfo* GetStatInfo() { return _statInfo; }
	void					SetStatInfo(Protocol::StatInfo* info) { _statInfo = info; }

	// [Combat]
	virtual void			OnDamaged(std::shared_ptr<Creature> attacker, int32 damage);
	virtual void			OnDead(std::shared_ptr<Creature> attacker);

	// 스킬 사용 가능 여부 체크 (쿨타임, 상태이상 등)
	bool CanUseSkill(int32 skillId);

	// 스킬 실행 (실제 로직)
	void UseSkill(int32 skillId);

protected:
	// [Identity]
	uint64					_objectId = 0;
	Protocol::ObjectType	_objectType = Protocol::OBJECT_TYPE_NONE;

	// [References]
	Protocol::PositionInfo* _posInfo = nullptr;
	Protocol::StatInfo* _statInfo = nullptr;

	std::weak_ptr<GameRoom> _room;
	int32					_zoneIndex = -1;

	// Key: SkillID, Value: 다음 사용 가능 시간 (Tick)
	std::map<int32, uint64> _cooldowns;

private:
	// ID 발급기 (스레드 안전)
	static std::atomic<uint64> s_idGenerator;
};