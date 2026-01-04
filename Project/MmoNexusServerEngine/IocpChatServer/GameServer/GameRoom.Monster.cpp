#include "pch.h"
#include "GameRoom.h"
#include "Player.h"
#include "Monster.h"
#include "RoomManager.h"
#include "GameRoom.Net.h"

void GameRoom::Update()
{
	// 몬스터 AI 구동
	for (auto& item : _monsters)
	{
		MonsterRef monster = item.second;
		monster->Update();
	}
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

	const uint64 mid = monster->GetObjectId();

	int32 oldZoneIndex = monster->GetZoneIndex();
	int32 newZoneIndex = _grid.GetZoneIndex(*monster->GetPosInfo());

	// zone membership 이동
	if (newZoneIndex != oldZoneIndex)
	{
		int32 totalZones = _grid.GetGridSizeX() * _grid.GetGridSizeY();
		if (oldZoneIndex >= 0 && oldZoneIndex < totalZones)
			_grid.GetZone(oldZoneIndex).monsters.erase(monster);

		_grid.GetZone(newZoneIndex).monsters.insert(monster);
		monster->SetZoneIndex(newZoneIndex);
	}

	// ---- 새 viewers 계산 ----
	std::unordered_set<uint64> newViewers;

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

	// ---- move: intersection (newViewers) ----
	{
		Protocol::S_MOVE movePkt;
		movePkt.set_objectid(mid);
		*movePkt.mutable_posinfo() = *monster->GetPosInfo();
		SendBufferRef sb = ClientPacketHandler::MakeSendBuffer(movePkt);

		for (uint64 pid : newViewers)
			SendToPlayer(pid, sb);
	}

	// 최종 viewers 갱신
	oldViewers = std::move(newViewers);
}

PlayerRef GameRoom::FindNearestPlayer(Protocol::PositionInfo* pos, float range)
{
	// [CHANGED] AOI: 그리드에 직접 문의
	int32 zoneIndex = _grid.GetZoneIndex(*pos);

	Vector<Zone*> zones;
	_grid.GetNearbyZones(zoneIndex, EffectiveAoiRadiusCells(), zones);

	PlayerRef target = nullptr;
	float minDistSqr = range * range;

	for (Zone* zone : zones)
	{
		for (const PlayerRef& player : zone->players)
		{
			float dx = player->GetPosInfo()->x() - pos->x();
			float dy = player->GetPosInfo()->z() - pos->z();
			float distSqr = dx * dx + dy * dy;

			if (distSqr < minDistSqr)
			{
				minDistSqr = distSqr;
				target = player;
			}
		}
	}

	return target;
}

