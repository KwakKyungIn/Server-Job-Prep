#include "pch.h"
#include "GameRoom.h"
#include "Projectile.h"
#include "Player.h"
#include "GameRoom.Net.h"
#include "ClientPacketHandler.h"
#include "GameMap.h"
#include "DataManager.h"
#include "ObjectUtils.h"
#include "Monster.h"

// XZ ���: ����-�� ���� (���� �̸� t ��ȯ)
static bool SegmentCircleHitXZ(
    float x0, float z0,
    float x1, float z1,
    float cx, float cz,
    float r,
    float& outT)
{
    const float dx = x1 - x0;
    const float dz = z1 - z0;

    const float fx = x0 - cx;
    const float fz = z0 - cz;

    const float a = dx * dx + dz * dz;
    if (a < 1e-6f)
        return false;

    const float b = 2.0f * (fx * dx + fz * dz);
    const float c = (fx * fx + fz * fz) - r * r;

    float disc = b * b - 4.0f * a * c;
    if (disc < 0.0f)
        return false;

    disc = std::sqrt(disc);

    const float t1 = (-b - disc) / (2.0f * a);
    const float t2 = (-b + disc) / (2.0f * a);

    float t = 1e9f;
    if (t1 >= 0.0f && t1 <= 1.0f) t = t1;
    else if (t2 >= 0.0f && t2 <= 1.0f) t = t2;

    if (t > 1.0f) return false;

    outT = t;
    return true;
}


void GameRoom::EnterProjectile(ProjectileRef p)
{
    if (!p) return;
    const uint64 pid = p->GetObjectId();
    if (_projectiles.find(pid) != _projectiles.end())
        return;

    _projectiles.insert({ pid, p });
    p->SetRoom(shared_from_this());

    int32 zoneIndex = _grid.GetZoneIndex(*p->GetPosInfo());
    p->SetZoneIndex(zoneIndex);
    _grid.GetZone(zoneIndex).projectiles.insert(p);

    // viewers ���(���� EnterMonster�� ����)
    Vector<Zone*> zones;
    _grid.GetNearbyZones(zoneIndex, EffectiveAoiRadiusCells(), zones);

    auto& viewers = p->Viewers_ActorOnly();
    viewers.clear();

    const auto& pp = *p->GetPosInfo();
    const uint32 pConn = GetConnectivityId_ActorOnly(pp);

    Protocol::S_SPAWN spawnPkt;
    auto* info = spawnPkt.add_projectiles();
    *info = *p->GetProjectileInfo();
    SendBufferRef sb = ClientPacketHandler::MakeSendBuffer(spawnPkt);

    for (Zone* z : zones)
    {
        for (const PlayerRef& pl : z->players)
        {
            if (!pl) continue;

            const auto& plPos = *pl->GetPosInfo();
            if (!PassDistance2D(pp, plPos, _interestRadius))
                continue;

            const uint32 plConn = GetConnectivityId_ActorOnly(plPos);
            if (plConn != pConn)
                continue;

            // ���� set ���ռ�
            if (pl->VisibleProjectiles_ActorOnly().insert(pid).second)
            {
                viewers.insert(pl->GetPlayerId());
                SendToPlayer(pl->GetPlayerId(), sb);
            }
            else
            {
                viewers.insert(pl->GetPlayerId());
            }
        }
    }
}

void GameRoom::LeaveProjectile(uint64 projectileId)
{
    auto it = _projectiles.find(projectileId);
    if (it == _projectiles.end()) return;

    ProjectileRef p = it->second;
    if (!p) return;

    const int32 zoneIndex = p->GetZoneIndex();

    // viewers���� despawn + player visible set ����
    {
        Protocol::S_DESPAWN pkt;
        pkt.add_objectids(projectileId);
        SendBufferRef sb = ClientPacketHandler::MakeSendBuffer(pkt);

        auto& viewers = p->Viewers_ActorOnly();
        for (uint64 vid : viewers)
        {
            PlayerRef pl = FindPlayer_ActorOnly(vid);
            if (pl)
                pl->VisibleProjectiles_ActorOnly().erase(projectileId);

            SendToPlayer(vid, sb);
        }
        viewers.clear();
    }

    // grid remove
    int32 totalZones = _grid.GetGridSizeX() * _grid.GetGridSizeY();
    if (zoneIndex >= 0 && zoneIndex < totalZones)
        _grid.GetZone(zoneIndex).projectiles.erase(p);

    _projectiles.erase(projectileId);
    p->SetRoom(nullptr);
}

void GameRoom::OnProjectileMoved(ProjectileRef p)
{
    if (!p) return;

    const uint64 pid = p->GetObjectId();

    const int32 oldZone = p->GetZoneIndex();
    const int32 newZone = _grid.GetZoneIndex(*p->GetPosInfo());

    if (newZone != oldZone)
    {
        int32 totalZones = _grid.GetGridSizeX() * _grid.GetGridSizeY();
        if (oldZone >= 0 && oldZone < totalZones)
            _grid.GetZone(oldZone).projectiles.erase(p);

        _grid.GetZone(newZone).projectiles.insert(p);
        p->SetZoneIndex(newZone);
    }

    // ---- �� viewers ��� ----
    std::unordered_set<uint64> newViewers;

    Vector<Zone*> zones;
    _grid.GetNearbyZones(p->GetZoneIndex(), EffectiveAoiRadiusCells(), zones);

    const auto& prPos = *p->GetPosInfo();
    const uint32 prConn = GetConnectivityId_ActorOnly(prPos);

    for (Zone* z : zones)
    {
        for (const PlayerRef& pl : z->players)
        {
            if (!pl) continue;

            const auto& plPos = *pl->GetPosInfo();
            if (!PassDistance2D(prPos, plPos, _interestRadius))
                continue;

            const uint32 plConn = GetConnectivityId_ActorOnly(plPos);
            if (plConn != prConn)
                continue;

            newViewers.insert(pl->GetPlayerId());
        }
    }

    auto& oldViewers = p->Viewers_ActorOnly();

    // ---- despawn: old - new ----
    {
        Protocol::S_DESPAWN pkt;
        pkt.add_objectids(pid);
        SendBufferRef sb = ClientPacketHandler::MakeSendBuffer(pkt);

        for (uint64 vid : oldViewers)
        {
            if (newViewers.find(vid) != newViewers.end())
                continue;

            PlayerRef pl = FindPlayer_ActorOnly(vid);
            if (pl) pl->VisibleProjectiles_ActorOnly().erase(pid);

            SendToPlayer(vid, sb);
        }
    }

    // ---- spawn: new - old ----
    {
        Protocol::S_SPAWN pkt;
        auto* info = pkt.add_projectiles();
        *info = *p->GetProjectileInfo();
        SendBufferRef sb = ClientPacketHandler::MakeSendBuffer(pkt);

        for (uint64 vid : newViewers)
        {
            if (oldViewers.find(vid) != oldViewers.end())
                continue;

            PlayerRef pl = FindPlayer_ActorOnly(vid);
            if (pl) pl->VisibleProjectiles_ActorOnly().insert(pid);

            SendToPlayer(vid, sb);
        }
    }

    // ---- move: newViewers ��ü ----
    {
        Protocol::S_MOVE movePkt;
        movePkt.set_objectid(pid);
        *movePkt.mutable_posinfo() = *p->GetPosInfo();
        SendBufferRef sb = ClientPacketHandler::MakeSendBuffer(movePkt);

        for (uint64 vid : newViewers)
            SendToPlayer(vid, sb);
    }

    oldViewers = std::move(newViewers);
}

void GameRoom::UpdateProjectiles(uint64 deltaMs)
{
    if (_projectiles.empty())
        return;

    Vector<uint64> toRemove;
    toRemove.reserve(64);

    for (auto& kv : _projectiles)
    {
        ProjectileRef p = kv.second;
        if (!p) continue;

        // ��ų ������ (damage / stopOnHit / hitRadius / maxHits)
        const Protocol::SkillTemplateInfo* skillData =
            DataManager::Instance()->GetSkillTemplate(p->GetSkillId());
        if (!skillData)
        {
            toRemove.push_back(p->GetObjectId());
            continue;
        }

        // owner ã�� (playerId or monster objectId)
        std::shared_ptr<Creature> owner = nullptr;
        const uint64 ownerId = p->GetOwnerId();

        if (auto pl = FindPlayer_ActorOnly(ownerId))
            owner = std::static_pointer_cast<Creature>(pl);
        else
        {
            auto itM = _monsters.find(ownerId);
            if (itM != _monsters.end() && itM->second)
                owner = std::static_pointer_cast<Creature>(itM->second);
        }

        // owner�� ������ �ǹ� ���� ����ü �� ����
        if (!owner)
        {
            toRemove.push_back(p->GetObjectId());
            continue;
        }

        // old -> new ���� Ȯ��
        Protocol::PositionInfo oldPos = *p->GetPosInfo();

        p->Update(deltaMs);
        Protocol::PositionInfo newPos = *p->GetPosInfo();

        bool shouldDespawn = false;

        // [NavMesh Wall] Clamp segment to navmesh boundary and despawn on wall hit.
        // No Rigidbody needed on walls; server-side navmesh raycast is authoritative.
        auto map = GetMap();          // GameMapRef
        if (map)
        {
            float tWall = 1.0f;
            if (map->RaycastNav(oldPos, newPos, tWall) && tWall < 1.0f)
            {
                const float ix = oldPos.x() + (newPos.x() - oldPos.x()) * tWall;
                const float iy = oldPos.y() + (newPos.y() - oldPos.y()) * tWall;
                const float iz = oldPos.z() + (newPos.z() - oldPos.z()) * tWall;

                p->GetPosInfo()->set_x(ix);
                p->GetPosInfo()->set_y(iy);
                p->GetPosInfo()->set_z(iz);
                newPos = *p->GetPosInfo();

                shouldDespawn = true;
            }
        }


        // ===== �浹/���� (PROJECTILE ����) =====
        const int32 damage = skillData->damage();
        float hitRadius = p->HitRadius();
        if (hitRadius <= 0.0f)
        {
            hitRadius = skillData->hitradius();
            if (hitRadius <= 0.0f) hitRadius = skillData->radius();
        }

        const bool stopOnHit = p->StopOnHit();
        const int32 maxHits = p->MaxHits();

        // �ĺ� �� ����: oldZone + newZone �ֺ� union
        const int32 oldZone = _grid.GetZoneIndex(oldPos);
        const int32 newZone = _grid.GetZoneIndex(newPos);

        Vector<Zone*> zonesA;
        Vector<Zone*> zonesB;
        _grid.GetNearbyZones(oldZone, EffectiveAoiRadiusCells(), zonesA);
        if (newZone != oldZone)
            _grid.GetNearbyZones(newZone, EffectiveAoiRadiusCells(), zonesB);

        std::unordered_set<Zone*> zoneSet;
        zoneSet.reserve(zonesA.size() + zonesB.size());
        for (Zone* z : zonesA) if (z) zoneSet.insert(z);
        for (Zone* z : zonesB) if (z) zoneSet.insert(z);

        const float x0 = oldPos.x();
        const float z0 = oldPos.z();
        const float x1 = newPos.x();
        const float z1 = newPos.z();

        // impact ����
        float lastImpactT = -1.0f;

        struct HitCand
        {
            std::shared_ptr<Creature> victim;
            uint64 victimNetId = 0;
            float t = 0.0f;
        };

        std::vector<HitCand> hits;
        hits.reserve(8);

        const bool ownerIsMonster = (owner->GetObjectType() == Protocol::OBJECT_TYPE_MONSTER);

        // connectivity (����ü�� ��impact ��ġ�� �������ε� üũ�� �ǵ�,
        // 1���� newPos ���� coarse filter�� ���)
        const uint32 prConn = GetConnectivityId_ActorOnly(newPos);

        for (Zone* z : zoneSet)
        {
            if (!z) continue;

            if (ownerIsMonster)
            {
                // target = players
                for (const PlayerRef& pl : z->players)
                {
                    if (!pl) continue;
                    if (pl->GetStatInfo() && pl->GetStatInfo()->hp() <= 0) continue;

                    const uint64 vNetId = pl->GetPlayerId();
                    if (p->HasAlreadyHit(vNetId)) continue;

                    const auto& vp = *pl->GetPosInfo();
                    if (GetConnectivityId_ActorOnly(vp) != prConn) continue;

                    float t = 0.0f;
                    if (!SegmentCircleHitXZ(x0, z0, x1, z1, vp.x(), vp.z(), hitRadius, t))
                        continue;

                    hits.push_back({ std::static_pointer_cast<Creature>(pl), vNetId, t });
                }
            }
            else
            {
                // target = monsters
                for (const MonsterRef& m : z->monsters)
                {
                    if (!m) continue;
                    if (m->GetStatInfo() && m->GetStatInfo()->hp() <= 0) continue;

                    const uint64 vNetId = m->GetObjectId();
                    if (p->HasAlreadyHit(vNetId)) continue;

                    const auto& vp = *m->GetPosInfo();
                    if (GetConnectivityId_ActorOnly(vp) != prConn) continue;

                    float t = 0.0f;
                    if (!SegmentCircleHitXZ(x0, z0, x1, z1, vp.x(), vp.z(), hitRadius, t))
                        continue;

                    hits.push_back({ std::static_pointer_cast<Creature>(m), vNetId, t });
                }
            }
        }

        if (!hits.empty() && p->CanHitMore())
        {
            std::sort(hits.begin(), hits.end(),
                [](const HitCand& a, const HitCand& b) { return a.t < b.t; });

            for (const HitCand& h : hits)
            {
                if (!h.victim) continue;
                if (!p->CanHitMore()) break;

                // ���� ������ ����
                h.victim->OnDamaged(owner, damage);

                // impact pos�� ����ü ��ġ ����(���־� ������ ����)
                const float ix = x0 + (x1 - x0) * h.t;
                const float iz = z0 + (z1 - z0) * h.t;
                p->GetPosInfo()->set_x(ix);
                p->GetPosInfo()->set_z(iz);
                lastImpactT = h.t;

                // HP ��ε�ĳ��Ʈ (impact zone ����)
                Protocol::S_CHANGE_HP changePkt;
                changePkt.set_objectid(NetId(h.victim));
                changePkt.set_attackerid(NetId(owner));
                changePkt.set_currenthp(h.victim->GetStatInfo()->hp());
                changePkt.set_damage(damage);

                SendBufferRef sb = ClientPacketHandler::MakeSendBuffer(changePkt);

                const int32 impactZone = _grid.GetZoneIndex(*p->GetPosInfo());
                BroadcastToZone(sb, impactZone);

                // ��Ʈ ���
                p->MarkHit(h.victimNetId);

                // stopOnHit or maxHits ���� �� despawn
                if (stopOnHit || p->HitCount() >= maxHits)
                {
                    shouldDespawn = true;
                    break;
                }
            }
        }

        // ===== ��Ʈ��ũ ����(���� ��ġ ����) =====
        OnProjectileMoved(p);

        // ����/��Ʈ�� ����
        if (shouldDespawn || p->IsExpired())
            toRemove.push_back(p->GetObjectId());
    }

    for (uint64 pid : toRemove)
        LeaveProjectile(pid);
}
