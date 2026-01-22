#pragma once
class Creature;
class SpatialGrid;
struct Zone;

///////////////////////////////////////////////////////////////////////////////
// 전투 결과 타입
///////////////////////////////////////////////////////////////////////////////

struct HitInfo
{
    std::shared_ptr<Creature> target;
    int32                     damage = 0;
};

struct SkillResult
{
    int32        skillId = 0;
    int32        zoneIndex = -1;  // 브로드캐스트 기준 존
    Vector<HitInfo> hits;         // 맞은 애 목록
};

///////////////////////////////////////////////////////////////////////////////
// BattleSystem : "누구를 얼마나 때릴지"만 계산하는 순수 전투 엔진
///////////////////////////////////////////////////////////////////////////////

class BattleSystem
{
public:
    explicit BattleSystem(SpatialGrid* grid);
    ~BattleSystem() = default;

    // 스킬 1번 사용에 대한 전투 판정
    // - attacker : 시전자
    // - skillId  : 사용한 스킬
    // - outResult: 누가 맞았고, 얼마 맞았는지 결과
    bool ResolveSkill(const std::shared_ptr<Creature>& attacker,
        int32 skillId,
        SkillResult& outResult);

private:
    bool CollectCandidates(const std::shared_ptr<Creature>& attacker,
        bool isMonster,
        int32 zoneIndex,
        Vector<std::shared_ptr<Creature>>& outCandidates);

private:
    SpatialGrid* _grid = nullptr; // AOI 접근용 (소유 X)
};
