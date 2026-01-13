#include "pch.h"
#include "Monster.h"
#include "GameRoom.h"
#include "DataManager.h"
#include "Player.h"
#include "ClientPacketHandler.h"
#include "ObjectUtils.h" // [New] 유틸 포함

// 쿨타임 관리 (임시)
static uint64 s_lastAttackTime = 0;

Monster::Monster() : Creature(Protocol::OBJECT_TYPE_MONSTER)
{
	_posInfo = _monsterInfo.mutable_posinfo();
	_statInfo = _monsterInfo.mutable_statinfo();

	_monsterInfo.set_objectid(GetObjectId());

	_posInfo->set_state(Protocol::MOVE_IDLE);
	_posInfo->set_actionstate(Protocol::ACTION_IDLE);
}

Monster::~Monster()
{
}

void Monster::Init(int32 templateId)
{
	_monsterInfo.set_templateid(templateId);

	// 스탯 설정
	Protocol::StatInfo* stat = GetStatInfo();
	stat->set_maxhp(10);
	stat->set_hp(10);
	stat->set_attack(10);
	stat->set_defense(0);
	stat->set_speed(1.0f); // [Check] 초당 이동 거리 (단위를 잘 맞춰야 함. 너무 느리면 안 움직이는 것처럼 보임)

	_monsterInfo.set_name("Slime_King");
}

void Monster::Update()
{
	if (_statInfo->hp() <= 0)
	{
		if (_posInfo->actionstate() != Protocol::ACTION_DEAD)
			OnDead(nullptr);
		return;
	}

	std::shared_ptr<Creature> target = GetTarget();
	if (target == nullptr)
	{
		_target.reset();
		_posInfo->set_state(Protocol::MOVE_IDLE);
		_posInfo->set_actionstate(Protocol::ACTION_IDLE);
	}

	if (_posInfo->actionstate() == Protocol::ACTION_ATTACK)
	{
		UpdateAttack();
	}
	else if (_target.lock() != nullptr)
	{
		UpdateMove();
	}
	else
	{
		UpdateIdle();
	}
}

void Monster::UpdateIdle()
{
	std::shared_ptr<GameRoom> room = GetGameRoom();
	if (room == nullptr) return;

	std::shared_ptr<Player> target = room->FindNearestPlayer(GetPosInfo(), _searchRange);

	if (target)
	{
		_target = target;
	}
}

void Monster::UpdateMove()
{
	std::shared_ptr<Creature> target = GetTarget();
	if (target == nullptr) return;

	// 0) 타겟을 바라보는 yaw 계산 (이게 없어서 방향이 안 갔던 거)
	Vector3 dir = ObjectUtils::GetDirection(*_posInfo, *target->GetPosInfo());

	// dir이 0이면(겹침) yaw 갱신 생략
	if (dir.x != 0.0f || dir.z != 0.0f)
	{
		float yawDeg = std::atan2(dir.x, dir.z) * 180.0f / 3.141592f; // Unity yaw 기준
		_posInfo->set_yaw(yawDeg);
	}

	float distSqr = ObjectUtils::DistSqr(*_posInfo, *target->GetPosInfo());
	float attackRangeSqr = _attackRange * _attackRange;

	// 1) 사거리 체크
	if (distSqr <= attackRangeSqr)
	{
		_posInfo->set_state(Protocol::MOVE_IDLE);
		_posInfo->set_actionstate(Protocol::ACTION_ATTACK);
		// 아래에서 OnMonsterMoved로 상태/방향 브로드캐스트
	}
	else
	{
		// 2) 이동
		_posInfo->set_state(Protocol::MOVE_RUN);
		_posInfo->set_actionstate(Protocol::ACTION_IDLE);

		float   deltaTime = 0.1f;
		Vector3 deltaMove = dir * _statInfo->speed() * deltaTime;

		_posInfo->set_x(_posInfo->x() + deltaMove.x);
		_posInfo->set_z(_posInfo->z() + deltaMove.z);
	}

	// 3) Zone 갱신 + 위치/방향 브로드캐스트
	if (auto room = GetGameRoom())
	{
		MonsterRef self = std::static_pointer_cast<Monster>(shared_from_this());
		room->OnMonsterMoved(self);
	}
}


void Monster::UpdateAttack()
{
	std::shared_ptr<Creature> target = GetTarget();
	if (target == nullptr)
	{
		_posInfo->set_actionstate(Protocol::ACTION_IDLE);
		_posInfo->set_state(Protocol::MOVE_IDLE);
		return;
	}

	// 0) 공격 중에도 타겟 바라보게 yaw 갱신 + 브로드캐스트 필요
	Vector3 dir = ObjectUtils::GetDirection(*_posInfo, *target->GetPosInfo());
	if (dir.x != 0.0f || dir.z != 0.0f)
	{
		float yawDeg = std::atan2(dir.x, dir.z) * 180.0f / 3.141592f;
		_posInfo->set_yaw(yawDeg);
	}

	// 1) 거리 체크
	float distSqr = ObjectUtils::DistSqr(*_posInfo, *target->GetPosInfo());
	float attackRangeSqr = _attackRange * _attackRange;

	if (distSqr > attackRangeSqr * 1.5f)
	{
		_posInfo->set_actionstate(Protocol::ACTION_IDLE);
		_posInfo->set_state(Protocol::MOVE_IDLE);
		return;
	}

	// ✅ 공격 상태 유지(이동은 없음)
	_posInfo->set_state(Protocol::MOVE_IDLE);
	_posInfo->set_actionstate(Protocol::ACTION_ATTACK);

	// 2) 공격 중에도 방향이 바뀌면 클라가 따라가게 브로드캐스트
	if (auto room = GetGameRoom())
	{
		MonsterRef self = std::static_pointer_cast<Monster>(shared_from_this());
		room->OnMonsterMoved(self); // yaw/state를 S_MOVE로 흘려보냄
	}

	// 3) 쿨타임 체크 (지금 static이라 "전체 몬스터 공용"임. 일단은 유지)
	uint64 now = ::GetTickCount64();
	if (now - s_lastAttackTime < 1000) return;
	s_lastAttackTime = now;

	// 4) 스킬 실행 → S_SKILL 브로드캐스트는 GameRoom::HandleSkill에서 함
	printf("🥊 [Monster] Attack! -> Player %llu\n", target->GetObjectId());
	if (auto room = GetGameRoom())
	{
		room->HandleSkill(static_pointer_cast<Creature>(shared_from_this()), 1, _posInfo->yaw(), 0);
	}
}

void Monster::OnDamaged(std::shared_ptr<Creature> attacker, int32 damage)
{
	Creature::OnDamaged(attacker, damage);

	if (attacker)
	{
		_target = attacker;
		if (_posInfo->state() == Protocol::MOVE_IDLE && _posInfo->actionstate() == Protocol::ACTION_IDLE)
		{
			_posInfo->set_state(Protocol::MOVE_RUN);
		}
	}
}

void Monster::OnDead(std::shared_ptr<Creature> attacker)
{
	if (_posInfo->actionstate() == Protocol::ACTION_DEAD) return;

	_posInfo->set_actionstate(Protocol::ACTION_DEAD);
	_posInfo->set_state(Protocol::MOVE_IDLE);

	Creature::OnDead(attacker);

	std::shared_ptr<GameRoom> room = GetGameRoom();
	if (room)
	{
		MonsterRef self = std::static_pointer_cast<Monster>(shared_from_this());
		room->PushJob(&GameRoom::HandleMonsterDead, attacker, self);
	}
}


std::shared_ptr<Creature> Monster::GetTarget()
{
	std::shared_ptr<Creature> target = _target.lock();
	if (target == nullptr) return nullptr;
	if (target->GetStatInfo()->hp() <= 0) return nullptr;
	if (target->GetRoom() != GetRoom()) return nullptr;

	return target;
}