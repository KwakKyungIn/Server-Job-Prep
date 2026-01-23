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

// 2D 거리 판별 함수
// sqrt 연산은 CPU를 많이 먹으니까, 거리의 제곱끼리 비교하는 방식으로 최적화함
// 서버에서 빈번하게 호출되는 함수라 이런 사소한 최적화가 중요함
bool GameRoom::PassDistance2D(const Protocol::PositionInfo& a, const Protocol::PositionInfo& b, float r) const
{
    const float dx = a.x() - b.x();
    const float dz = a.z() - b.z();
    const float rr = r * r;
    return (dx * dx + dz * dz) <= rr;
}

// 내 주변 존(Zone)에 있는 오브젝트들을 1차적으로 긁어모으는 함수 (Broad Phase)
// 전체 맵을 다 뒤지면 느리니까, Grid 시스템을 통해 인접한 구역만 탐색함
void GameRoom::CollectCandidates(int32 zoneIndex, Vector<PlayerRef>& outPlayers, Vector<MonsterRef>& outMonsters)
{
    outPlayers.clear();
    outMonsters.clear();

    Vector<Zone*> zones;
    // 현재 내 위치 기준 시야 반경 내에 있는 Zone들을 가져옴
    _grid.GetNearbyZones(zoneIndex, EffectiveAoiRadiusCells(), zones);

    for (Zone* z : zones)
    {
        for (const PlayerRef& p : z->players)
            if (p) outPlayers.push_back(p);

        for (const MonsterRef& m : z->monsters)
            if (m) outMonsters.push_back(m);
    }
}

// AOI 갱신이 필요한지 검사하는 스로틀링(Throttling) 함수
// 매 프레임 갱신하면 서버 터지니까, 일정 거리 이상 움직였거나 시간이 좀 지났을 때만 갱신함
bool GameRoom::ShouldUpdateAOI(PlayerRef me, bool zoneChanged) const
{
    if (!me) return false;
    // 존이 바뀌었으면 무조건 갱신해야 함 (새로운 구역 데이터를 로딩해야 하니까)
    if (zoneChanged) return true;

    const uint64 now = ::GetTickCount64();
    const uint64 last = me->LastAoiTickMs_ActorOnly();

    // 시간 기반 체크: 마지막 갱신 후 일정 시간(lazyUpdateTickMs)이 지났으면 갱신
    if (now - last >= _lazyUpdateTickMs)
        return true;

    // 거리 기반 체크: 마지막 갱신 위치에서 일정 거리(lazyUpdateDist) 이상 이동했으면 갱신
    const auto& cur = *me->GetPosInfo();
    const auto& lastPos = me->LastAoiPos_ActorOnly();

    const float dx = cur.x() - lastPos.x();
    const float dz = cur.z() - lastPos.z();

    return (dx * dx + dz * dz) >= (_lazyUpdateDist * _lazyUpdateDist);
}

// 나에게 주변 플레이어/몬스터들의 스폰 패킷을 보내는 함수 (Batching 처리)
// 한 번에 너무 많은 패킷을 보내면 소켓 버퍼가 터질 수 있어서 적당히 잘라서 보냄
void GameRoom::SendSpawnBatchedToMe(PlayerSessionRef session,
    const Vector<PlayerRef>& spawnPlayers,
    const Vector<MonsterRef>& spawnMonsters,
    bool snapshotMode,
    uint32 snapshotId)
{
    if (!session) return;

    // 스냅샷 모드: 맵 이동 직후처럼 대량의 데이터를 받을 때 사용
    // Begin -> Data -> End 순서로 보내서 클라이언트가 로딩 완료 시점을 알 수 있게 함
    Vector<Protocol::S_SPAWN> pkts;
    pkts.reserve(
        (spawnPlayers.size() + _batchSpawnPlayers - 1) / _batchSpawnPlayers +
        (spawnMonsters.size() + _batchSpawnMonsters - 1) / _batchSpawnMonsters +
        1);

    // 플레이어 스폰 패킷 배칭 (N명씩 묶어서 패킷 하나로 만듦)
    for (int32 i = 0; i < (int32)spawnPlayers.size(); )
    {
        Protocol::S_SPAWN pkt;

        // 메타데이터 설정 (스냅샷 ID 등)
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

    // 몬스터 스폰 패킷 배칭
    for (int32 j = 0; j < (int32)spawnMonsters.size(); )
    {
        Protocol::S_SPAWN pkt;

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

    // 예외 처리: 스냅샷 모드인데 보낼 데이터가 하나도 없을 경우
    // 그래도 빈 패킷에 begin/end를 찍어서 보내야 클라가 무한 로딩에 안 걸림
    if (snapshotMode && pkts.empty())
    {
        Protocol::S_SPAWN emptyPkt;
        emptyPkt.set_snapshot_id(snapshotId);
        emptyPkt.set_snapshot_begin(true);
        emptyPkt.set_snapshot_end(true);
        session->Send(ClientPacketHandler::MakeSendBuffer(emptyPkt));
        return;
    }

    // 패킷 리스트의 맨 처음과 맨 끝에 플래그 마킹
    if (snapshotMode && !pkts.empty())
    {
        pkts.front().set_snapshot_begin(true);
        pkts.back().set_snapshot_end(true);
    }

    // 실제 전송
    for (auto& pkt : pkts)
    {
        session->Send(ClientPacketHandler::MakeSendBuffer(pkt));
    }
}

// 시야에서 사라진 객체들의 ID를 모아서 클라에 전송
void GameRoom::SendDespawnBatchedToMe(PlayerSessionRef session, const Vector<uint64>& objectIds)
{
    if (!session) return;
    int32 i = 0;

    // 이것도 ID 목록이 너무 길면 잘라서 보냄
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

// 네비게이션 메쉬 상에서 연결된 구역 ID 반환
// 벽 너머에 있는 적을 보지 못하게 하거나, 층이 다를 때 구분하기 위함
uint32 GameRoom::GetConnectivityId_ActorOnly(const Protocol::PositionInfo& pos) const
{
    if (!_map) return 0;
    return _map->GetConnectivityId(pos.x(), pos.y(), pos.z());
}


// [핵심] AOI 메인 로직
// 내 주변에 누가 새로 들어왔고(Spawn), 누가 나갔는지(Despawn) 계산해서 처리함
void GameRoom::UpdateAOI(PlayerSessionRef session, PlayerRef me, bool forceFullSnapshot)
{
    if (!session || !me) return;

    const uint64 meId = me->GetPlayerId();
    const auto& myPos = *me->GetPosInfo();
    const uint32 myConn = GetConnectivityId_ActorOnly(myPos);

    // 1. 후보군 수집 (Broad Phase)
    // Grid 시스템을 이용해 대충 근처에 있는 애들을 긁어옴
    Vector<PlayerRef> candPlayers;
    Vector<MonsterRef> candMonsters;
    CollectCandidates(me->GetZoneIndex(), candPlayers, candMonsters);

    // 투사체(화살, 스킬 등)도 시야 처리가 필요하므로 별도로 수집
    Vector<Zone*> candZones;
    _grid.GetNearbyZones(me->GetZoneIndex(), EffectiveAoiRadiusCells(), candZones);

    // 2. 정확한 가시성 검사 (Narrow Phase)
    // 실제로 거리가 닿는지, 벽으로 막혀있지는 않은지 확인 후 Set에 넣음
    HashSet<uint64> newVisPlayers;
    HashSet<uint64> newVisMonsters;
    HashSet<uint64> newVisProjectiles;

    // 플레이어 검사
    for (const PlayerRef& other : candPlayers)
    {
        if (!other) continue;
        const uint64 oid = other->GetPlayerId();
        if (oid == meId) continue; // 나는 제외

        const auto& op = *other->GetPosInfo();

        // 2D 거리 체크 (원형 시야)
        if (!PassDistance2D(myPos, op, _interestRadius))
            continue;

        // Connectivity 체크 (같은 층/구역인지)
        const uint32 oConn = GetConnectivityId_ActorOnly(op);
        if (oConn != myConn)
            continue;

        newVisPlayers.insert(oid);
    }

    // 몬스터 검사
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

    // 투사체 검사 (추가된 로직)
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

    // 3. Diff Algorithm (Delta Update)
    // 이전 프레임에 보였던 목록(Old) vs 지금 보이는 목록(New) 비교
    auto& oldP = me->VisiblePlayers_ActorOnly();
    auto& oldM = me->VisibleMonsters_ActorOnly();
    auto& oldPr = me->VisibleProjectiles_ActorOnly();

    // 강제 스냅샷 모드면, 이전에 보던걸 싹 다 잊어버리고 새로 그림
    // (맵 이동 직후 등에 사용)
    if (forceFullSnapshot)
    {
        // 몬스터/투사체 입장에서 "나(Player)"라는 관찰자를 제거
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

    // 4. 변경점 계산 (Spawn / Despawn 리스트 작성)
    Vector<uint64> toDespawnP;
    Vector<uint64> toDespawnM;
    Vector<uint64> toDespawnPr;
    Vector<PlayerRef> toSpawnP;
    Vector<MonsterRef> toSpawnM;
    Vector<ProjectileRef> toSpawnPr;

    // 플레이어: Old에는 있는데 New에는 없으면 -> Despawn (시야 밖으로 나감)
    for (uint64 pid : oldP)
        if (newVisPlayers.find(pid) == newVisPlayers.end())
            toDespawnP.push_back(pid);

    // 플레이어: New에는 있는데 Old에는 없으면 -> Spawn (시야 안으로 들어옴)
    for (uint64 pid : newVisPlayers)
        if (oldP.find(pid) == oldP.end())
        {
            PlayerRef p = FindPlayer_ActorOnly(pid);
            if (p) toSpawnP.push_back(p);
        }

    // 몬스터 Despawn 처리
    for (uint64 mid : oldM)
        if (newVisMonsters.find(mid) == newVisMonsters.end())
            toDespawnM.push_back(mid);

    // 몬스터 Spawn 처리
    for (uint64 mid : newVisMonsters)
        if (oldM.find(mid) == oldM.end())
        {
            auto it = _monsters.find(mid);
            if (it != _monsters.end() && it->second)
                toSpawnM.push_back(it->second);
        }

    // 투사체 Despawn 처리
    for (uint64 prid : oldPr)
        if (newVisProjectiles.find(prid) == newVisProjectiles.end())
            toDespawnPr.push_back(prid);

    // 투사체 Spawn 처리
    for (uint64 prid : newVisProjectiles)
        if (oldPr.find(prid) == oldPr.end())
        {
            auto it = _projectiles.find(prid);
            if (it != _projectiles.end() && it->second)
                toSpawnPr.push_back(it->second);
        }

    // 5. 몬스터/투사체 Viewers 목록 동기화
    // 몬스터 AI가 동작하려면 주변에 누가 있는지 알아야 하므로 Viewers 목록을 관리함
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

    // 투사체도 Viewers 관리 (나중에 피격 판정이나 이펙트 보여줄 때 필요할 수 있음)
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

    // 6. 클라이언트에게 패킷 전송

    // (1) 나에게서 사라진 애들 처리 (Despawn)
    {
        Vector<uint64> ids;
        ids.reserve(toDespawnP.size() + toDespawnM.size() + toDespawnPr.size());
        for (uint64 pid : toDespawnP) ids.push_back(pid);
        for (uint64 mid : toDespawnM) ids.push_back(mid);
        for (uint64 prid : toDespawnPr) ids.push_back(prid);

        if (!ids.empty())
            SendDespawnBatchedToMe(session, ids);
    }

    // (2) 나에게 새로 나타난 애들 처리 (Spawn)
    if (forceFullSnapshot)
    {
        // 스냅샷 모드면 플레이어, 몬스터, 투사체를 하나의 흐름으로 묶어서 보냄
        const uint32 snapId = me->NextSnapshotSeq_ActorOnly();
        Vector<Protocol::S_SPAWN> pkts;

        // 플레이어 배칭
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

        // 몬스터 배칭
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

        // 투사체 배칭
        const int32 batchProj = 200;
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

        // 스폰이 0개여도 완료 패킷은 보내야 함
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
            // Begin/End 플래그 찍어서 전송
            pkts.front().set_snapshot_begin(true);
            pkts.back().set_snapshot_end(true);

            for (auto& pkt : pkts)
                session->Send(ClientPacketHandler::MakeSendBuffer(pkt));
        }
    }
    else
    {
        // 일반 모드: 스냅샷 플래그 없이 그냥 스폰 패킷 전송
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

    // 7. 대칭 업데이트 (Symmetric Update)
    // 내가 A를 보게 되었다면, A에게도 "나"를 보여줘야 함
    for (const PlayerRef& other : toSpawnP)
    {
        if (!other) continue;
        auto& oVis = other->VisiblePlayers_ActorOnly();

        // 상대방의 Visible List에 나를 추가하고, 성공하면 상대에게 내 정보를 보냄
        if (oVis.insert(meId).second)
        {
            Protocol::S_SPAWN pkt;
            auto* info = pkt.add_players();
            *info = *me->GetPlayerInfo();

            SendToPlayer(other->GetPlayerId(), ClientPacketHandler::MakeSendBuffer(pkt));
        }
    }

    // 내가 A를 못 보게 되었다면(멀어짐), A에게서도 "나"를 지워야 함
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

    // 8. 내 상태 업데이트
    // 현재 보이는 목록을 Old 목록으로 저장
    oldP = std::move(newVisPlayers);
    oldM = std::move(newVisMonsters);
    oldPr = std::move(newVisProjectiles);

    // 마지막으로 AOI 체크한 위치와 시간을 저장 (스로틀링용)
    me->SetLastAoiPos_ActorOnly(*me->GetPosInfo());
    me->SetLastAoiTickMs_ActorOnly(::GetTickCount64());
}

int32 GameRoom::EffectiveAoiRadiusCells() const
{
    // 시야 반경이 그리드 셀 몇 개에 해당하는지 계산
    // 올림 처리(floor + 1)해서 여유 있게 가져옴
    const int32 cell = _grid.GetCellSize();
    const int32 need = (int32)(_interestRadius / (float)cell) + 1;
    return max(_aoiNeighborRadiusCells, need);
}