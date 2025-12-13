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

bool BattleSystem::ResolveSkill(const std::shared_ptr<Creature>& attacker,
    int32 skillId,
    SkillResult& outResult)
{
    if (_grid == nullptr || attacker == nullptr)
        return false;

    // 1. 스킬 데이터
    const Protocol::SkillTemplateInfo* skillData =
        DataManager::Instance()->GetSkillTemplate(skillId);
    if (skillData == nullptr)
        return false;

    Protocol::SkillType type = skillData->skilltype();
    float               range = skillData->range();
    int32               damage = skillData->damage();

    bool isMonster = (attacker->GetObjectType() == Protocol::OBJECT_TYPE_MONSTER);

    // 2. 기준 존 계산
    int32 zoneIndex = _grid->GetZoneIndex(*attacker->GetPosInfo());

    // 3. AOI로 후보 수집
    Vector<std::shared_ptr<Creature>> candidates;
    if (!CollectCandidates(attacker, isMonster, zoneIndex, candidates))
        return false;

    // 4. Narrow Phase + 데미지 적용
    outResult.skillId = skillId;
    outResult.zoneIndex = zoneIndex;
    outResult.hits.clear();

    for (auto& victim : candidates)
    {
        if (victim->GetStatInfo()->hp() <= 0)
            continue;

        bool isHit = false;

        switch (type)
        {
        case Protocol::SKILL_AUTO:
        {
            if (ObjectUtils::CheckCircle(*attacker->GetPosInfo(),
                range,
                *victim->GetPosInfo()))
            {
                isHit = true;
            }
        }
        break;

        // TODO: 다른 스킬 타입들 추가 (부채꼴, 라인 등)
        default:
            break;
        }

        if (!isHit)
            continue;

        // 실제 데미지 적용 (도메인)
        victim->OnDamaged(attacker, damage);

        HitInfo info;
        info.target = victim;
        info.damage = damage;
        outResult.hits.push_back(info);

        // 단일 타겟 스킬이면 첫 명중 후 종료
        if (type == Protocol::SKILL_AUTO)
            break;
    }

    return true;
}

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
