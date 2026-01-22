#pragma once
#include "Creature.h"
#include "Protocol.pb.h"
#include <unordered_set>

class Projectile : public Creature
{
public:
    Projectile();
    virtual ~Projectile();

    void Init(uint64 ownerId, int32 skillId,
        const Protocol::PositionInfo& startPos,
        float speed, uint32 lifeTimeMs, float range);

    Protocol::ProjectileInfo* GetProjectileInfo() { return &_projInfo; }
    const Protocol::ProjectileInfo* GetProjectileInfo() const { return &_projInfo; }

    uint64 GetOwnerId() const { return _projInfo.ownerid(); }
    int32  GetSkillId()  const { return _projInfo.skillid(); }

    HashSet<uint64>& Viewers_ActorOnly() { return _viewers; }
    const HashSet<uint64>& Viewers_ActorOnly() const { return _viewers; }

    // ===== [C] Combat Params (runtime only) =====
    void SetCombatParams(float hitRadius, bool stopOnHit, int32 maxHits)
    {
        _hitRadius = hitRadius;
        _stopOnHit = stopOnHit;
        _maxHits = (maxHits <= 0 ? 1 : maxHits);
    }

    float HitRadius() const { return _hitRadius; }
    bool  StopOnHit() const { return _stopOnHit; }
    int32 MaxHits() const { return (_maxHits <= 0 ? 1 : _maxHits); }
    int32 HitCount() const { return _hitCount; }

    bool CanHitMore() const { return _hitCount < MaxHits(); }

    bool HasAlreadyHit(uint64 victimNetId) const
    {
        return _hitVictims.find(victimNetId) != _hitVictims.end();
    }

    void MarkHit(uint64 victimNetId)
    {
        _hitVictims.insert(victimNetId);
        _hitCount++;
    }

    void ResetHitState()
    {
        _hitVictims.clear();
        _hitCount = 0;
    }

    // A단계: 최소 이동/만료만 제공(충돌/피해는 C가 확장)
    bool Update(uint64 deltaMs);   // true면 pos가 변했다
    bool IsExpired() const;

private:
    Protocol::ProjectileInfo _projInfo;
    HashSet<uint64> _viewers;

    uint64 _elapsedMs = 0;
    float  _traveled = 0.0f;
    float  _range = 0.0f;

    // ===== [C] hit runtime state =====
    float _hitRadius = 0.0f;
    bool  _stopOnHit = true;
    int32 _maxHits = 1;
    int32 _hitCount = 0;
    HashSet<uint64> _hitVictims;
};
