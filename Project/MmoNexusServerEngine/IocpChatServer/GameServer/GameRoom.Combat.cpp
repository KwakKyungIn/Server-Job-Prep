#include "pch.h"
#include "GameRoom.h"
#include "GameMap.h"
#include "Player.h"
#include "RoomManager.h"
#include "GameRoom.Net.h"
#include "Projectile.h"


void GameRoom::HandleSkill(std::shared_ptr<Creature> attacker, int32 skillId, float castYaw, uint32 clientTimeMs)
{
    if (attacker == nullptr)
        return;

    (void)clientTimeMs;

    // 방 검증
    if (attacker->GetRoom().get() != this)
        return;

    // 스킬 데이터
    const Protocol::SkillTemplateInfo* skillData = DataManager::Instance()->GetSkillTemplate(skillId);
    if (skillData == nullptr)
        return;

    const Protocol::SkillType type = skillData->skilltype();

    // =========================================================
    //  [C] PROJECTILE: 즉시 데미지 금지 -> Projectile 생성
    // =========================================================
    if (type == Protocol::SKILL_PROJECTILE)
    {
        // 1) 모션 브로드캐스트
        const int32 zoneIndex = _grid.GetZoneIndex(*attacker->GetPosInfo());
        {
            Protocol::S_SKILL skillPkt;
            skillPkt.set_objectid(NetId(attacker));
            skillPkt.set_skillid(skillId);
            skillPkt.set_cooldownms(skillData->cooldown());

            SendBufferRef skillBuffer = ClientPacketHandler::MakeSendBuffer(skillPkt);
            BroadcastToZone(skillBuffer, zoneIndex);
        }

        // 2) Projectile 파라미터 (Data 기반)
        float speed = skillData->projectilespeed();
        uint32 lifeMs = (uint32)skillData->projectilelifems();

        float hitRadius = skillData->hitradius();
        if (hitRadius <= 0.0f)
            hitRadius = skillData->radius(); // fallback

        bool stopOnHit = skillData->stoponhit();
        int32 maxHits = skillData->maxhits();
        if (maxHits <= 0) maxHits = 1;

        // travel range 정책 (일단 skillData.range 재사용)
        float range = skillData->range();
        if (range <= 0.0f && lifeMs > 0 && speed > 0.0f)
            range = speed * ((float)lifeMs / 1000.0f);

        // 방어: life/range 둘 다 0이면 “무한” 방지
        if (lifeMs == 0 && range <= 0.0f)
            lifeMs = 800; // 기본 0.8s

        // 3) 시작 포지션/방향
        Protocol::PositionInfo startPos = *attacker->GetPosInfo();

        // 플레이어는 client castYaw를 권위 입력으로 사용
        if (attacker->GetObjectType() == Protocol::OBJECT_TYPE_PLAYER)
            startPos.set_yaw(castYaw);

        // 몬스터는 서버 pos.yaw가 권위
        // (castYaw는 무시해도 됨)

        // 4) ownerId 규칙: playerId / monster objectId
        uint64 ownerId = 0;
        if (attacker->GetObjectType() == Protocol::OBJECT_TYPE_PLAYER)
            ownerId = std::static_pointer_cast<Player>(attacker)->GetPlayerId();
        else
            ownerId = attacker->GetObjectId();

        // 5) 생성 + 룸 등록
        ProjectileRef p = std::make_shared<Projectile>();
        p->Init(ownerId, skillId, startPos, speed, lifeMs, range);
        p->SetCombatParams(hitRadius, stopOnHit, maxHits);

        EnterProjectile(p);
        return;
    }

    // =========================================================
    // 기존: 즉시 판정 스킬 (AUTO 등)
    // =========================================================
    if (_battle == nullptr)
        return;

    SkillResult result;
    if (_battle->ResolveSkill(attacker, skillId, result) == false)
        return;

    // 스킬 모션 브로드캐스트
    {
        Protocol::S_SKILL skillPkt;
        skillPkt.set_objectid(NetId(attacker));
        skillPkt.set_skillid(skillId);
        skillPkt.set_cooldownms(skillData->cooldown());

        SendBufferRef skillBuffer = ClientPacketHandler::MakeSendBuffer(skillPkt);
        BroadcastToZone(skillBuffer, result.zoneIndex);
    }

    // 피격 결과 브로드캐스트 (HP 변경)
    for (const HitInfo& hit : result.hits)
    {
        auto victim = hit.target;
        if (victim == nullptr) continue;

        Protocol::S_CHANGE_HP changePkt;
        changePkt.set_objectid(NetId(victim));
        changePkt.set_attackerid(NetId(attacker));
        changePkt.set_currenthp(victim->GetStatInfo()->hp());
        changePkt.set_damage(hit.damage);

        SendBufferRef sendBuffer = ClientPacketHandler::MakeSendBuffer(changePkt);
        BroadcastToZone(sendBuffer, result.zoneIndex);
    }
}

//  기존 버전 유지 (다른 호출부 안 깨지게)
//    내부적으로 NEW 버전으로 포워딩
void GameRoom::HandleSkill(std::shared_ptr<Creature> attacker, int32 skillId)
{
    HandleSkill(attacker, skillId, /*castYaw=*/0.f, /*clientTimeMs=*/0);
}


//  NEW: 확장 버전
void GameRoom::HandleSkillById(PlayerSessionRef session, uint64 playerId, int32 skillId, float castYaw, uint32 clientTimeMs)
{
    auto it = _players.find(playerId);
    if (it == _players.end())
        return;

    PlayerRef player = it->second;
    if (!player) return;

    std::shared_ptr<Creature> attacker = std::static_pointer_cast<Creature>(player);
    HandleSkill(attacker, skillId, castYaw, clientTimeMs);
}

//  기존 버전 유지 (기존 호출부 안 깨지게)
void GameRoom::HandleSkillById(PlayerSessionRef session, uint64 playerId, int32 skillId)
{
    HandleSkillById(session, playerId, skillId, /*castYaw=*/0.f, /*clientTimeMs=*/0);
}
