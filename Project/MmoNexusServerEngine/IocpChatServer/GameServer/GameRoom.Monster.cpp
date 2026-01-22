#include "pch.h"
#include "GameRoom.h"
#include "Player.h"
#include "Monster.h"
#include "RoomManager.h"
#include "GameRoom.Net.h"

void GameRoom::Update()
{
	//  deltaMs 계산(투사체만 사용해도 됨)
	const uint64 now = ::GetTickCount64();
	if (_lastUpdateMs == 0)
		_lastUpdateMs = now;

	uint64 deltaMs = now - _lastUpdateMs;
	if (deltaMs > 100) deltaMs = 100;
	_lastUpdateMs = now;

	// 몬스터 AI
	for (auto& item : _monsters)
		item.second->Update(now, deltaMs);

	//  Projectile tick
	UpdateProjectiles(deltaMs);

	//  Trade timeout tick
	UpdateTrades_ActorOnly(now);
}

void GameRoom::EnterMonster(MonsterRef monster)
{
	if (!monster) return;
	const uint64 mid = monster->GetObjectId();
	if (_monsters.find(mid) != _monsters.end())
		return;

	_monsters.insert({ mid, monster });
	monster->SetRoom(shared_from_this());

	int32 zoneIndex = _grid.GetZoneIndex(*monster->GetPosInfo());
	monster->SetZoneIndex(zoneIndex);
	_grid.GetZone(zoneIndex).monsters.insert(monster);

	// v2: 주변 플레이어 중 "필터 통과한 애들"에게만 spawn
	Vector<Zone*> zones;
	_grid.GetNearbyZones(zoneIndex, EffectiveAoiRadiusCells(), zones);

	Protocol::S_SPAWN spawnPkt;
	auto* mInfo = spawnPkt.add_monsters();
	*mInfo = *monster->GetMonsterInfo();
	SendBufferRef sb = ClientPacketHandler::MakeSendBuffer(spawnPkt);

	auto& viewers = monster->Viewers_ActorOnly();
	viewers.clear();

	const auto& mp = *monster->GetPosInfo();
	const uint32 mConn = GetConnectivityId_ActorOnly(mp);

	for (Zone* z : zones)
	{
		for (const PlayerRef& p : z->players)
		{
			if (!p) continue;

			const auto& pp = *p->GetPosInfo();
			if (!PassDistance2D(mp, pp, _interestRadius))
				continue;

			const uint32 pConn = GetConnectivityId_ActorOnly(pp);
			if (pConn != mConn)
				continue;

			// 서버 상태 동기화: 양쪽 set 갱신
			if (p->VisibleMonsters_ActorOnly().insert(mid).second)
			{
				viewers.insert(p->GetPlayerId());
				SendToPlayer(p->GetPlayerId(), sb);
			}
			else
			{
				// 이미 보던 상태면 viewers만 보정
				viewers.insert(p->GetPlayerId());
			}
		}
	}
}

void GameRoom::LeaveMonster(uint64 objectId)
{
	auto it = _monsters.find(objectId);
	if (it == _monsters.end()) return;

	MonsterRef m = it->second;
	if (!m) return;

	const int32 zoneIndex = m->GetZoneIndex();

	// v2: 이 몬스터를 보던 플레이어들에게만 despawn
	{
		Protocol::S_DESPAWN pkt;
		pkt.add_objectids(objectId);
		SendBufferRef sb = ClientPacketHandler::MakeSendBuffer(pkt);

		// viewers 순회하면서 플레이어 visibleMonsters에서도 제거
		auto& viewers = m->Viewers_ActorOnly();
		for (uint64 pid : viewers)
		{
			PlayerRef p = FindPlayer_ActorOnly(pid);
			if (p)
				p->VisibleMonsters_ActorOnly().erase(objectId);

			SendToPlayer(pid, sb);
		}
		viewers.clear();
	}

	// grid / map remove
	int32 totalZones = _grid.GetGridSizeX() * _grid.GetGridSizeY();
	if (zoneIndex >= 0 && zoneIndex < totalZones)
		_grid.GetZone(zoneIndex).monsters.erase(m);

	_monsters.erase(objectId);
	m->SetRoom(nullptr);
}

void GameRoom::OnMonsterMoved(MonsterRef monster)
{
	if (!monster) return;

	const uint64 now = ::GetTickCount64();
	const uint64 mid = monster->GetObjectId();

	int32 oldZoneIndex = monster->GetZoneIndex();
	int32 newZoneIndex = _grid.GetZoneIndex(*monster->GetPosInfo());
	const bool zoneChanged = (newZoneIndex != oldZoneIndex);

	// 0) zone membership은 항상 최신
	if (zoneChanged)
	{
		int32 totalZones = _grid.GetGridSizeX() * _grid.GetGridSizeY();
		if (oldZoneIndex >= 0 && oldZoneIndex < totalZones)
			_grid.GetZone(oldZoneIndex).monsters.erase(monster);

		_grid.GetZone(newZoneIndex).monsters.insert(monster);
		monster->SetZoneIndex(newZoneIndex);
	}

	// 1)  Cheap: 기존 viewers에게 MOVE만
	{
		Protocol::S_MOVE movePkt;
		movePkt.set_objectid(mid);
		*movePkt.mutable_posinfo() = *monster->GetPosInfo();
		SendBufferRef sb = ClientPacketHandler::MakeSendBuffer(movePkt);

		auto& viewers = monster->Viewers_ActorOnly();
		for (uint64 pid : viewers)
			SendToPlayer(pid, sb);
	}

	// 2)  Expensive 트리거 판단
	float lastX, lastZ;
	monster->GetLastAoiExpensivePos(lastX, lastZ);

	const float dx = monster->GetPosInfo()->x() - lastX;
	const float dz = monster->GetPosInfo()->z() - lastZ;
	const float moved = std::sqrt(dx * dx + dz * dz);

	const uint64 lastMs = monster->GetLastAoiExpensiveMs();

	const bool needExpensive =
		zoneChanged ||
		(lastMs == 0) ||
		(now - lastMs >= _lazyUpdateTickMs) ||
		(moved >= _lazyUpdateDist);

	if (!needExpensive)
		return;

	// 3) Expensive: (너 기존 코드 그대로) newViewers 계산 + spawn/despawn diff
	// ---- 새 viewers 계산 ----
	HashSet<uint64> newViewers;

	Vector<Zone*> zones;
	_grid.GetNearbyZones(monster->GetZoneIndex(), EffectiveAoiRadiusCells(), zones);

	const auto& mp = *monster->GetPosInfo();
	const uint32 mConn = GetConnectivityId_ActorOnly(mp);

	for (Zone* z : zones)
	{
		for (const PlayerRef& p : z->players)
		{
			if (!p) continue;

			const auto& pp = *p->GetPosInfo();
			if (!PassDistance2D(mp, pp, _interestRadius))
				continue;

			const uint32 pConn = GetConnectivityId_ActorOnly(pp);
			if (pConn != mConn)
				continue;

			newViewers.insert(p->GetPlayerId());
		}
	}

	auto& oldViewers = monster->Viewers_ActorOnly();

	// ---- despawn: old - new ----
	{
		Protocol::S_DESPAWN pkt;
		pkt.add_objectids(mid);
		SendBufferRef sb = ClientPacketHandler::MakeSendBuffer(pkt);

		for (uint64 pid : oldViewers)
		{
			if (newViewers.find(pid) != newViewers.end())
				continue;

			PlayerRef p = FindPlayer_ActorOnly(pid);
			if (p) p->VisibleMonsters_ActorOnly().erase(mid);
			SendToPlayer(pid, sb);
		}
	}

	// ---- spawn: new - old ----
	{
		Protocol::S_SPAWN pkt;
		auto* mInfo = pkt.add_monsters();
		*mInfo = *monster->GetMonsterInfo();
		SendBufferRef sb = ClientPacketHandler::MakeSendBuffer(pkt);

		for (uint64 pid : newViewers)
		{
			if (oldViewers.find(pid) != oldViewers.end())
				continue;

			PlayerRef p = FindPlayer_ActorOnly(pid);
			if (p) p->VisibleMonsters_ActorOnly().insert(mid);
			SendToPlayer(pid, sb);
		}
	}

	oldViewers = std::move(newViewers);

	monster->SetLastAoiExpensiveMs(now);
	monster->SetLastAoiExpensivePos(monster->GetPosInfo()->x(), monster->GetPosInfo()->z());
}

PlayerRef GameRoom::FindNearestPlayer(Protocol::PositionInfo* pos, float range)
{
	if (!pos) return nullptr;

	// [CHANGED] AOI: 그리드에 직접 문의
	const int32 zoneIndex = _grid.GetZoneIndex(*pos);

	Vector<Zone*> zones;
	_grid.GetNearbyZones(zoneIndex, EffectiveAoiRadiusCells(), zones);

	//  Connectivity 필터 (벽 너머 타겟 금지)
	const uint32 myConn = GetConnectivityId_ActorOnly(*pos);

	PlayerRef target = nullptr;
	const float rangeSqr = range * range;
	float minDistSqr = rangeSqr;

	for (Zone* zone : zones)
	{
		for (const PlayerRef& player : zone->players)
		{
			if (!player) continue;
			if (player->GetStatInfo() && player->GetStatInfo()->hp() <= 0) continue;

			const auto& pp = *player->GetPosInfo();

			//  Range 컷 (성능 + 정확도)
			const float dx = pp.x() - pos->x();
			const float dz = pp.z() - pos->z();
			const float distSqr = dx * dx + dz * dz;
			if (distSqr > rangeSqr)
				continue;

			//  Connectivity 컷
			if (GetConnectivityId_ActorOnly(pp) != myConn)
				continue;

			if (distSqr < minDistSqr)
			{
				minDistSqr = distSqr;
				target = player;
			}
		}
	}

	return target;
}

