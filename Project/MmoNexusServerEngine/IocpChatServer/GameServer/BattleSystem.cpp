#include "pch.h"
#include "BattleSystem.h"
#include "SpatialGrid.h"
#include "Zone.h"
#include "Creature.h"
#include "DataManager.h"
#include "ObjectUtils.h"
#include "Protocol.pb.h"
#include "Player.h"
#include "Monster.h"


BattleSystem::BattleSystem(SpatialGrid* grid)
    : _grid(grid)
{
}

// 스킬 사용 시 실제로 누가 맞았는지 판정하는 핵심 로직
// 여기서 데미지 계산까지 끝내서 결과 구조체에 담아준다
bool BattleSystem::ResolveSkill(const std::shared_ptr<Creature>& attacker,
    int32 skillId,
    float castYaw,
    SkillResult& outResult)
{
    if (_grid == nullptr || attacker == nullptr)
        return false;

    // 데이터 시트에서 스킬 정보를 먼저 가져옴
    const Protocol::SkillTemplateInfo* skillData =
        DataManager::Instance()->GetSkillTemplate(skillId);
    if (skillData == nullptr)
        return false;

    Protocol::SkillType type = skillData->skilltype();
    float               range = skillData->range();
    float               radius = skillData->radius();
    float               angle = skillData->angle();
    int32               damage = skillData->damage();

    if (range < 0.0f) range = 0.0f;
    if (radius < 0.0f) radius = 0.0f;
    if (angle < 0.0f) angle = 0.0f;

    auto pickPrimaryOrFallback = [](float primary, float fallback) -> float
        {
            if (primary > 0.0f)
                return primary;
            return fallback > 0.0f ? fallback : 0.0f;
        };

    const float singleRange = pickPrimaryOrFallback(range, radius);
    const float circleRadius = pickPrimaryOrFallback(radius, range);
    const float coneRange = pickPrimaryOrFallback(radius, range);
    float coneAngle = angle;
    if (coneAngle <= 0.0f)
        coneAngle = 90.0f;

    Vector3 viewDir = ObjectUtils::GetVectorFromYaw(castYaw);

    bool isMonster = (attacker->GetObjectType() == Protocol::OBJECT_TYPE_MONSTER);

    // 공격자가 현재 위치한 존의 인덱스를 구함
    // 이 인덱스를 기준으로 주변 존을 탐색할 예정
    int32 zoneIndex = _grid->GetZoneIndex(*attacker->GetPosInfo());

    // 주변 존에 있는 모든 잠재적 타겟들을 먼저 긁어모음 (Broad Phase)
    Vector<std::shared_ptr<Creature>> candidates;
    if (!CollectCandidates(attacker, isMonster, zoneIndex, candidates))
        return false;

    // 실제로 사거리 안에 들어왔는지 정밀 검사 (Narrow Phase)
    outResult.skillId = skillId;
    outResult.zoneIndex = zoneIndex;
    outResult.hits.clear();

    for (auto& victim : candidates)
    {
        // 이미 죽은 대상은 타겟에서 제외
        if (victim->GetStatInfo()->hp() <= 0)
            continue;

        bool isHit = false;

        switch (type)
        {
        case Protocol::SKILL_AUTO:
        {
            // 평타나 타겟팅 스킬은 원형 범위 체크로 단순하게 판정
            if (ObjectUtils::CheckCircle(*attacker->GetPosInfo(),
                singleRange,
                *victim->GetPosInfo()))
            {
                isHit = true;
            }
        }
        break;
        case Protocol::SKILL_AREA_CIRCLE:
        {
            if (ObjectUtils::CheckCircle(*attacker->GetPosInfo(),
                circleRadius,
                *victim->GetPosInfo()))
            {
                isHit = true;
            }
        }
        break;
        case Protocol::SKILL_AREA_CONE:
        {
            if (ObjectUtils::CheckFan(*attacker->GetPosInfo(),
                viewDir,
                coneRange,
                coneAngle,
                *victim->GetPosInfo()))
            {
                isHit = true;
            }
        }
        break;

        // 나중에 논타겟팅 스킬이나 범위 스킬 구현할 때 여기 추가해야 함
        default:
            break;
        }

        if (!isHit)
            continue;

        // 피격 판정이 났으므로 실제 객체에 데미지 적용
        victim->OnDamaged(attacker, damage);

        HitInfo info;
        info.target = victim;
        info.damage = damage;
        outResult.hits.push_back(info);

        // 오토 스킬은 단일 타겟이라 한 명 맞으면 바로 루프 종료
        if (type == Protocol::SKILL_AUTO)
            break;
    }

    return true;
}

// AOI 그리드를 이용해서 주변 존에 있는 플레이어나 몬스터 목록을 가져옴
// 전체 맵을 다 뒤지면 느리니까 내 주변 9개 존만 검사하도록 최적화함
bool BattleSystem::CollectCandidates(const std::shared_ptr<Creature>& /*attacker*/,
    bool isMonster,
    int32 zoneIndex,
    Vector<std::shared_ptr<Creature>>& outCandidates)
{
    outCandidates.clear();
    if (_grid == nullptr)
        return false;

    Vector<Zone*> zones;
    _grid->GetNearbyZones(zoneIndex, zones);

    for (Zone* zone : zones)
    {
        if (zone == nullptr) continue;

        // 몬스터가 공격자면 플레이어를 찾고
        // 플레이어가 공격자면 몬스터를 찾아서 후보군에 등록
        if (isMonster)
        {
            for (auto& p : zone->players)
                outCandidates.push_back(p);
        }
        else
        {
            for (auto& m : zone->monsters)
                outCandidates.push_back(m);
        }
    }

    return true;
}
