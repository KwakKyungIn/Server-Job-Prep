#include "pch.h"
#include "GameRoom.h"
#include "Player.h"
#include "Monster.h"
#include "GameRoom.Net.h"
#include <algorithm>
#include "ClientPacketHandler.h"
#include "PlayerSession.h"
#include "GameMap.h"
#include "Projectile.h"

bool GameRoom::PassDistance2D(const Protocol::PositionInfo& a, const Protocol::PositionInfo& b, float r) const
{
    const float dx = a.x() - b.x();
    const float dz = a.z() - b.z();
    const float rr = r * r;
    return (dx * dx + dz * dz) <= rr;
}

void GameRoom::CollectCandidates(int32 zoneIndex, Vector<PlayerRef>& outPlayers, Vector<MonsterRef>& outMonsters)
{
    outPlayers.clear();
    outMonsters.clear();

    Vector<Zone*> zones;
    _grid.GetNearbyZones(zoneIndex, EffectiveAoiRadiusCells(), zones);

    for (Zone* z : zones)
    {
        for (const PlayerRef& p : z->players)
            if (p) outPlayers.push_back(p);

        for (const MonsterRef& m : z->monsters)
            if (m) outMonsters.push_back(m);
    }
}

bool GameRoom::ShouldUpdateAOI(PlayerRef me, bool zoneChanged) const
{
    if (!me) return false;
    if (zoneChanged) return true;

    const uint64 now = ::GetTickCount64();
    const uint64 last = me->LastAoiTickMs_ActorOnly();

    // tick 기반
    if (now - last >= _lazyUpdateTickMs)
        return true;

    // 거리 기반
    const auto& cur = *me->GetPosInfo();
    const auto& lastPos = me->LastAoiPos_ActorOnly();

    const float dx = cur.x() - lastPos.x();
    const float dz = cur.z() - lastPos.z();

    return (dx * dx + dz * dz) >= (_lazyUpdateDist * _lazyUpdateDist);
}

void GameRoom::SendSpawnBatchedToMe(PlayerSessionRef session,
    const Vector<PlayerRef>& spawnPlayers,
    const Vector<MonsterRef>& spawnMonsters,
    bool snapshotMode,
    uint32 snapshotId)
{
    if (!session) return;

    //  스냅샷 모드면 "패킷 리스트"를 먼저 만들고,
    //    첫 패킷 begin=true, 마지막 패킷 end=true로 마감한다.
    Vector<Protocol::S_SPAWN> pkts;
    pkts.reserve(
        (spawnPlayers.size() + _batchSpawnPlayers - 1) / _batchSpawnPlayers +
        (spawnMonsters.size() + _batchSpawnMonsters - 1) / _batchSpawnMonsters +
        1);

    // ---- players batching ----
    for (int32 i = 0; i < (int32)spawnPlayers.size(); )
    {
        Protocol::S_SPAWN pkt;

        // meta 기본값
        pkt.set_snapshot_id(snapshotMode ? snapshotId : 0);
        pkt.set_snapshot_begin(false);
        pkt.set_snapshot_end(false);

        int32 take = min(_batchSpawnPlayers, (int32)spawnPlayers.size() - i);
        for (int32 k = 0; k < take; k++)
        {
            auto* info = pkt.add_players();
            *info = *spawnPlayers[i + k]->GetPlayerInfo();
        }
        i += take;

        pkts.push_back(std::move(pkt));
    }

    // ---- monsters batching ----
    for (int32 j = 0; j < (int32)spawnMonsters.size(); )
    {
        Protocol::S_SPAWN pkt;

        // meta 기본값
        pkt.set_snapshot_id(snapshotMode ? snapshotId : 0);
        pkt.set_snapshot_begin(false);
        pkt.set_snapshot_end(false);

        int32 take = min(_batchSpawnMonsters, (int32)spawnMonsters.size() - j);
        for (int32 k = 0; k < take; k++)
        {
            auto* info = pkt.add_monsters();
            *info = *spawnMonsters[j + k]->GetMonsterInfo();
        }
        j += take;

        pkts.push_back(std::move(pkt));
    }

    //  스폰이 "0개"인 스냅샷도 begin/end로 닫아줘야 클라가 완료를 알 수 있다.
    if (snapshotMode && pkts.empty())
    {
        Protocol::S_SPAWN emptyPkt;
        emptyPkt.set_snapshot_id(snapshotId);
        emptyPkt.set_snapshot_begin(true);
        emptyPkt.set_snapshot_end(true);
        session->Send(ClientPacketHandler::MakeSendBuffer(emptyPkt));
        return;
    }

    //  스냅샷이면 첫/끝 플래그 마감
    if (snapshotMode && !pkts.empty())
    {
        pkts.front().set_snapshot_begin(true);
        pkts.back().set_snapshot_end(true);
    }

    // ---- send ----
    for (auto& pkt : pkts)
    {
        session->Send(ClientPacketHandler::MakeSendBuffer(pkt));
    }
}

void GameRoom::SendDespawnBatchedToMe(PlayerSessionRef session, const Vector<uint64>& objectIds)
{
    if (!session) return;
    int32 i = 0;

    while (i < static_cast<int32>(objectIds.size()))
    {
        Protocol::S_DESPAWN pkt;
        int32 take = min(_batchDespawn, static_cast<int32>(objectIds.size()) - i);
        for (int32 k = 0; k < take; k++)
            pkt.add_objectids(objectIds[i + k]);

        i += take;

        session->Send(ClientPacketHandler::MakeSendBuffer(pkt));
    }
}

uint32 GameRoom::GetConnectivityId_ActorOnly(const Protocol::PositionInfo& pos) const
{
    if (!_map) return 0;
    return _map->GetConnectivityId(pos.x(), pos.y(), pos.z());
}


void GameRoom::UpdateAOI(PlayerSessionRef session, PlayerRef me, bool forceFullSnapshot)
{
    if (!session || !me) return;

    const uint64 meId = me->GetPlayerId();
    const auto& myPos = *me->GetPosInfo();
    const uint32 myConn = GetConnectivityId_ActorOnly(myPos);

    // 후보 수집 (기존 유지)
    Vector<PlayerRef> candPlayers;
    Vector<MonsterRef> candMonsters;
    CollectCandidates(me->GetZoneIndex(), candPlayers, candMonsters);

    //  [ADD] projectile 후보는 zone에서 직접 긁음 (CollectCandidates 영향 최소)
    Vector<Zone*> candZones;
    _grid.GetNearbyZones(me->GetZoneIndex(), EffectiveAoiRadiusCells(), candZones);

    // 새 가시집합 계산
    HashSet<uint64> newVisPlayers;
    HashSet<uint64> newVisMonsters;
    HashSet<uint64> newVisProjectiles;

    for (const PlayerRef& other : candPlayers)
    {
        if (!other) continue;
        const uint64 oid = other->GetPlayerId();
        if (oid == meId) continue;

        const auto& op = *other->GetPosInfo();

        if (!PassDistance2D(myPos, op, _interestRadius))
            continue;

        const uint32 oConn = GetConnectivityId_ActorOnly(op);
        if (oConn != myConn)
            continue;

        newVisPlayers.insert(oid);
    }

    for (const MonsterRef& m : candMonsters)
    {
        if (!m) continue;
        const uint64 mid = m->GetObjectId();
        const auto& mp = *m->GetPosInfo();

        if (!PassDistance2D(myPos, mp, _interestRadius))
            continue;

        const uint32 mConn = GetConnectivityId_ActorOnly(mp);
        if (mConn != myConn)
            continue;

        newVisMonsters.insert(mid);
    }

    //  [ADD] projectiles
    for (Zone* z : candZones)
    {
        if (!z) continue;

        for (const ProjectileRef& pr : z->projectiles)
        {
            if (!pr) continue;

            const uint64 prid = pr->GetObjectId();
            const auto& pp = *pr->GetPosInfo();

            if (!PassDistance2D(myPos, pp, _interestRadius))
                continue;

            const uint32 prConn = GetConnectivityId_ActorOnly(pp);
            if (prConn != myConn)
                continue;

            newVisProjectiles.insert(prid);
        }
    }

    // old sets
    auto& oldP = me->VisiblePlayers_ActorOnly();
    auto& oldM = me->VisibleMonsters_ActorOnly();
    auto& oldPr = me->VisibleProjectiles_ActorOnly(); //  [ADD]

    //  forceFullSnapshot이면, 기존에 보던 몬스터/투사체 viewers에서 나 제거하고 비움
    if (forceFullSnapshot)
    {
        for (uint64 mid : oldM)
        {
            auto it = _monsters.find(mid);
            if (it != _monsters.end() && it->second)
                it->second->Viewers_ActorOnly().erase(meId);
        }

        for (uint64 prid : oldPr)
        {
            auto it = _projectiles.find(prid);
            if (it != _projectiles.end() && it->second)
                it->second->Viewers_ActorOnly().erase(meId);
        }

        oldP.clear();
        oldM.clear();
        oldPr.clear();
    }

    // diff 계산
    Vector<uint64> toDespawnP;
    Vector<uint64> toDespawnM;
    Vector<uint64> toDespawnPr;              //  [ADD]
    Vector<PlayerRef> toSpawnP;
    Vector<MonsterRef> toSpawnM;
    Vector<ProjectileRef> toSpawnPr;         //  [ADD]

    // despawn players: old - new
    for (uint64 pid : oldP)
        if (newVisPlayers.find(pid) == newVisPlayers.end())
            toDespawnP.push_back(pid);

    // spawn players: new - old
    for (uint64 pid : newVisPlayers)
        if (oldP.find(pid) == oldP.end())
        {
            PlayerRef p = FindPlayer_ActorOnly(pid);
            if (p) toSpawnP.push_back(p);
        }

    // despawn monsters: old - new
    for (uint64 mid : oldM)
        if (newVisMonsters.find(mid) == newVisMonsters.end())
            toDespawnM.push_back(mid);

    // spawn monsters: new - old
    for (uint64 mid : newVisMonsters)
        if (oldM.find(mid) == oldM.end())
        {
            auto it = _monsters.find(mid);
            if (it != _monsters.end() && it->second)
                toSpawnM.push_back(it->second);
        }

    //  [ADD] despawn projectiles: old - new
    for (uint64 prid : oldPr)
        if (newVisProjectiles.find(prid) == newVisProjectiles.end())
            toDespawnPr.push_back(prid);

    //  [ADD] spawn projectiles: new - old
    for (uint64 prid : newVisProjectiles)
        if (oldPr.find(prid) == oldPr.end())
        {
            auto it = _projectiles.find(prid);
            if (it != _projectiles.end() && it->second)
                toSpawnPr.push_back(it->second);
        }

    // 몬스터 viewers 동기화 (despawn: remove, spawn: add)
    for (uint64 mid : toDespawnM)
    {
        auto it = _monsters.find(mid);
        if (it != _monsters.end() && it->second)
            it->second->Viewers_ActorOnly().erase(meId);
    }

    for (const MonsterRef& m : toSpawnM)
    {
        if (!m) continue;
        m->Viewers_ActorOnly().insert(meId);
    }

    //  [ADD] 투사체 viewers 동기화 (despawn: remove, spawn: add)
    for (uint64 prid : toDespawnPr)
    {
        auto it = _projectiles.find(prid);
        if (it != _projectiles.end() && it->second)
            it->second->Viewers_ActorOnly().erase(meId);
    }

    for (const ProjectileRef& pr : toSpawnPr)
    {
        if (!pr) continue;
        pr->Viewers_ActorOnly().insert(meId);
    }

    // === 1) 나에게 despawn ===
    {
        Vector<uint64> ids;
        ids.reserve(toDespawnP.size() + toDespawnM.size() + toDespawnPr.size());
        for (uint64 pid : toDespawnP) ids.push_back(pid);
        for (uint64 mid : toDespawnM) ids.push_back(mid);
        for (uint64 prid : toDespawnPr) ids.push_back(prid); //  [ADD]

        if (!ids.empty())
            SendDespawnBatchedToMe(session, ids);
    }

    // === 2) 나에게 spawn(배칭) ===
    if (forceFullSnapshot)
    {
        //  스냅샷 모드: players/monsters/projectiles를 "한 스트림"으로 begin/end 마감
        const uint32 snapId = me->NextSnapshotSeq_ActorOnly();
        Vector<Protocol::S_SPAWN> pkts;

        // ---- players batching ----
        for (int32 i = 0; i < (int32)toSpawnP.size(); )
        {
            Protocol::S_SPAWN pkt;
            pkt.set_snapshot_id(snapId);
            pkt.set_snapshot_begin(false);
            pkt.set_snapshot_end(false);

            int32 take = min(_batchSpawnPlayers, (int32)toSpawnP.size() - i);
            for (int32 k = 0; k < take; k++)
            {
                auto* info = pkt.add_players();
                *info = *toSpawnP[i + k]->GetPlayerInfo();
            }
            i += take;
            pkts.push_back(std::move(pkt));
        }

        // ---- monsters batching ----
        for (int32 j = 0; j < (int32)toSpawnM.size(); )
        {
            Protocol::S_SPAWN pkt;
            pkt.set_snapshot_id(snapId);
            pkt.set_snapshot_begin(false);
            pkt.set_snapshot_end(false);

            int32 take = min(_batchSpawnMonsters, (int32)toSpawnM.size() - j);
            for (int32 k = 0; k < take; k++)
            {
                auto* info = pkt.add_monsters();
                *info = *toSpawnM[j + k]->GetMonsterInfo();
            }
            j += take;
            pkts.push_back(std::move(pkt));
        }

        // ---- projectiles batching ----
        const int32 batchProj = 200; //  [ADD] 필요하면 숫자만 조절
        for (int32 t = 0; t < (int32)toSpawnPr.size(); )
        {
            Protocol::S_SPAWN pkt;
            pkt.set_snapshot_id(snapId);
            pkt.set_snapshot_begin(false);
            pkt.set_snapshot_end(false);

            int32 take = min(batchProj, (int32)toSpawnPr.size() - t);
            for (int32 k = 0; k < take; k++)
            {
                auto* info = pkt.add_projectiles();
                *info = *toSpawnPr[t + k]->GetProjectileInfo();
            }
            t += take;
            pkts.push_back(std::move(pkt));
        }

        // 스폰 0개여도 begin/end 닫기
        if (pkts.empty())
        {
            Protocol::S_SPAWN emptyPkt;
            emptyPkt.set_snapshot_id(snapId);
            emptyPkt.set_snapshot_begin(true);
            emptyPkt.set_snapshot_end(true);
            session->Send(ClientPacketHandler::MakeSendBuffer(emptyPkt));
        }
        else
        {
            pkts.front().set_snapshot_begin(true);
            pkts.back().set_snapshot_end(true);

            for (auto& pkt : pkts)
                session->Send(ClientPacketHandler::MakeSendBuffer(pkt));
        }
    }
    else
    {
        //  평소 모드: 기존 함수 그대로 + 투사체 스폰만 추가 전송(스냅샷 플래그 없음)
        SendSpawnBatchedToMe(session, toSpawnP, toSpawnM, false, 0);

        if (!toSpawnPr.empty())
        {
            const int32 batchProj = 200;
            for (int32 t = 0; t < (int32)toSpawnPr.size(); )
            {
                Protocol::S_SPAWN pkt;
                int32 take = min(batchProj, (int32)toSpawnPr.size() - t);

                for (int32 k = 0; k < take; k++)
                {
                    auto* info = pkt.add_projectiles();
                    *info = *toSpawnPr[t + k]->GetProjectileInfo();
                }

                t += take;
                session->Send(ClientPacketHandler::MakeSendBuffer(pkt));
            }
        }
    }

    // === 3) 대칭 업데이트: 플레이어는 기존 그대로 ===
    for (const PlayerRef& other : toSpawnP)
    {
        if (!other) continue;
        auto& oVis = other->VisiblePlayers_ActorOnly();

        if (oVis.insert(meId).second)
        {
            Protocol::S_SPAWN pkt;
            auto* info = pkt.add_players();
            *info = *me->GetPlayerInfo();

            SendToPlayer(other->GetPlayerId(), ClientPacketHandler::MakeSendBuffer(pkt));
        }
    }

    for (uint64 pid : toDespawnP)
    {
        PlayerRef other = FindPlayer_ActorOnly(pid);
        if (!other) continue;

        auto& oVis = other->VisiblePlayers_ActorOnly();
        if (oVis.erase(meId) > 0)
        {
            Protocol::S_DESPAWN pkt;
            pkt.add_objectids(meId);
            SendToPlayer(pid, ClientPacketHandler::MakeSendBuffer(pkt));
        }
    }

    // === 4) 내 visible set 갱신 ===
    oldP = std::move(newVisPlayers);
    oldM = std::move(newVisMonsters);
    oldPr = std::move(newVisProjectiles); //  [ADD]

    // === 5) lazy update state 갱신 ===
    me->SetLastAoiPos_ActorOnly(*me->GetPosInfo());
    me->SetLastAoiTickMs_ActorOnly(::GetTickCount64());
}

int32 GameRoom::EffectiveAoiRadiusCells() const
{
    const int32 cell = _grid.GetCellSize();
    const int32 need = (int32)(_interestRadius / (float)cell) + 1; // floor + 1
    return max(_aoiNeighborRadiusCells, need);
}
