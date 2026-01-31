#pragma once
class Creature;
class SpatialGrid;
struct Zone;

// 전투 결과 데이터를 담는 구조체들

// 피격된 대상과 입은 데미지 정보
struct HitInfo
{
    std::shared_ptr<Creature> target;
    int32                     damage = 0;
};

// 스킬 하나 썼을 때 발생하는 전체 결과
// 이걸 패킷으로 만들어서 클라한테 보내주면 됨
struct SkillResult
{
    int32        skillId = 0;
    int32        zoneIndex = -1;  // 브로드캐스팅할 때 중심이 될 존 인덱스
    Vector<HitInfo> hits;         // 스킬에 맞은 대상 목록
};
// BattleSystem : 전투 공식과 판정을 담당하는 클래스
// 상태를 저장하지 않고 계산만 수행하는 엔진 역할이라 순수 로직에 가깝다

class BattleSystem
{
public:
    // 생성자에서 그리드 포인터를 받아둔다 (AOI 체크용)
    explicit BattleSystem(SpatialGrid* grid);
    ~BattleSystem() = default;

    // 스킬 사용에 대한 최종 판정을 내리는 메인 함수
    // attacker가 skillId를 썼을 때 결과가 outResult에 담김
    bool ResolveSkill(const std::shared_ptr<Creature>& attacker,
        int32 skillId,
        float castYaw,
        SkillResult& outResult);

private:
    // 주변에 맞을만한 후보군들을 1차적으로 추려내는 헬퍼 함수
    bool CollectCandidates(const std::shared_ptr<Creature>& attacker,
        bool isMonster,
        int32 zoneIndex,
        Vector<std::shared_ptr<Creature>>& outCandidates);

private:
    SpatialGrid* _grid = nullptr; // 그리드 객체는 외부에서 관리하니까 포인터만 들고 있음
};
