#pragma once
#include "Creature.h"

class Monster : public Creature
{
public:
	Monster();
	virtual ~Monster();

	// [Override]
	virtual void	OnDamaged(std::shared_ptr<Creature> attacker, int32 damage) override;
	virtual void	OnDead(std::shared_ptr<Creature> attacker) override;

	// [Logic]
	void			Init(int32 templateId); // 생성 후 데이터 세팅
	void			Update(); // AI 메인 루프 (GameRoom에서 주기적으로 호출)

	// [Data Access] (New)
	// GameRoom에서 S_SPAWN 패킷 만들 때 가져가야 함
	Protocol::MonsterInfo* GetMonsterInfo() { return &_monsterInfo; }

private:
	// [AI State Machine]
	void			UpdateIdle();
	void			UpdateMove();
	void			UpdateAttack();

	// [Helper]
	std::shared_ptr<Creature> GetTarget(); // 현재 타겟 유효성 검증

private:
	// [Data Container]
	Protocol::MonsterInfo _monsterInfo;

	// [AI Context]
	std::weak_ptr<Creature> _target; // 현재 쫓고 있는 대상
	float		_searchRange = 10.0f; // 인식 범위
	float		_attackRange = 1.5f;  // 공격 사거리
};