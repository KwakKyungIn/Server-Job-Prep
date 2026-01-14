#include "pch.h"
#include "Monster.h"
#include "GameRoom.h"
#include "GameMap.h"
#include "DataManager.h"
#include "Player.h"
#include "ClientPacketHandler.h"
#include "ObjectUtils.h"

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
	// 죽음 처리
	if (_statInfo->hp() <= 0)
	{
		if (_posInfo->actionstate() != Protocol::ACTION_DEAD)
			OnDead(nullptr);
		return;
	}

	auto room = GetGameRoom();
	if (!room) return;

	// spawn pos 1회 캡처
	if (!_spawnInited)
	{
		_spawnPos = *_posInfo;
		_spawnInited = true;

		_lastProgressX = _posInfo->x();
		_lastProgressZ = _posInfo->z();
	}

	// 타겟 유효성
	auto target = GetTarget();
	if (!target)
	{
		_target.reset();

		// Chase/Attack 중에 타겟 잃으면 Return으로
		if (_aiState == AiState::Chase || _aiState == AiState::Attack)
			EnterState(AiState::Return, /*notify=*/true);
	}

	switch (_aiState)
	{
	case AiState::Idle:   UpdateIdle(nowMs); break;
	case AiState::Chase:  UpdateChase(nowMs, deltaMs); break;
	case AiState::Attack: UpdateAttack(nowMs, deltaMs); break;
	case AiState::Return: UpdateReturn(nowMs, deltaMs); break;
	}
}

void Monster::UpdateIdle(uint64 nowMs)
{
	auto room = GetGameRoom();
	if (!room) return;

	// Return 중이었다가 Idle로 왔다면 path/stuck 정리
	_stuckAccumMs = 0;

	auto target = room->FindNearestPlayer(GetPosInfo(), _searchRange);
	if (target)
	{
		_target = target;
		EnterState(AiState::Chase, /*notify=*/true);
	}
}


float Monster::Dist2DSqr(const Protocol::PositionInfo& a, const Protocol::PositionInfo& b) const
{
	const float dx = a.x() - b.x();
	const float dz = a.z() - b.z();
	return dx * dx + dz * dz;
}

float Monster::Dist2DSqrToSpawn() const
{
	Protocol::PositionInfo cur = *_posInfo;
	return Dist2DSqr(cur, _spawnPos);
}

bool Monster::MoveTowardPosInfo(const Protocol::PositionInfo& dst, uint64 deltaMs, bool notifyAlways)
{
	auto room = GetGameRoom();
	if (!room) return false;

	// XZ direction
	Vector3 dir = ObjectUtils::GetDirectionXZ(*_posInfo, dst);

	// yaw
	if (dir.x != 0.0f || dir.z != 0.0f)
	{
		float yawDeg = std::atan2(dir.x, dir.z) * 180.0f / 3.141592f;
		_posInfo->set_yaw(yawDeg);
	}

	_posInfo->set_state(Protocol::MOVE_RUN);
	_posInfo->set_actionstate(Protocol::ACTION_IDLE);

	const float speed = static_cast<float>(_statInfo->speed()); // proto가 int32면 여기서 float 캐스팅
	const float dtSec = static_cast<float>(deltaMs) * 0.001f;

	Vector3 deltaMove = dir * speed * dtSec;

	Protocol::PositionInfo cur = *_posInfo;
	Protocol::PositionInfo next = cur;
	next.set_x(cur.x() + deltaMove.x);
	next.set_z(cur.z() + deltaMove.z);
	next.set_yaw(_posInfo->yaw());

	Protocol::PositionInfo fixed;
	bool moved = false;

	if (room->GetMap() && room->GetMap()->ValidateMove(cur, next, fixed))
	{
		_posInfo->set_x(fixed.x());
		_posInfo->set_y(fixed.y());
		_posInfo->set_z(fixed.z());
		_posInfo->set_yaw(fixed.yaw());

		moved = true;
	}
	else
	{
		// nav 밖이면 멈춤
		_posInfo->set_state(Protocol::MOVE_IDLE);
		moved = false;
	}

	// stuck detection (2D)
	const float px = _posInfo->x();
	const float pz = _posInfo->z();
	const float mdx = px - _lastProgressX;
	const float mdz = pz - _lastProgressZ;
	const float moved2 = mdx * mdx + mdz * mdz;

	if (moved2 < (_stuckEps * _stuckEps))
		_stuckAccumMs += deltaMs;
	else
	{
		_stuckAccumMs = 0;
		_lastProgressX = px;
		_lastProgressZ = pz;
	}

	if (notifyAlways)
	{
		room->OnMonsterMoved(std::static_pointer_cast<Monster>(shared_from_this()));
	}

	return moved;
}

bool Monster::RebuildPathTo(const Protocol::PositionInfo& dst, uint64 nowMs)
{
	auto room = GetGameRoom();
	if (!room || !room->GetMap()) return false;

	_lastRepathMs = nowMs;
	_pathIndex = 0;
	_path.clear();

	if (room->GetMap()->FindPathWaypoints(*_posInfo, dst, _path) == false)
		return false;

	// 시작점이 너무 가까운 첫 waypoint는 스킵
	while (_pathIndex < (int)_path.size())
	{
		const float dx = _path[_pathIndex].x - _posInfo->x();
		const float dz = _path[_pathIndex].z - _posInfo->z();
		if ((dx * dx + dz * dz) > (_waypointArriveDist * _waypointArriveDist))
			break;
		_pathIndex++;
	}

	return true;
}

bool Monster::FollowPath(uint64 nowMs, uint64 deltaMs, const Protocol::PositionInfo& finalDst)
{
	auto room = GetGameRoom();
	if (!room || !room->GetMap()) return false;

	// repath 조건: 주기 or stuck
	const bool needRepath =
		(_path.empty()) ||
		(_pathIndex >= (int)_path.size()) ||
		(nowMs - _lastRepathMs >= _repathIntervalMs) ||
		(_stuckAccumMs >= _stuckMs);

	if (needRepath)
	{
		if (RebuildPathTo(finalDst, nowMs) == false)
			return false;
	}

	if (_pathIndex >= (int)_path.size())
		return false;

	// 현재 waypoint를 목적지로 이동
	Protocol::PositionInfo wp;
	wp.set_x(_path[_pathIndex].x);
	wp.set_y(_path[_pathIndex].y);
	wp.set_z(_path[_pathIndex].z);
	wp.set_yaw(_posInfo->yaw());

	MoveTowardPosInfo(wp, deltaMs, /*notifyAlways=*/true);

	// 도착하면 다음 waypoint
	const float dx = wp.x() - _posInfo->x();
	const float dz = wp.z() - _posInfo->z();
	if ((dx * dx + dz * dz) <= (_waypointArriveDist * _waypointArriveDist))
		_pathIndex++;

	return true;
}


void Monster::UpdateChase(uint64 nowMs, uint64 deltaMs)
{
	auto room = GetGameRoom();
	if (!room) return;

	auto target = GetTarget();
	if (!target)
	{
		EnterState(AiState::Return, /*notify=*/true);
		return;
	}

	// leash 체크
	if (Dist2DSqrToSpawn() > (_leashRange * _leashRange))
	{
		_target.reset();
		EnterState(AiState::Return, /*notify=*/true);
		return;
	}

	// 사거리면 Attack
	const float distSqr = ObjectUtils::DistSqr(*_posInfo, *target->GetPosInfo());
	const float attackRangeSqr = _attackRange * _attackRange;

	if (distSqr <= attackRangeSqr)
	{
		EnterState(AiState::Attack, /*notify=*/true);
		return;
	}

	// LOS면 직선 추적, 아니면 path-follow
	bool los = false;
	if (room->GetMap())
		los = room->GetMap()->HasLineOfSight(*_posInfo, *target->GetPosInfo());

	if (los)
	{
		_path.clear(); _pathIndex = 0;
		_stuckAccumMs = 0;
		MoveTowardPosInfo(*target->GetPosInfo(), deltaMs, /*notifyAlways=*/true);
		return;
	}

	// LOS 불가면 path-follow
	if (FollowPath(nowMs, deltaMs, *target->GetPosInfo()) == false)
	{
		// path 실패 시 fallback: 그냥 target 방향으로 밀어보되 ValidateMove가 막아줌
		MoveTowardPosInfo(*target->GetPosInfo(), deltaMs, /*notifyAlways=*/true);
	}
}

void Monster::EnterState(AiState next, bool notify)
{
	if (_aiState == next) return;

	_aiState = next;

	// client-visible state mapping (proto 변경 없이)
	switch (_aiState)
	{
	case AiState::Idle:
		_posInfo->set_state(Protocol::MOVE_IDLE);
		_posInfo->set_actionstate(Protocol::ACTION_IDLE);
		_path.clear(); _pathIndex = 0;
		break;

	case AiState::Chase:
	case AiState::Return:
		_posInfo->set_state(Protocol::MOVE_RUN);
		_posInfo->set_actionstate(Protocol::ACTION_IDLE);
		break;

	case AiState::Attack:
		_posInfo->set_state(Protocol::MOVE_IDLE);
		_posInfo->set_actionstate(Protocol::ACTION_ATTACK);
		_path.clear(); _pathIndex = 0;
		break;
	}

	if (notify)
	{
		auto room = GetGameRoom();
		if (room)
			room->OnMonsterMoved(std::static_pointer_cast<Monster>(shared_from_this()));
	}
}

void Monster::UpdateReturn(uint64 nowMs, uint64 deltaMs)
{
	auto room = GetGameRoom();
	if (!room) return;

	// Return 중 타겟 재획득(선택)
	if (auto t = room->FindNearestPlayer(GetPosInfo(), _searchRange))
	{
		// 리쉬 안쪽이면 다시 chase
		_target = t;
		EnterState(AiState::Chase, /*notify=*/true);
		return;
	}

	// spawn 도착?
	const float dx = _spawnPos.x() - _posInfo->x();
	const float dz = _spawnPos.z() - _posInfo->z();
	if ((dx * dx + dz * dz) <= (_arriveDist * _arriveDist))
	{
		EnterState(AiState::Idle, /*notify=*/true);
		return;
	}

	// LOS면 직선 복귀, 아니면 path-follow
	bool los = false;
	if (room->GetMap())
		los = room->GetMap()->HasLineOfSight(*_posInfo, _spawnPos);

	if (los)
	{
		_path.clear(); _pathIndex = 0;
		_stuckAccumMs = 0;
		MoveTowardPosInfo(_spawnPos, deltaMs, /*notifyAlways=*/true);
		return;
	}

	if (FollowPath(nowMs, deltaMs, _spawnPos) == false)
	{
		// path 실패 fallback
		MoveTowardPosInfo(_spawnPos, deltaMs, /*notifyAlways=*/true);
	}
}

void Monster::UpdateAttack(uint64 nowMs, uint64 deltaMs)
{
	auto room = GetGameRoom();
	if (!room) return;

	// leash 체크 (Attack 중에도)
	if (Dist2DSqrToSpawn() > (_leashRange * _leashRange))
	{
		_target.reset();
		EnterState(AiState::Return, /*notify=*/true);
		return;
	}

	auto target = GetTarget();
	if (!target)
	{
		EnterState(AiState::Return, /*notify=*/true);
		return;
	}

	// yaw만 추적 (네 기존 로직 유지)
	Vector3 dir = ObjectUtils::GetDirectionXZ(*_posInfo, *target->GetPosInfo());
	const float prevYaw = _posInfo->yaw();

	if (dir.x != 0.0f || dir.z != 0.0f)
	{
		float yawDeg = std::atan2(dir.x, dir.z) * 180.0f / 3.141592f;
		_posInfo->set_yaw(yawDeg);
	}

	// 너무 멀어지면 Chase로
	const float distSqr = ObjectUtils::DistSqr(*_posInfo, *target->GetPosInfo());
	const float attackRangeSqr = _attackRange * _attackRange;

	if (distSqr > attackRangeSqr * 1.5f)
	{
		EnterState(AiState::Chase, /*notify=*/true);
		return;
	}

	// Attack 상태 유지 (proto 반영)
	_posInfo->set_state(Protocol::MOVE_IDLE);
	_posInfo->set_actionstate(Protocol::ACTION_ATTACK);

	// yaw 변화만 전파
	if (std::fabs(_posInfo->yaw() - prevYaw) > 0.5f)
		room->OnMonsterMoved(std::static_pointer_cast<Monster>(shared_from_this()));

	// 쿨
	if (nowMs - _lastAttackMs < 1000)
		return;

	_lastAttackMs = nowMs;

	printf("🥊 [Monster] Attack!\n");
	room->HandleSkill(static_pointer_cast<Creature>(shared_from_this()), 1, _posInfo->yaw(), 0);
}

void Monster::OnDamaged(std::shared_ptr<Creature> attacker, int32 damage)
{
	Creature::OnDamaged(attacker, damage);

	if (!attacker) return;

	_target = attacker;

	if (_aiState == AiState::Idle || _aiState == AiState::Return)
		EnterState(AiState::Chase, /*notify=*/true);
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