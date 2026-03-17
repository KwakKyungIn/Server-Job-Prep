#include "pch.h"
#include "GameRoom.h"
#include "Projectile.h"
#include "Player.h"
#include "GameRoom.Net.h"
#include "ClientPacketHandler.h"
#include "GameMap.h"
#include "DataManager.h"
#include "ExperimentUtils.h"
#include "ObjectUtils.h"
#include "Monster.h"
#include "GameMetrics.h"

// 2D 충돌 판정용 수학 함수 (선분 vs 원)
// 투사체 경로(선분)와 타겟의 히트박스(원)가 겹치는지 검사함
// 2차 방정식 근의 공식을 사용해서 교차 시점 t (0~1)를 구해낸다
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
    // 이동 거리가 거의 없으면 판정 안 함
    if (a < 1e-6f)
        return false;

    const float b = 2.0f * (fx * dx + fz * dz);
    const float c = (fx * fx + fz * fz) - r * r;

    float disc = b * b - 4.0f * a * c;
    // 판별식이 음수면 교차하지 않음 (충돌 X)
    if (disc < 0.0f)
        return false;

    disc = std::sqrt(disc);

    // 근 두 개(진입점, 탈출점) 구하기
    const float t1 = (-b - disc) / (2.0f * a);
    const float t2 = (-b + disc) / (2.0f * a);

    float t = 1e9f;
    if (t1 >= 0.0f && t1 <= 1.0f) t = t1;
    else if (t2 >= 0.0f && t2 <= 1.0f) t = t2;

    // 선분 범위(0~1) 밖이면 무효
    if (t > 1.0f) return false;

    outT = t;
    return true;
}


void GameRoom::EnterProjectile(ProjectileRef p)
{
    if (!p) return;
    const uint64 pid = p->GetObjectId();

    // 이미 방에 있는 투사체면 패스
    if (_projectiles.find(pid) != _projectiles.end())
        return;

    // 관리 목록에 추가하고 방 포인터 연결
    _projectiles.insert({ pid, p });
    p->SetRoom(shared_from_this());

    // Grid 시스템에 투사체 등록. 플레이어랑 똑같이 Zone 관리를 받아야 함
    int32 zoneIndex = _grid.GetZoneIndex(*p->GetPosInfo());
    p->SetZoneIndex(zoneIndex);
    _grid.GetZone(zoneIndex).projectiles.insert(p);

    if (ExperimentUtils::IsHotRoomRoomWideBaseline())
    {
        auto& viewers = p->Viewers_ActorOnly();
        viewers.clear();

        for (auto& item : _players)
        {
            PlayerRef pl = item.second;
            if (!pl)
                continue;

            pl->VisibleProjectiles_ActorOnly().insert(pid);
            viewers.insert(pl->GetPlayerId());
        }

        Protocol::S_SPAWN spawnPkt;
        auto* info = spawnPkt.add_projectiles();
        *info = *p->GetProjectileInfo();
        SendBufferRef sb = ClientPacketHandler::MakeSendBuffer(spawnPkt);

        const int32 recipients = Broadcast(sb);
        GameMetrics::OnBroadcastRecipients(
            GameMetrics::HotRoomBroadcastKind::Spawn,
            GameMetrics::HotRoomBroadcastMode::Room,
            static_cast<std::size_t>(recipients));
        return;
    }

    // AOI 처리: 주변 플레이어들에게 투사체 스폰 패킷 전송
    // 투사체는 빠르니까 시야 처리가 중요함
    Vector<Zone*> zones;
    _grid.GetNearbyZones(zoneIndex, EffectiveAoiRadiusCells(), zones);

    auto& viewers = p->Viewers_ActorOnly();
    viewers.clear();
    std::size_t spawnRecipients = 0;

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

            // 거리 및 벽(Connectivity) 체크
            const auto& plPos = *pl->GetPosInfo();
            if (!PassDistance2D(pp, plPos, _interestRadius))
                continue;

            const uint32 plConn = GetConnectivityId_ActorOnly(plPos);
            if (plConn != pConn)
                continue;

            // 플레이어 시야 목록 갱신 및 패킷 전송
            if (pl->VisibleProjectiles_ActorOnly().insert(pid).second)
            {
                viewers.insert(pl->GetPlayerId());
                SendToPlayer(pl->GetPlayerId(), sb);
                ++spawnRecipients;
            }
            else
            {
                viewers.insert(pl->GetPlayerId());
            }
        }
    }

    GameMetrics::OnBroadcastRecipients(
        GameMetrics::HotRoomBroadcastKind::Spawn,
        GameMetrics::HotRoomBroadcastMode::Aoi,
        spawnRecipients);
}

void GameRoom::LeaveProjectile(uint64 projectileId)
{
    auto it = _projectiles.find(projectileId);
    if (it == _projectiles.end()) return;

    ProjectileRef p = it->second;
    if (!p) return;

    const int32 zoneIndex = p->GetZoneIndex();

    // 소멸 처리: 이걸 보고 있던 플레이어들에게 Despawn 패킷을 날린다
    {
        Protocol::S_DESPAWN pkt;
        pkt.add_objectids(projectileId);
        SendBufferRef sb = ClientPacketHandler::MakeSendBuffer(pkt);

        if (ExperimentUtils::IsHotRoomRoomWideBaseline())
        {
            auto& viewers = p->Viewers_ActorOnly();
            for (uint64 vid : viewers)
            {
                PlayerRef pl = FindPlayer_ActorOnly(vid);
                if (pl)
                    pl->VisibleProjectiles_ActorOnly().erase(projectileId);
            }

            const int32 recipients = Broadcast(sb);
            GameMetrics::OnBroadcastRecipients(
                GameMetrics::HotRoomBroadcastKind::Despawn,
                GameMetrics::HotRoomBroadcastMode::Room,
                static_cast<std::size_t>(recipients));
            viewers.clear();
        }
        else
        {
            auto& viewers = p->Viewers_ActorOnly();
            std::size_t despawnRecipients = 0;
            for (uint64 vid : viewers)
            {
                PlayerRef pl = FindPlayer_ActorOnly(vid);
                if (pl)
                    pl->VisibleProjectiles_ActorOnly().erase(projectileId);

                SendToPlayer(vid, sb);
                ++despawnRecipients;
            }
            viewers.clear();
            GameMetrics::OnBroadcastRecipients(
                GameMetrics::HotRoomBroadcastKind::Despawn,
                GameMetrics::HotRoomBroadcastMode::Aoi,
                despawnRecipients);
        }
    }

    // Grid 시스템에서 완전히 제거
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

    // Zone이 바뀌었으면 Grid 업데이트 수행
    if (newZone != oldZone)
    {
        int32 totalZones = _grid.GetGridSizeX() * _grid.GetGridSizeY();
        if (oldZone >= 0 && oldZone < totalZones)
            _grid.GetZone(oldZone).projectiles.erase(p);

        _grid.GetZone(newZone).projectiles.insert(p);
        p->SetZoneIndex(newZone);
    }

    if (ExperimentUtils::IsHotRoomRoomWideBaseline())
    {
        Protocol::S_MOVE movePkt;
        movePkt.set_objectid(pid);
        *movePkt.mutable_posinfo() = *p->GetPosInfo();
        SendBufferRef sb = ClientPacketHandler::MakeSendBuffer(movePkt);

        const int32 recipients = Broadcast(sb);
        GameMetrics::OnBroadcastRecipients(
            GameMetrics::HotRoomBroadcastKind::Move,
            GameMetrics::HotRoomBroadcastMode::Room,
            static_cast<std::size_t>(recipients));
        return;
    }

    // AOI 업데이트 로직 (GameRoom.Monster.cpp와 유사)
    // 투사체는 계속 움직이니까 매번 시야 목록을 갱신해줘야 함

    // 1. 현재 위치 기준으로 새로운 시청자 목록 계산
    HashSet<uint64> newViewers;

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

    // 2. Despawn 처리: 예전엔 봤는데 이제 못 보는 사람들 (Old - New)
    {
        Protocol::S_DESPAWN pkt;
        pkt.add_objectids(pid);
        SendBufferRef sb = ClientPacketHandler::MakeSendBuffer(pkt);
        std::size_t despawnRecipients = 0;

        for (uint64 vid : oldViewers)
        {
            if (newViewers.find(vid) != newViewers.end())
                continue;

            PlayerRef pl = FindPlayer_ActorOnly(vid);
            if (pl) pl->VisibleProjectiles_ActorOnly().erase(pid);

            SendToPlayer(vid, sb);
            ++despawnRecipients;
        }

        GameMetrics::OnBroadcastRecipients(
            GameMetrics::HotRoomBroadcastKind::Despawn,
            GameMetrics::HotRoomBroadcastMode::Aoi,
            despawnRecipients);
    }

    // 3. Spawn 처리: 새로 보게 된 사람들 (New - Old)
    {
        Protocol::S_SPAWN pkt;
        auto* info = pkt.add_projectiles();
        *info = *p->GetProjectileInfo();
        SendBufferRef sb = ClientPacketHandler::MakeSendBuffer(pkt);
        std::size_t spawnRecipients = 0;

        for (uint64 vid : newViewers)
        {
            if (oldViewers.find(vid) != oldViewers.end())
                continue;

            PlayerRef pl = FindPlayer_ActorOnly(vid);
            if (pl) pl->VisibleProjectiles_ActorOnly().insert(pid);

            SendToPlayer(vid, sb);
            ++spawnRecipients;
        }

        GameMetrics::OnBroadcastRecipients(
            GameMetrics::HotRoomBroadcastKind::Spawn,
            GameMetrics::HotRoomBroadcastMode::Aoi,
            spawnRecipients);
    }

    // 4. Move 패킷 전송: 계속 보고 있는 사람들 (Intersection) + New Viewers
    {
        Protocol::S_MOVE movePkt;
        movePkt.set_objectid(pid);
        *movePkt.mutable_posinfo() = *p->GetPosInfo();
        SendBufferRef sb = ClientPacketHandler::MakeSendBuffer(movePkt);
        std::size_t moveRecipients = 0;

        for (uint64 vid : newViewers)
        {
            SendToPlayer(vid, sb);
            ++moveRecipients;
        }

        GameMetrics::OnBroadcastRecipients(
            GameMetrics::HotRoomBroadcastKind::Move,
            GameMetrics::HotRoomBroadcastMode::Aoi,
            moveRecipients);
    }

    // 시청자 목록 교체
    oldViewers = std::move(newViewers);
}

void GameRoom::UpdateProjectiles(uint64 deltaMs)
{
    if (_projectiles.empty())
        return;

    // 루프 돌다가 삭제될 투사체들 모아두는 곳
    Vector<uint64> toRemove;
    toRemove.reserve(64);

    for (auto& kv : _projectiles)
    {
        ProjectileRef p = kv.second;
        if (!p) continue;

        // 스킬 데이터 확인 (데미지, 관통 여부 등)
        const Protocol::SkillTemplateInfo* skillData =
            DataManager::Instance()->GetSkillTemplate(p->GetSkillId());
        if (!skillData)
        {
            toRemove.push_back(p->GetObjectId());
            continue;
        }

        // 투사체 주인(Attacker) 찾기. 주인이 나가거나 없으면 투사체도 소멸
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

        if (!owner)
        {
            toRemove.push_back(p->GetObjectId());
            continue;
        }

        // 이동 전 위치 저장 (충돌 판정용 선분 시점)
        Protocol::PositionInfo oldPos = *p->GetPosInfo();

        // 투사체 위치 업데이트 (여기서 좌표가 바뀜)
        p->Update(deltaMs);
        Protocol::PositionInfo newPos = *p->GetPosInfo();

        bool shouldDespawn = false;

        // [NavMesh Wall Check] 서버 권위의 핵심
        // 투사체가 벽을 뚫고 지나가는지 NavMesh Raycast로 검증함
        // 벽에 맞았으면 충돌 지점으로 좌표 보정하고 소멸 처리
        auto map = GetMap();
        if (map)
        {
            float tWall = 1.0f;
            if (map->RaycastNav(oldPos, newPos, tWall) && tWall < 1.0f)
            {
                // 보간해서 정확한 벽 충돌 지점 계산
                const float ix = oldPos.x() + (newPos.x() - oldPos.x()) * tWall;
                const float iy = oldPos.y() + (newPos.y() - oldPos.y()) * tWall;
                const float iz = oldPos.z() + (newPos.z() - oldPos.z()) * tWall;

                p->GetPosInfo()->set_x(ix);
                p->GetPosInfo()->set_y(iy);
                p->GetPosInfo()->set_z(iz);
                newPos = *p->GetPosInfo();

                shouldDespawn = true; // 벽 맞으면 사라짐
            }
        }


        // ===== 엔티티 충돌 판정 로직 =====
        const int32 damage = skillData->damage();
        float hitRadius = p->HitRadius();

        // 데이터 시트에서 반경 정보 가져오기
        if (hitRadius <= 0.0f)
        {
            hitRadius = skillData->hitradius();
            if (hitRadius <= 0.0f) hitRadius = skillData->radius();
        }

        const bool stopOnHit = p->StopOnHit(); // 맞으면 멈추는가? (관통 불가)
        const int32 maxHits = p->MaxHits();    // 최대 타격 개체 수

        // 최적화를 위해 Old Zone과 New Zone 주변만 검사함
        const int32 oldZone = _grid.GetZoneIndex(oldPos);
        const int32 newZone = _grid.GetZoneIndex(newPos);

        Vector<Zone*> zonesA;
        Vector<Zone*> zonesB;
        _grid.GetNearbyZones(oldZone, EffectiveAoiRadiusCells(), zonesA);
        if (newZone != oldZone)
            _grid.GetNearbyZones(newZone, EffectiveAoiRadiusCells(), zonesB);

        // 중복 검사 방지를 위해 Set 사용
        HashSet<Zone*> zoneSet;
        zoneSet.reserve(zonesA.size() + zonesB.size());
        for (Zone* z : zonesA) if (z) zoneSet.insert(z);
        for (Zone* z : zonesB) if (z) zoneSet.insert(z);

        const float x0 = oldPos.x();
        const float z0 = oldPos.z();
        const float x1 = newPos.x();
        const float z1 = newPos.z();

        float lastImpactT = -1.0f;

        // 충돌 후보군 구조체
        struct HitCand
        {
            std::shared_ptr<Creature> victim;
            uint64 victimNetId = 0;
            float t = 0.0f; // 충돌 시점 (0~1)
        };

        Vector<HitCand> hits;
        hits.reserve(8);

        const bool ownerIsMonster = (owner->GetObjectType() == Protocol::OBJECT_TYPE_MONSTER);
        const uint32 prConn = GetConnectivityId_ActorOnly(newPos);

        // 주변 Zone들을 순회하며 충돌 검사 수행
        for (Zone* z : zoneSet)
        {
            if (!z) continue;

            if (ownerIsMonster)
            {
                // 몬스터가 쏘면 플레이어만 맞음
                for (const PlayerRef& pl : z->players)
                {
                    if (!pl) continue;
                    if (pl->GetStatInfo() && pl->GetStatInfo()->hp() <= 0) continue; // 시체는 무시

                    const uint64 vNetId = pl->GetPlayerId();
                    if (p->HasAlreadyHit(vNetId)) continue; // 이미 맞은 놈은 패스

                    // 벽 너머에 있는 애들은 안 맞게 처리
                    const auto& vp = *pl->GetPosInfo();
                    if (GetConnectivityId_ActorOnly(vp) != prConn) continue;

                    float t = 0.0f;
                    // 여기서 아까 만든 수학 함수 호출
                    if (!SegmentCircleHitXZ(x0, z0, x1, z1, vp.x(), vp.z(), hitRadius, t))
                        continue;

                    hits.push_back({ std::static_pointer_cast<Creature>(pl), vNetId, t });
                }
            }
            else
            {
                // 플레이어가 쏘면 몬스터만 맞음
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

        // 충돌한 대상이 있으면 처리
        if (!hits.empty() && p->CanHitMore())
        {
            // 가까운 순서대로 정렬 (관통 스킬일 경우 앞의 적부터 맞아야 하니까)
            std::sort(hits.begin(), hits.end(),
                [](const HitCand& a, const HitCand& b) { return a.t < b.t; });

            for (const HitCand& h : hits)
            {
                if (!h.victim) continue;
                if (!p->CanHitMore()) break;

                // 데미지 적용
                h.victim->OnDamaged(owner, damage);

                // 투사체 위치를 충돌 지점으로 강제 이동 (시각적 정확도)
                const float ix = x0 + (x1 - x0) * h.t;
                const float iz = z0 + (z1 - z0) * h.t;
                p->GetPosInfo()->set_x(ix);
                p->GetPosInfo()->set_z(iz);
                lastImpactT = h.t;

                // HP 감소 패킷 브로드캐스팅
                Protocol::S_CHANGE_HP changePkt;
                changePkt.set_objectid(NetId(h.victim));
                changePkt.set_attackerid(NetId(owner));
                changePkt.set_currenthp(h.victim->GetStatInfo()->hp());
                changePkt.set_damage(damage);

                SendBufferRef sb = ClientPacketHandler::MakeSendBuffer(changePkt);

                const int32 impactZone = _grid.GetZoneIndex(*p->GetPosInfo());
                const bool roomWideBaseline = ExperimentUtils::IsHotRoomRoomWideBaseline();
                const int32 recipients = roomWideBaseline
                    ? Broadcast(sb)
                    : BroadcastToZone(sb, impactZone);
                GameMetrics::OnBroadcastRecipients(
                    GameMetrics::HotRoomBroadcastKind::Hp,
                    roomWideBaseline ? GameMetrics::HotRoomBroadcastMode::Room : GameMetrics::HotRoomBroadcastMode::Aoi,
                    static_cast<std::size_t>(recipients));

                // 피격 기록 (다단히트 방지)
                p->MarkHit(h.victimNetId);

                // 관통 불가거나 최대 타겟 수 채웠으면 소멸
                if (stopOnHit || p->HitCount() >= maxHits)
                {
                    shouldDespawn = true;
                    break;
                }
            }
        }

        // 네트워크 동기화 (이동 패킷 전송)
        OnProjectileMoved(p);

        // 수명 다했거나 벽/적에 맞았으면 삭제 목록에 추가
        if (shouldDespawn || p->IsExpired())
            toRemove.push_back(p->GetObjectId());
    }

    // 일괄 삭제 처리
    for (uint64 pid : toRemove)
        LeaveProjectile(pid);
}
