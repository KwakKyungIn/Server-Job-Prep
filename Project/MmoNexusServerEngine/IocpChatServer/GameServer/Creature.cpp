#include "pch.h"
#include "Creature.h"
#include "GameRoom.h"

// 초기값 1부터 시작
std::atomic<uint64> Creature::s_idGenerator = 1;

Creature::Creature(Protocol::ObjectType type) : _objectType(type)
{
	// [ID Generation Logic]
	// 1. 카운터 증가 (1, 2, 3...)
	uint64 id = s_idGenerator.fetch_add(1);

	// 2. 타입 비트 삽입 (상위 8비트로 이동)
	// 예: OBJECT_TYPE_PLAYER(1) << 56 -> 0x0100000000000000
	// 최종 ID = (타입 정보) | (일련 번호)
	_objectId = ((uint64)_objectType << 56) | id;
}

Creature::~Creature()
{
}

void Creature::OnDamaged(std::shared_ptr<Creature> attacker, int32 damage)
{
	if (_statInfo == nullptr) return;

	int32 currentHp = _statInfo->hp();
	int32 finalHp = currentHp - damage;
	if (finalHp < 0) finalHp = 0;

	_statInfo->set_hp(finalHp);

	if (finalHp <= 0)
	{
		OnDead(attacker);
	}
}

void Creature::OnDead(std::shared_ptr<Creature> attacker)
{
	// TODO: 사망 처리
}