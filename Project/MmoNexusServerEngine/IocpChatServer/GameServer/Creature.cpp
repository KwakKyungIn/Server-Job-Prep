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

    // 1. 스킬 데이터 가져오기
    const Protocol::SkillTemplateInfo* skillData = DataManager::Instance()->GetSkillTemplate(skillId);

    // 2. 쿨타임 적용
    uint64 now = ::GetTickCount64();
    _cooldowns[skillId] = now + skillData->cooldown();

    // 3. [Broadcast] "나 스킬 썼음" 알림 (모션용)
    // 데미지 판정과는 별개로, 모션은 바로 보여줘야 반응성이 좋음
    std::shared_ptr<GameRoom> room = GetRoom();

    if (room)
    {
        Protocol::S_SKILL skillPkt;
        skillPkt.set_objectid(GetObjectId());
        skillPkt.set_skillid(skillId);
        auto sendBuffer = ClientPacketHandler::MakeSendBuffer(skillPkt);
        room->Broadcast(sendBuffer); // 혹은 BroadcastToZone
    }

    // 4. [Hit Detection] 피격 판정 및 데미지 적용
    if (room)
    {
        // 룸에게 판정 위임 (동기화된 룸 스레드에서 실행됨)
        // 람다 캡처로 안전하게 전달
        auto self = shared_from_this();
        room->PushJob([room, self, skillId]()
            {
                room->HandleSkill(self, skillId);
            });
    }

    printf("⚔️ [Skill] %s used Skill %d (Cooldown: %dms)\n",
        (GetObjectType() == Protocol::OBJECT_TYPE_PLAYER ? "Player" : "Monster"),
        skillId, skillData->cooldown());
}