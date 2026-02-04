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
	// Protocol Buffer 포인터 연결 (성능을 위해 직접 포인터로 제어)
	_posInfo = _monsterInfo.mutable_posinfo();
	_statInfo = _monsterInfo.mutable_statinfo();

	_monsterInfo.set_objectid(GetObjectId());

	// 초기 상태: 가만히 서 있음
	_posInfo->set_state(Protocol::MOVE_IDLE);
	_posInfo->set_actionstate(Protocol::ACTION_IDLE);
}

Monster::~Monster()
{
}

void Monster::Init(int32 templateId, int32 spawnId)
{
	_spawnId = spawnId;
	_monsterInfo.set_templateid(templateId);

	Protocol::StatInfo* stat = GetStatInfo();

	const MonsterTemplate* tpl = DataManager::Instance()->GetMonsterTemplate(templateId);
	if (tpl)
	{
		stat->CopyFrom(tpl->stat);

		if (stat->maxhp() <= 0) stat->set_maxhp(1);
		if (stat->hp() <= 0) stat->set_hp(stat->maxhp());

		_monsterInfo.set_name(tpl->name.empty() ? ("Monster_" + std::to_string(templateId)) : tpl->name);

		_searchRange = tpl->searchRange;
		_attackRange = tpl->attackRange;
		_leashRange = tpl->leashRange;
	}
	else
	{
		// 스탯 설정 (Template 없을 때 기본값)
		stat->set_maxhp(10);
		stat->set_hp(10);
		stat->set_attack(10);
		stat->set_defense(0);
		stat->set_speed(1);

		_monsterInfo.set_name("Monster_Default");
	}
}

// [AI 메인 루프]
// 서버 프레임마다 호출되어 현재 상태에 맞는 로직을 수행함
void Monster::Update(uint64 nowMs, uint64 deltaMs)
{
	// 1. 사망 처리 (HP 0 이하)
	if (_statInfo->hp() <= 0)
	{
		// 아직 죽음 상태가 아니면 Dead 이벤트 발생
		if (_posInfo->actionstate() != Protocol::ACTION_DEAD)
			OnDead(nullptr);
		return;
	}

	auto room = GetGameRoom();
	if (!room) return;

	// 2. 초기 스폰 위치 저장 (리셋용)
	if (!_spawnInited)
	{
		_spawnPos = *_posInfo;
		_spawnInited = true;

		_lastProgressX = _posInfo->x();
		_lastProgressZ = _posInfo->z();
	}

	// 3. 타겟 유효성 검사 (죽었거나 방 나간 타겟은 버림)
	auto target = GetTarget();
	if (!target)
	{
		_target.reset();

		// 추적/공격 중에 타겟이 사라지면 즉시 복귀(Return) 상태로 전환
		if (_aiState == AiState::Chase || _aiState == AiState::Attack)
			EnterState(AiState::Return, /*notify=*/true);
	}

	// 4. 상태별 업데이트 분기 (FSM Pattern)
	switch (_aiState)
	{
	case AiState::Idle:   UpdateIdle(nowMs); break;
	case AiState::Chase:  UpdateChase(nowMs, deltaMs); break;
	case AiState::Attack: UpdateAttack(nowMs, deltaMs); break;
	case AiState::Return: UpdateReturn(nowMs, deltaMs); break;
	}
}

// [State: Idle] 대기 상태
// 주변에 플레이어가 있는지 탐색함 (Scan)
void Monster::UpdateIdle(uint64 nowMs)
{
	auto room = GetGameRoom();
	if (!room) return;

	// 복귀하다가 Idle 된 경우 Stuck 정보 초기화
	_stuckAccumMs = 0;

	// 가장 가까운 플레이어 탐색 (Grid 기반 최적화)
	auto target = room->FindNearestPlayer(GetPosInfo(), _searchRange);
	if (target)
	{
		_target = target;
		EnterState(AiState::Chase, /*notify=*/true); // 발견 즉시 추격 모드
	}
}


float Monster::Dist2DSqr(const Protocol::PositionInfo& a, const Protocol::PositionInfo& b) const
{
	const float dx = a.x() - b.x();
	const float dz = a.z() - b.z();
	return dx * dx + dz * dz;
}

// 스폰 위치로부터의 거리 (제곱)
// 몬스터가 너무 멀리 가면(Leash Range) 돌아오게 하기 위함
float Monster::Dist2DSqrToSpawn() const
{
	Protocol::PositionInfo cur = *_posInfo;
	return Dist2DSqr(cur, _spawnPos);
}

// [이동 로직의 핵심]
// 단순 좌표 이동뿐만 아니라, NavMesh 검증과 벽 충돌 처리를 수행함
bool Monster::MoveTowardPosInfo(const Protocol::PositionInfo& dst, uint64 deltaMs, bool notifyAlways)
{
	auto room = GetGameRoom();
	if (!room) return false;

	// 1. 방향 벡터 계산
	Vector3 dir = ObjectUtils::GetDirectionXZ(*_posInfo, dst);

	// 2. 회전(Yaw) 처리
	if (dir.x != 0.0f || dir.z != 0.0f)
	{
		float yawDeg = std::atan2(dir.x, dir.z) * 180.0f / 3.141592f;
		_posInfo->set_yaw(yawDeg);
	}

	// 상태 동기화 (뛰는 모션)
	_posInfo->set_state(Protocol::MOVE_RUN);
	_posInfo->set_actionstate(Protocol::ACTION_IDLE);

	const float speed = static_cast<float>(_statInfo->speed());
	const float dtSec = static_cast<float>(deltaMs) * 0.001f;

	Vector3 deltaMove = dir * speed * dtSec;

	// 예상 이동 지점
	Protocol::PositionInfo cur = *_posInfo;
	Protocol::PositionInfo next = cur;
	next.set_x(cur.x() + deltaMove.x);
	next.set_z(cur.z() + deltaMove.z);
	next.set_yaw(_posInfo->yaw());

	Protocol::PositionInfo fixed;
	bool moved = false;

	// 3. NavMesh 검증 (서버 권위)
	// 갈 수 있는 곳인지 확인하고, 벽에 막히면 슬라이딩 벡터 등을 계산해준 좌표(fixed)를 받음
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
		// 갈 수 없으면 제자리 멈춤
		_posInfo->set_state(Protocol::MOVE_IDLE);
		moved = false;
	}

	// 4. 끼임(Stuck) 감지 로직
	// 이동 명령을 내렸는데 실제 좌표가 거의 안 변하면 어딘가 꼈다고 판단
	const float px = _posInfo->x();
	const float pz = _posInfo->z();
	const float mdx = px - _lastProgressX;
	const float mdz = pz - _lastProgressZ;
	const float moved2 = mdx * mdx + mdz * mdz;

	if (moved2 < (_stuckEps * _stuckEps))
		_stuckAccumMs += deltaMs;
	else
	{
		_stuckAccumMs = 0; // 잘 움직이면 누적 시간 리셋
		_lastProgressX = px;
		_lastProgressZ = pz;
	}

	// 이동 패킷 브로드캐스팅
	if (notifyAlways)
	{
		room->OnMonsterMoved(std::static_pointer_cast<Monster>(shared_from_this()));
	}

	return moved;
}

// 길찾기 재계산 (A*)
bool Monster::RebuildPathTo(const Protocol::PositionInfo& dst, uint64 nowMs)
{
	auto room = GetGameRoom();
	if (!room || !room->GetMap()) return false;

	_lastRepathMs = nowMs;
	_pathIndex = 0;
	_path.clear();

	// Detour NavMesh를 이용해 경로(Waypoints) 찾기
	if (room->GetMap()->FindPathWaypoints(*_posInfo, dst, _path) == false)
		return false;

	// 시작점이 첫 번째 웨이포인트랑 너무 가까우면 건너뜀 (부들거림 방지)
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

// 경로 따라가기 (Path Following)
bool Monster::FollowPath(uint64 nowMs, uint64 deltaMs, const Protocol::PositionInfo& finalDst)
{
	auto room = GetGameRoom();
	if (!room || !room->GetMap()) return false;

	// 경로 재계산 조건: 
	// 1. 경로가 없거나 끝났을 때
	// 2. 일정 시간(RepathInterval)이 지났을 때 (타겟이 움직였을 수 있으므로)
	// 3. 어딘가 꼈을 때 (Stuck)
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

	// 현재 목표 웨이포인트 설정
	Protocol::PositionInfo wp;
	wp.set_x(_path[_pathIndex].x);
	wp.set_y(_path[_pathIndex].y);
	wp.set_z(_path[_pathIndex].z);
	wp.set_yaw(_posInfo->yaw());

	// 웨이포인트 향해 이동
	MoveTowardPosInfo(wp, deltaMs, /*notifyAlways=*/true);

	// 웨이포인트 도착 판정
	const float dx = wp.x() - _posInfo->x();
	const float dz = wp.z() - _posInfo->z();
	if ((dx * dx + dz * dz) <= (_waypointArriveDist * _waypointArriveDist))
		_pathIndex++; // 다음 지점으로

	return true;
}

// [State: Chase] 추격 상태
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

	// Leash Range(활동 반경) 벗어나면 추격 포기하고 복귀
	if (Dist2DSqrToSpawn() > (_leashRange * _leashRange))
	{
		_target.reset();
		EnterState(AiState::Return, /*notify=*/true);
		return;
	}

	// 사거리 안에 들어왔으면 공격 상태로 전이
	const float distSqr = ObjectUtils::DistSqr(*_posInfo, *target->GetPosInfo());
	const float attackRangeSqr = _attackRange * _attackRange;

	if (distSqr <= attackRangeSqr)
	{
		EnterState(AiState::Attack, /*notify=*/true);
		return;
	}

	// 시야(LOS) 체크: 장애물 없이 뻥 뚫려 있으면 직선 이동 (성능 최적화)
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

	// 장애물 있으면 길찾기(Path Finding) 수행
	if (FollowPath(nowMs, deltaMs, *target->GetPosInfo()) == false)
	{
		// 길찾기 실패 시 비상 대책: 그냥 타겟 방향으로 들이받음 (NavMesh가 막아주길 기대)
		MoveTowardPosInfo(*target->GetPosInfo(), deltaMs, /*notifyAlways=*/true);
	}
}

// FSM 상태 전이 처리
void Monster::EnterState(AiState next, bool notify)
{
	if (_aiState == next) return;

	_aiState = next;

	// 클라이언트에게 보낼 애니메이션 상태 매핑
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

// [State: Return] 제자리 복귀 상태
void Monster::UpdateReturn(uint64 nowMs, uint64 deltaMs)
{
	auto room = GetGameRoom();
	if (!room) return;

	// 복귀 중에도 어그로 다시 끌리면 추격 재개
	if (auto t = room->FindNearestPlayer(GetPosInfo(), _searchRange))
	{
		_target = t;
		EnterState(AiState::Chase, /*notify=*/true);
		return;
	}

	// 스폰 위치 도착했는지 확인
	const float dx = _spawnPos.x() - _posInfo->x();
	const float dz = _spawnPos.z() - _posInfo->z();
	if ((dx * dx + dz * dz) <= (_arriveDist * _arriveDist))
	{
		EnterState(AiState::Idle, /*notify=*/true);
		return;
	}

	// 복귀 경로 이동 (LOS 체크 후 직선 이동 or 길찾기)
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
		MoveTowardPosInfo(_spawnPos, deltaMs, /*notifyAlways=*/true);
	}
}

// [State: Attack] 공격 상태
void Monster::UpdateAttack(uint64 nowMs, uint64 deltaMs)
{
	auto room = GetGameRoom();
	if (!room) return;

	// 공격 중에도 너무 멀리 끌려나왔는지 확인 (Leash)
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

	// 타겟 바라보기 (Yaw 회전)
	Vector3 dir = ObjectUtils::GetDirectionXZ(*_posInfo, *target->GetPosInfo());
	const float prevYaw = _posInfo->yaw();

	if (dir.x != 0.0f || dir.z != 0.0f)
	{
		float yawDeg = std::atan2(dir.x, dir.z) * 180.0f / 3.141592f;
		_posInfo->set_yaw(yawDeg);
	}

	// 타겟이 사거리 밖으로 도망가면 다시 추격 모드로
	const float distSqr = ObjectUtils::DistSqr(*_posInfo, *target->GetPosInfo());
	const float attackRangeSqr = _attackRange * _attackRange;

	if (distSqr > attackRangeSqr * 1.5f) // 약간의 히스테리시스(Hysteresis) 줌
	{
		EnterState(AiState::Chase, /*notify=*/true);
		return;
	}

	// 공격 모션 유지
	_posInfo->set_state(Protocol::MOVE_IDLE);
	_posInfo->set_actionstate(Protocol::ACTION_ATTACK);

	// 회전만 했어도 클라한테 알려줘야 자연스러움
	if (std::fabs(_posInfo->yaw() - prevYaw) > 0.5f)
		room->OnMonsterMoved(std::static_pointer_cast<Monster>(shared_from_this()));

	// 공격 쿨타임 체크 (1초)
	if (nowMs - _lastAttackMs < 1000)
		return;

	_lastAttackMs = nowMs;

	printf(" [Monster] Attack!\n");
	// 실제 스킬 피격 판정 요청
	room->HandleSkill(static_pointer_cast<Creature>(shared_from_this()), 1, _posInfo->yaw(), 0);
}

// 피격 시 어그로 반응
void Monster::OnDamaged(std::shared_ptr<Creature> attacker, int32 damage)
{
	Creature::OnDamaged(attacker, damage);

	if (!attacker) return;

	_target = attacker;

	// 맞으면 멍 때리지 말고 바로 쫓아감
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
		// 죽음 처리 Job (경험치 분배, 드랍 등)
		room->PushJob(&GameRoom::HandleMonsterDead, attacker, self);
	}
}


std::shared_ptr<Creature> Monster::GetTarget()
{
	std::shared_ptr<Creature> target = _target.lock();
	if (target == nullptr) return nullptr;
	if (target->GetStatInfo()->hp() <= 0) return nullptr; // 죽은 놈은 타겟 아님
	if (target->GetRoom() != GetRoom()) return nullptr;   // 방 나간 놈도 아님

	return target;
}
