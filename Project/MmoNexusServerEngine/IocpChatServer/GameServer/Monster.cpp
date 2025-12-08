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
	stat->set_maxhp(100);
	stat->set_hp(100);
	stat->set_attack(10);
	stat->set_defense(0);
	stat->set_speed(5.0f); // [Check] 초당 이동 거리 (단위를 잘 맞춰야 함. 너무 느리면 안 움직이는 것처럼 보임)

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
	std::shared_ptr<GameRoom> room = GetRoom();
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

	// [Refactoring] ObjectUtils 사용
	// 1. 거리 체크 (제곱 계산으로 최적화)
	float distSqr = ObjectUtils::DistSqr(*_posInfo, *target->GetPosInfo());
	float attackRangeSqr = _attackRange * _attackRange;

	// 2. 사거리 판정
	if (distSqr <= attackRangeSqr)
	{
		_posInfo->set_state(Protocol::MOVE_IDLE);
		_posInfo->set_actionstate(Protocol::ACTION_ATTACK);
		return;
	}

	// 3. 이동 로직
	_posInfo->set_state(Protocol::MOVE_RUN);
	_posInfo->set_actionstate(Protocol::ACTION_IDLE);

	// 방향 벡터 구하기
	Vector3 dir = ObjectUtils::GetDirection(*_posInfo, *target->GetPosInfo());

	// 속도 적용 (DeltaTime은 일단 0.1f로 가정)
	// 나중엔 함수 인자로 deltaTime을 받아와야 함
	float deltaTime = 0.1f;
	Vector3 deltaMove = dir * _statInfo->speed() * deltaTime;

	// 좌표 갱신
	_posInfo->set_x(_posInfo->x() + deltaMove.x);
	_posInfo->set_z(_posInfo->z() + deltaMove.z);

	// 4. [Broadcast] 이동 패킷 전송
	std::shared_ptr<GameRoom> room = GetRoom();
	if (room)
	{
		Protocol::S_MOVE movePkt;
		movePkt.set_objectid(_objectId);
		*movePkt.mutable_posinfo() = *_posInfo;

		SendBufferRef sendBuffer = ClientPacketHandler::MakeSendBuffer(movePkt);

		// [Fixed] 이제 에러 안 날 거다. (GameRoom.h 수정 필수)
		room->BroadcastToZone(sendBuffer, _zoneIndex);
	}
}

void Monster::UpdateAttack()
{
	std::shared_ptr<Creature> target = GetTarget();
	if (target == nullptr)
	{
		_posInfo->set_actionstate(Protocol::ACTION_IDLE);
		return;
	}

	// [Refactoring] 거리 체크
	float distSqr = ObjectUtils::DistSqr(*_posInfo, *target->GetPosInfo());
	float attackRangeSqr = _attackRange * _attackRange;

	// 공격 범위 약간 여유(Buffer) 줌 (1.5배)
	if (distSqr > attackRangeSqr * 1.5f)
	{
		_posInfo->set_actionstate(Protocol::ACTION_IDLE);
		return;
	}

	// 쿨타임 체크
	uint64 now = ::GetTickCount64();
	if (now - s_lastAttackTime < 1000) return;

	s_lastAttackTime = now;

	// 타격
	printf("🥊 [Monster] Attack! -> Player %llu\n", target->GetObjectId());
	target->OnDamaged(static_pointer_cast<Creature>(shared_from_this()), _statInfo->attack());
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

	std::shared_ptr<GameRoom> room = GetRoom();
	if (room)
	{
		room->LeaveMonster(GetObjectId());
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