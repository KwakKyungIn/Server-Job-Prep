#include "pch.h"
#include "Projectile.h"

static constexpr float kPI = 3.1415926535f;

// 투사체 생성자
// Creature를 상속받았기 때문에 기본 위치 정보(_posInfo)를 초기화하고 연결해주는 작업이 필수
Projectile::Projectile() : Creature(Protocol::OBJECT_TYPE_PROJECTILE)
{
    // Protobuf 메시지 내부 포인터 연결
    // 이거 안 하면 부모 클래스에서 위치 접근할 때 터짐
    _posInfo = _projInfo.mutable_posinfo();
    _projInfo.set_objectid(GetObjectId());

    // 기본 상태 설정
    // 투사체는 생성되자마자 날아가는 게 기본이라 RUN 상태로 둠
    _posInfo->set_state(Protocol::MOVE_RUN);
    _posInfo->set_actionstate(Protocol::ACTION_IDLE);
}

Projectile::~Projectile()
{
}

// 투사체 발사 (초기화)
// 매번 `new`로 만드는 게 아니라 오브젝트 풀(Object Pool)에서 꺼내 쓸 거라서
// 생성자 대신 이 Init 함수에서 모든 상태를 리셋해줘야 함
void Projectile::Init(uint64 ownerId, int32 skillId,
    const Protocol::PositionInfo& startPos,
    float speed, uint32 lifeTimeMs, float range)
{
    _projInfo.set_ownerid(ownerId);
    _projInfo.set_skillid(skillId);
    _projInfo.set_speed(speed);
    _projInfo.set_lifetimems(lifeTimeMs);

    // 시작 위치 복사
    *_posInfo = startPos;

    // 이동 거리 및 시간 초기화
    _elapsedMs = 0;
    _traveled = 0.0f;
    _range = range;

    // 피격 목록 초기화 (이거 안 비우면 재사용할 때 이전 타겟들이 안 맞음)
    ResetHitState();

    _posInfo->set_state(Protocol::MOVE_RUN);
    _posInfo->set_actionstate(Protocol::ACTION_IDLE);
}

// 투사체 수명 체크
// 시간(LifeTime)이 다 됐거나, 사거리(Range)를 벗어나면 삭제 대상임
bool Projectile::IsExpired() const
{
    const uint32 life = _projInfo.lifetimems();
    if (life > 0 && _elapsedMs >= life)
        return true;

    if (_range > 0.0f && _traveled >= _range)
        return true;

    return false;
}

// 투사체 이동 업데이트 (서버 틱마다 호출)
// 단순 등속 운동 구현
bool Projectile::Update(uint64 deltaMs)
{
    if (deltaMs == 0)
        return false;

    // [중요] 델타 타임 보정 (Tunneling 방지)
    // 서버가 순간적으로 랙 걸려서 deltaMs가 1초(1000ms) 이렇게 들어오면
    // 투사체가 벽을 뚫고 지나가거나 맵 밖으로 튕겨 나갈 수 있음
    // 그래서 한 번에 이동할 수 있는 최대 시간을 100ms로 강제 제한함
    if (deltaMs > 100)
        deltaMs = 100;

    _elapsedMs += deltaMs;

    const float speed = _projInfo.speed();
    if (speed <= 0.0f)
        return true; // 움직임은 없지만 만료 체크는 해야 하니 true 반환

    // 밀리초 -> 초 단위 변환
    const float dt = (float)deltaMs / 1000.0f;

    // [이동 벡터 계산]
    // Unity 좌표계 기준: Y축이 회전축
    // Yaw 값이 0도일 때 Z축(앞)을 바라본다고 가정하고 삼각함수 적용
    // x = sin(theta), z = cos(theta)
    const float yawRad = _posInfo->yaw() * kPI / 180.0f;
    const float dirX = std::sinf(yawRad);
    const float dirZ = std::cosf(yawRad);

    // 변위 계산 (속력 * 시간)
    const float dx = dirX * speed * dt;
    const float dz = dirZ * speed * dt;

    // 실제 좌표 갱신
    _posInfo->set_x(_posInfo->x() + dx);
    _posInfo->set_z(_posInfo->z() + dz);

    // 총 이동 거리 누적 (사거리 체크용)
    _traveled += std::sqrt(dx * dx + dz * dz);
    return true;
}