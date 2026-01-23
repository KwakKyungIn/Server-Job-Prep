// Monster.h

#pragma once
#include "Creature.h"
#include "ObjectUtils.h" 

// 몬스터 AI 및 상태 관리를 담당하는 클래스
// Creature를 상속받아 기본 이동/스탯 기능을 공유함
class Monster : public Creature
{
public:
	Monster();
	virtual ~Monster();

	virtual void	OnDamaged(std::shared_ptr<Creature> attacker, int32 damage) override;
	virtual void	OnDead(std::shared_ptr<Creature> attacker) override;

	void			Init(int32 templateId);
	void			Update(uint64 nowMs, uint64 deltaMs); // AI 메인 업데이트

	Protocol::MonsterInfo* GetMonsterInfo() { return &_monsterInfo; }

	// AOI 최적화를 위한 시야(Viewer) 목록 관리
	HashSet<uint64>& Viewers_ActorOnly() { return _viewers; }
	const HashSet<uint64>& Viewers_ActorOnly() const { return _viewers; }

	// Lazy Update (비싼 AOI 갱신을 덜 자주 하기 위한 타임스탬프)
	uint64 GetLastAoiExpensiveMs() const { return _lastAoiExpensiveMs; }
	void   SetLastAoiExpensiveMs(uint64 v) { _lastAoiExpensiveMs = v; }
	void   SetLastAoiExpensivePos(float x, float z) { _lastAoiExpensiveX = x; _lastAoiExpensiveZ = z; }
	void   GetLastAoiExpensivePos(float& x, float& z) const { x = _lastAoiExpensiveX; z = _lastAoiExpensiveZ; }

private:
	// ===============================
	// [AI FSM] Finite State Machine
	// ===============================
	enum class AiState : uint8_t
	{
		Idle,   // 대기/배회
		Chase,  // 추격 (Path Finding)
		Attack, // 공격
		Return, // 복귀 (스폰 위치로)
	};

	void UpdateIdle(uint64 nowMs);
	void UpdateChase(uint64 nowMs, uint64 deltaMs);
	void UpdateAttack(uint64 nowMs, uint64 deltaMs);
	void UpdateReturn(uint64 nowMs, uint64 deltaMs);

	// AI 헬퍼 함수들
	std::shared_ptr<Creature> GetTarget();
	void EnterState(AiState next, bool notify);

	bool MoveTowardPosInfo(const Protocol::PositionInfo& dst, uint64 deltaMs, bool notifyAlways = true);
	bool RebuildPathTo(const Protocol::PositionInfo& dst, uint64 nowMs);
	bool FollowPath(uint64 nowMs, uint64 deltaMs, const Protocol::PositionInfo& finalDst);

	float Dist2DSqr(const Protocol::PositionInfo& a, const Protocol::PositionInfo& b) const;
	float Dist2DSqrToSpawn() const;

private:
	Protocol::MonsterInfo _monsterInfo;

	// 타겟 및 사거리 설정
	std::weak_ptr<Creature> _target;
	float _searchRange = 10.0f; // 인식 범위
	float _attackRange = 1.5f;  // 공격 사거리

	// 활동 반경 제한 (Leash)
	float _leashRange = 25.0f;     // 스폰 위치 기준 이 범위를 벗어나면 복귀함
	float _arriveDist = 0.8f;      // 복귀 완료 판정 거리

	// 현재 AI 상태
	AiState _aiState = AiState::Idle;

	// 초기 스폰 위치 (복귀용)
	bool _spawnInited = false;
	Protocol::PositionInfo _spawnPos;

	// 길찾기 관련 변수
	Vector<Vector3> _path; // 경로 웨이포인트 목록
	int _pathIndex = 0;    // 현재 목표 웨이포인트 인덱스
	uint64 _lastRepathMs = 0; // 마지막 경로 계산 시간

	// 끼임(Stuck) 감지 변수
	uint64 _stuckAccumMs = 0;
	float _lastProgressX = 0.f;
	float _lastProgressZ = 0.f;

	// AI 튜닝 파라미터
	uint64 _repathIntervalMs = 350; // 경로 재계산 주기 (너무 짧으면 CPU 부하, 길면 멍청해짐)
	uint64 _stuckMs = 700;          // 0.7초 동안 제자리 걸음이면 Stuck 판정
	float  _waypointArriveDist = 0.6f; // 웨이포인트 도착 인정 오차
	float  _stuckEps = 0.02f;       // 이동량이 이보다 적으면 안 움직인 걸로 간주

	HashSet<uint64> _viewers; // 나를 보고 있는 플레이어들

	uint64 _lastAttackMs = 0; // 공격 쿨타임 체크

	// AOI 최적화용 캐시
	uint64 _lastAoiExpensiveMs = 0;
	float  _lastAoiExpensiveX = 0.f;
	float  _lastAoiExpensiveZ = 0.f;
};