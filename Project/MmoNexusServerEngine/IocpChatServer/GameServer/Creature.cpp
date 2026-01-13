#include "pch.h"
#include "Creature.h"
#include "GameRoom.h"
#include "DataManager.h"
#include "ClientPacketHandler.h"

// 초기값 1부터 시작
std::atomic<uint64> Creature::s_idGenerator = 1;

Creature::Creature(Protocol::ObjectType type) : _objectType(type)
{
	// [ID Generation Logic]
	// 1. 카운터 증가 (1, 2, 3...)
	uint64 id = s_idGenerator.fetch_add(1);

	// 2. 타입 비트 삽입 (상위 8비트로 이동)
	// 예: OBJECT_TYPE_PLAYER(1) << 56 -> 0x0100000000000000
	// 최종 ID = (타입 정보) | (일련 번호)
	_objectId = ((uint64)_objectType << 56) | id;
}

Creature::~Creature()
{
}

void Creature::OnDamaged(std::shared_ptr<Creature> attacker, int32 damage)
{
	if (_statInfo == nullptr) return;

	int32 currentHp = _statInfo->hp();
	int32 finalHp = currentHp - damage;
	if (finalHp < 0) finalHp = 0;

	_statInfo->set_hp(finalHp);

	if (finalHp <= 0)
	{
		OnDead(attacker);
	}
}

void Creature::OnDead(std::shared_ptr<Creature> attacker)
{
	// TODO: 사망 처리
}

bool Creature::CanUseSkill(int32 skillId)
{
    // 1. 데이터 존재 여부
    const Protocol::SkillTemplateInfo* skillData = DataManager::Instance()->GetSkillTemplate(skillId);
    if (skillData == nullptr)
        return false;

    // 2. 쿨타임 체크
    uint64 now = ::GetTickCount64();
    auto findIt = _cooldowns.find(skillId);
    if (findIt != _cooldowns.end())
    {
        if (findIt->second > now)
            return false; // 아직 쿨 안 돔
    }

    // 3. 상태 체크 (기절, 침묵 등 - 나중에 추가)
    // if (_state == STUN) return false;

    return true;
}

void Creature::UseSkill(int32 skillId)
{
    if (CanUseSkill(skillId) == false)
        return;

    // 1. 스킬 데이터 가져오기 (쿨타임용)
    const Protocol::SkillTemplateInfo* skillData =
        DataManager::Instance()->GetSkillTemplate(skillId);
    if (skillData == nullptr)
        return;

    // 2. 쿨타임 적용
    uint64 now = ::GetTickCount64();
    _cooldowns[skillId] = now + skillData->cooldown();

    // 3. 룸에 "이 스킬 썼다" 요청만 던짐 (브로드캐스트/판정은 룸 책임)
    if (auto room = GetRoom())
    {
        // Lobby에서는 스킬 처리 금지
        if (room->GetKind() != RoomKind::Game)
            return;

        auto gr = std::dynamic_pointer_cast<GameRoom>(room);
        if (!gr)
            return;

        auto self = shared_from_this();
        gr->Push([gr, self, skillId]()
            {
                float yaw = 0.f;
                if (self->GetPosInfo())
                    yaw = self->GetPosInfo()->yaw();

                gr->HandleSkill(self, skillId, yaw, 0);
            });
    }


    printf("⚔️ [Skill] %s used Skill %d (Cooldown: %dms)\n",
        (GetObjectType() == Protocol::OBJECT_TYPE_PLAYER ? "Player" : "Monster"),
        skillId, skillData->cooldown());
}
