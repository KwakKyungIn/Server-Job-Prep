#pragma once
#include "Creature.h"
#include "Protocol.pb.h"
#include <unordered_set>

// 투사체 클래스 (화살, 마법 구체 등)
// Creature를 상속받아 위치 동기화 및 기본적인 객체 관리 기능을 공유함
// 플레이어나 몬스터와 달리 AI나 복잡한 상태 머신은 없고, 단순 이동과 충돌만 처리
class Projectile : public Creature
{
public:
    Projectile();
    virtual ~Projectile();

    // 풀링(Pooling)을 위해 생성자 대신 Init으로 초기화
    // 소유자(Owner), 스킬 ID, 속도, 수명, 사거리 등을 설정
    void Init(uint64 ownerId, int32 skillId,
        const Protocol::PositionInfo& startPos,
        float speed, uint32 lifeTimeMs, float range);

    Protocol::ProjectileInfo* GetProjectileInfo() { return &_projInfo; }
    const Protocol::ProjectileInfo* GetProjectileInfo() const { return &_projInfo; }

    uint64 GetOwnerId() const { return _projInfo.ownerid(); }
    int32  GetSkillId()  const { return _projInfo.skillid(); }

    // 투사체를 보고 있는 유저 목록 (AOI)
    // 투사체는 시야 거리가 짧거나 이동이 빨라서 별도의 관리가 필요할 수 있음
    HashSet<uint64>& Viewers_ActorOnly() { return _viewers; }
    const HashSet<uint64>& Viewers_ActorOnly() const { return _viewers; }

    // ===== [C] Combat Params (runtime only) =====
    // 투사체마다 충돌 특성이 다름 (ex: 관통 여부, 피격 범위)
    // 데이터 시트에서 읽어온 값을 여기에 세팅함
    void SetCombatParams(float hitRadius, bool stopOnHit, int32 maxHits)
    {
        _hitRadius = hitRadius;
        _stopOnHit = stopOnHit; // true면 한 명 맞고 소멸, false면 관통
        _maxHits = (maxHits <= 0 ? 1 : maxHits); // 최대 몇 명까지 맞출 수 있는지
    }

    float HitRadius() const { return _hitRadius; }
    bool  StopOnHit() const { return _stopOnHit; }
    int32 MaxHits() const { return (_maxHits <= 0 ? 1 : _maxHits); }
    int32 HitCount() const { return _hitCount; }

    // 더 때릴 수 있는지 체크 (관통 스킬용)
    bool CanHitMore() const { return _hitCount < MaxHits(); }

    // 중복 피격 방지 로직
    // 0.1초 사이에 같은 투사체가 같은 대상을 두 번 때리는 버그를 막기 위해
    // 이미 맞은 대상의 ID를 기록해둠
    bool HasAlreadyHit(uint64 victimNetId) const
    {
        return _hitVictims.find(victimNetId) != _hitVictims.end();
    }

    // 피격 성공 시 기록
    void MarkHit(uint64 victimNetId)
    {
        _hitVictims.insert(victimNetId);
        _hitCount++;
    }

    // 재사용을 위한 상태 초기화
    void ResetHitState()
    {
        _hitVictims.clear();
        _hitCount = 0;
    }

    // A단계: 이동 로직만 담당
    // 충돌 판정은 Room 스레드에서 별도로 돌리고, 여기서는 좌표만 갱신함
    bool Update(uint64 deltaMs);   // true면 위치가 변했다는 뜻
    bool IsExpired() const;

private:
    Protocol::ProjectileInfo _projInfo;
    HashSet<uint64> _viewers;

    uint64 _elapsedMs = 0;
    float  _traveled = 0.0f;
    float  _range = 0.0f;

    // ===== [C] 피격 판정용 런타임 변수들 =====
    float _hitRadius = 0.0f;
    bool  _stopOnHit = true;
    int32 _maxHits = 1;
    int32 _hitCount = 0;

    // 피격된 대상 목록 (중복 피격 방지용)
    HashSet<uint64> _hitVictims;
};