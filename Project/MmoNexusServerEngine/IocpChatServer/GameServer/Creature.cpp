#include "pch.h"
#include "Creature.h"
#include "GameRoom.h"
#include "DataManager.h"
#include "ClientPacketHandler.h"

// ID 생성기는 1부터 시작 (0은 유효하지 않은 ID로 간주)
std::atomic<uint64> Creature::s_idGenerator = 1;

Creature::Creature(Protocol::ObjectType type) : _objectType(type)
{
    // [ID Generation Logic]
    // 멀티스레드 환경에서도 중복되지 않는 고유 ID를 생성하는 로직
    // 1. atomic 변수를 사용하여 스레드 안전하게 카운터를 1 증가시킴
    uint64 id = s_idGenerator.fetch_add(1);

    // 2. 타입 정보를 ID 자체에 비트 연산으로 포함시킴
    // 상위 8비트에 ObjectType을 넣고, 하위 비트에 일련번호를 넣는 방식
    // 이렇게 하면 ID만 봐도 이게 플레이어인지 몬스터인지 바로 알 수 있어서 유용하다
    // 예: OBJECT_TYPE_PLAYER(1) << 56 -> 0x01
    _objectId = ((uint64)_objectType << 56) | id;
}

Creature::~Creature()
{
}

// 데미지를 입었을 때 호출되는 함수
// 체력을 깎고 0 이하가 되면 사망 처리를 호출한다
void Creature::OnDamaged(std::shared_ptr<Creature> attacker, int32 damage)
{
    if (_statInfo == nullptr) return;

    int32 currentHp = _statInfo->hp();
    int32 finalHp = currentHp - damage;

    // 체력이 음수가 되지 않도록 보정
    if (finalHp < 0) finalHp = 0;

    _statInfo->set_hp(finalHp);

    if (finalHp <= 0)
    {
        OnDead(attacker);
    }
}

void Creature::OnDead(std::shared_ptr<Creature> attacker)
{
    // 사망 시 경험치 분배나 아이템 드랍 등은 자식 클래스나 매니저에서 처리
}

// 스킬을 사용할 수 있는 상태인지 검사하는 함수
// 데이터 존재 여부와 쿨타임을 주로 체크한다
bool Creature::CanUseSkill(int32 skillId)
{
    // 1. 데이터 시트에 존재하는 스킬인지 확인
    const Protocol::SkillTemplateInfo* skillData = DataManager::Instance()->GetSkillTemplate(skillId);
    if (skillData == nullptr)
        return false;

    // 2. 현재 시간과 쿨타임 만료 시간을 비교
    uint64 now = ::GetTickCount64();
    auto findIt = _cooldowns.find(skillId);
    if (findIt != _cooldowns.end())
    {
        if (findIt->second > now)
            return false; // 아직 쿨타임이 도는 중
    }

    // 3. 기절이나 침묵 같은 상태 이상 체크는 추후 구현 예정
    // if (_state == STUN) return false;

    return true;
}

// 스킬 사용을 요청하는 메인 함수
// 여기서 직접 스킬 로직을 돌리는 게 아니라, GameRoom의 JobQueue에 작업을 넣는다
void Creature::UseSkill(int32 skillId)
{
    // 사용 조건 1차 검증
    if (CanUseSkill(skillId) == false)
        return;

    const Protocol::SkillTemplateInfo* skillData =
        DataManager::Instance()->GetSkillTemplate(skillId);
    if (skillData == nullptr)
        return;

    // 쿨타임 갱신 (서버 기준 시간)
    uint64 now = ::GetTickCount64();
    _cooldowns[skillId] = now + skillData->cooldown();

    // 실제 스킬 판정은 Room 스레드에서 안전하게 처리해야 하므로 작업을 위임함
    // 이렇게 해야 여러 명이 동시에 스킬을 써도 동기화 문제가 발생하지 않음
    if (auto room = GetRoom())
    {
        // 로비에서는 스킬 사용 불가
        if (room->GetKind() != RoomKind::Game)
            return;

        auto gr = std::dynamic_pointer_cast<GameRoom>(room);
        if (!gr)
            return;

        // 람다 캡처로 self를 잡아두어 작업이 끝날 때까지 객체가 소멸되지 않게 함
        auto self = shared_from_this();
        gr->Push([gr, self, skillId]()
            {
                float yaw = 0.f;
                if (self->GetPosInfo())
                    yaw = self->GetPosInfo()->yaw();

                // GameRoom 스레드 안에서 스킬 로직 수행
                gr->HandleSkill(self, skillId, yaw, 0);
            });
    }


    printf(" [Skill] %s used Skill %d (Cooldown: %dms)\n",
        (GetObjectType() == Protocol::OBJECT_TYPE_PLAYER ? "Player" : "Monster"),
        skillId, skillData->cooldown());
}