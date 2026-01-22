#include "pch.h"
#include "Projectile.h"

static constexpr float kPI = 3.1415926535f;

Projectile::Projectile() : Creature(Protocol::OBJECT_TYPE_PROJECTILE)
{
    _posInfo = _projInfo.mutable_posinfo();
    _projInfo.set_objectid(GetObjectId());

    _posInfo->set_state(Protocol::MOVE_RUN);
    _posInfo->set_actionstate(Protocol::ACTION_IDLE);
}

Projectile::~Projectile()
{
}

void Projectile::Init(uint64 ownerId, int32 skillId,
    const Protocol::PositionInfo& startPos,
    float speed, uint32 lifeTimeMs, float range)
{
    _projInfo.set_ownerid(ownerId);
    _projInfo.set_skillid(skillId);
    _projInfo.set_speed(speed);
    _projInfo.set_lifetimems(lifeTimeMs);

    *_posInfo = startPos;

    _elapsedMs = 0;
    _traveled = 0.0f;
    _range = range;

    ResetHitState();

    _posInfo->set_state(Protocol::MOVE_RUN);
    _posInfo->set_actionstate(Protocol::ACTION_IDLE);
}

bool Projectile::IsExpired() const
{
    const uint32 life = _projInfo.lifetimems();
    if (life > 0 && _elapsedMs >= life)
        return true;

    if (_range > 0.0f && _traveled >= _range)
        return true;

    return false;
}

bool Projectile::Update(uint64 deltaMs)
{
    if (deltaMs == 0)
        return false;

    // 과도한 점프 방지
    if (deltaMs > 100)
        deltaMs = 100;

    _elapsedMs += deltaMs;

    const float speed = _projInfo.speed();
    if (speed <= 0.0f)
        return true; // 움직임 없지만(이상) 만료 체크는 하게 true 처리해도 됨

    const float dt = (float)deltaMs / 1000.0f;

    // Unity yaw 기준: x = sin(yaw), z = cos(yaw)
    const float yawRad = _posInfo->yaw() * kPI / 180.0f;
    const float dirX = std::sinf(yawRad);
    const float dirZ = std::cosf(yawRad);

    const float dx = dirX * speed * dt;
    const float dz = dirZ * speed * dt;

    _posInfo->set_x(_posInfo->x() + dx);
    _posInfo->set_z(_posInfo->z() + dz);

    _traveled += std::sqrt(dx * dx + dz * dz);
    return true;
}
