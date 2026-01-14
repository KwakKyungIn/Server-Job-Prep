// Monster.h

#pragma once
#include "Creature.h"
#include "ObjectUtils.h" 
class Monster : public Creature
{
public:
	Monster();
	virtual ~Monster();

	virtual void	OnDamaged(std::shared_ptr<Creature> attacker, int32 damage) override;
	virtual void	OnDead(std::shared_ptr<Creature> attacker) override;

	void			Init(int32 templateId);
	void			Update(uint64 nowMs, uint64 deltaMs);

	Protocol::MonsterInfo* GetMonsterInfo() { return &_monsterInfo; }

	std::unordered_set<uint64>& Viewers_ActorOnly() { return _viewers; }
	const std::unordered_set<uint64>& Viewers_ActorOnly() const { return _viewers; }

	uint64 GetLastAoiExpensiveMs() const { return _lastAoiExpensiveMs; }
	void   SetLastAoiExpensiveMs(uint64 v) { _lastAoiExpensiveMs = v; }
	void   SetLastAoiExpensivePos(float x, float z) { _lastAoiExpensiveX = x; _lastAoiExpensiveZ = z; }
	void   GetLastAoiExpensivePos(float& x, float& z) const { x = _lastAoiExpensiveX; z = _lastAoiExpensiveZ; }

private:
	// ===============================
	// [C] Server-only AI FSM
	// ===============================
	enum class AiState : uint8_t
	{
		Idle,
		Chase,
		Attack,
		Return,
	};

	void UpdateIdle(uint64 nowMs);
	void UpdateChase(uint64 nowMs, uint64 deltaMs);
	void UpdateAttack(uint64 nowMs, uint64 deltaMs);
	void UpdateReturn(uint64 nowMs, uint64 deltaMs);

	// helpers
	std::shared_ptr<Creature> GetTarget();
	void EnterState(AiState next, bool notify);

	bool MoveTowardPosInfo(const Protocol::PositionInfo& dst, uint64 deltaMs, bool notifyAlways = true);
	bool RebuildPathTo(const Protocol::PositionInfo& dst, uint64 nowMs);
	bool FollowPath(uint64 nowMs, uint64 deltaMs, const Protocol::PositionInfo& finalDst);

	float Dist2DSqr(const Protocol::PositionInfo& a, const Protocol::PositionInfo& b) const;
	float Dist2DSqrToSpawn() const;

private:
	Protocol::MonsterInfo _monsterInfo;

	// target / ranges
	std::weak_ptr<Creature> _target;
	float _searchRange = 10.0f;
	float _attackRange = 1.5f;

	// leash/return
	float _leashRange = 25.0f;     // 스폰 기준 리쉬
	float _arriveDist = 0.8f;      // 복귀 완료 판정(2D)

	// fsm
	AiState _aiState = AiState::Idle;

	// spawn pos cache
	bool _spawnInited = false;
	Protocol::PositionInfo _spawnPos;

	// path-follow
	std::vector<Vector3> _path;
	int _pathIndex = 0;
	uint64 _lastRepathMs = 0;

	// stuck detection
	uint64 _stuckAccumMs = 0;
	float _lastProgressX = 0.f;
	float _lastProgressZ = 0.f;

	// tuning
	uint64 _repathIntervalMs = 350; // 250~500ms 권장
	uint64 _stuckMs = 700;          // 이 이상 거의 안 움직이면 stuck
	float  _waypointArriveDist = 0.6f;
	float  _stuckEps = 0.02f;       // 2cm

	std::unordered_set<uint64> _viewers;

	uint64 _lastAttackMs = 0;

	uint64 _lastAoiExpensiveMs = 0;
	float  _lastAoiExpensiveX = 0.f;
	float  _lastAoiExpensiveZ = 0.f;
};
