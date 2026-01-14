#include "pch.h"
#include "Monster.h"
#include "GameRoom.h"
#include "GameMap.h"
#include "DataManager.h"
#include "Player.h"
#include "ClientPacketHandler.h"
#include "ObjectUtils.h" // [New] 유틸 포함

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

void Monster::Update(uint64 nowMs, uint64 deltaMs)
{
	// ✅ 죽음 처리
	if (_statInfo->hp() <= 0)
	{
		if (_posInfo->actionstate() != Protocol::ACTION_DEAD)
			OnDead(nullptr);
		return;
	}

	// ✅ 타겟 유효성
	std::shared_ptr<Creature> target = GetTarget();
	if (target == nullptr)
	{
		_target.reset();
		_posInfo->set_state(Protocol::MOVE_IDLE);
		_posInfo->set_actionstate(Protocol::ACTION_IDLE);
	}

	// ✅ 상태 머신
	if (_posInfo->actionstate() == Protocol::ACTION_ATTACK)
	{
		UpdateAttack(nowMs, deltaMs);
	}
	else if (_target.lock() != nullptr)
	{
		UpdateMove(nowMs, deltaMs);
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

void Monster::UpdateMove(uint64 nowMs, uint64 deltaMs)
{
	auto room = GetGameRoom();
	if (!room) return;

	auto target = GetTarget();
	if (!target) return;

	// ✅ XZ 전용 방향 (경사/높낮이 속도왜곡 제거)
	Vector3 dir = ObjectUtils::GetDirectionXZ(*_posInfo, *target->GetPosInfo());

	// ✅ yaw 갱신
	if (dir.x != 0.0f || dir.z != 0.0f)
	{
		float yawDeg = std::atan2(dir.x, dir.z) * 180.0f / 3.141592f;
		_posInfo->set_yaw(yawDeg);
	}

	const float distSqr = ObjectUtils::DistSqr(*_posInfo, *target->GetPosInfo());
	const float attackRangeSqr = _attackRange * _attackRange;

	// 1) 사거리면 Attack 진입 (상태만 바꾸고 한번만 흘림)
	if (distSqr <= attackRangeSqr)
	{
		_posInfo->set_state(Protocol::MOVE_IDLE);
		_posInfo->set_actionstate(Protocol::ACTION_ATTACK);

		room->OnMonsterMoved(std::static_pointer_cast<Monster>(shared_from_this()));
		return;
	}

	// 2) 이동
	_posInfo->set_state(Protocol::MOVE_RUN);
	_posInfo->set_actionstate(Protocol::ACTION_IDLE);

	const float dtSec = static_cast<float>(deltaMs) * 0.001f;
	Vector3 deltaMove = dir * _statInfo->speed() * dtSec;

	Protocol::PositionInfo cur = *_posInfo;
	Protocol::PositionInfo next = cur;
	next.set_x(cur.x() + deltaMove.x);
	next.set_z(cur.z() + deltaMove.z);

	Protocol::PositionInfo fixed;
	if (room->GetMap() && room->GetMap()->ValidateMove(cur, next, fixed))
	{
		_posInfo->set_x(fixed.x());
		_posInfo->set_y(fixed.y());
		_posInfo->set_z(fixed.z());

		// ✅ ValidateMove가 yaw를 target yaw로 복사하니 일관성 유지
		_posInfo->set_yaw(fixed.yaw());
	}
	else
	{
		// ✅ Nav 밖이면 stop (Phase1 안전)
		_posInfo->set_state(Protocol::MOVE_IDLE);
	}

	// 3) 이동/방향 갱신 전파
	room->OnMonsterMoved(std::static_pointer_cast<Monster>(shared_from_this()));
}


void Monster::UpdateAttack(uint64 nowMs, uint64 deltaMs)
{
	auto room = GetGameRoom();
	if (!room) return;

	auto target = GetTarget();
	if (!target)
	{
		_posInfo->set_actionstate(Protocol::ACTION_IDLE);
		_posInfo->set_state(Protocol::MOVE_IDLE);

		// ✅ 상태변화는 한번 흘려주면 안전
		room->OnMonsterMoved(std::static_pointer_cast<Monster>(shared_from_this()));
		return;
	}

	// ✅ yaw는 XZ 전용으로 통일 (경사면에서도 안정)
	Vector3 dir = ObjectUtils::GetDirectionXZ(*_posInfo, *target->GetPosInfo());

	// yaw 변경 감지 (불필요한 네트워크/CPU 줄이기)
	const float prevYaw = _posInfo->yaw();

	if (dir.x != 0.0f || dir.z != 0.0f)
	{
		float yawDeg = std::atan2(dir.x, dir.z) * 180.0f / 3.141592f;
		_posInfo->set_yaw(yawDeg);
	}

	// 거리 체크
	const float distSqr = ObjectUtils::DistSqr(*_posInfo, *target->GetPosInfo());
	const float attackRangeSqr = _attackRange * _attackRange;

	// ✅ 너무 멀어지면 공격 해제
	if (distSqr > attackRangeSqr * 1.5f)
	{
		_posInfo->set_actionstate(Protocol::ACTION_IDLE);
		_posInfo->set_state(Protocol::MOVE_IDLE);

		room->OnMonsterMoved(std::static_pointer_cast<Monster>(shared_from_this()));
		return;
	}

	// ✅ 공격 상태 유지
	_posInfo->set_state(Protocol::MOVE_IDLE);
	_posInfo->set_actionstate(Protocol::ACTION_ATTACK);

	// ✅ Attack 중에는 "yaw가 바뀌었을 때만" 전파 (매틱 전파 금지)
	if (std::fabs(_posInfo->yaw() - prevYaw) > 0.5f) // 0.5도 이상 변화 시만
	{
		room->OnMonsterMoved(std::static_pointer_cast<Monster>(shared_from_this()));
	}

	// ✅ 개별 쿨타임 (nowMs 재사용)
	if (nowMs - _lastAttackMs < 1000)
		return;

	_lastAttackMs = nowMs;

	// 스킬 실행
	printf("🥊 [Monster] Attack! -> Player %llu\n", target->GetObjectId());
	room->HandleSkill(static_pointer_cast<Creature>(shared_from_this()), 1, _posInfo->yaw(), 0);
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