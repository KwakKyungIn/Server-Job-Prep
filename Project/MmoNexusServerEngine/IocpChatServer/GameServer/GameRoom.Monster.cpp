#include "pch.h"
#include "GameRoom.h"
#include "GameMap.h"
#include "Player.h"
#include "PlayerSession.h"
#include "ClientPacketHandler.h"
#include "Monster.h"
#include "DataManager.h"
#include "ObjectUtils.h"
#include "BattleSystem.h"
#include "Zone.h"
#include "Creature.h"
#include "GameSessionManager.h"
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
	if (monster == nullptr) return;
	if (_monsters.find(monster->GetObjectId()) != _monsters.end())
		return;

	_monsters.insert({ monster->GetObjectId(), monster });
	monster->SetRoom(shared_from_this());

	// [CHANGED] AOI: zoneIndex 계산을 SpatialGrid로
	int32 zoneIndex = _grid.GetZoneIndex(*monster->GetPosInfo());
	monster->SetZoneIndex(zoneIndex);

	printf("👾 [EnterMonster] Monster ID=%llu entering Zone[%d]\n",
		monster->GetObjectId(), zoneIndex);
	printf("    Position: (%.1f, %.1f, %.1f)\n",
		monster->GetPosInfo()->x(),
		monster->GetPosInfo()->y(),
		monster->GetPosInfo()->z());

	// [CHANGED] 해당 Zone의 몬스터 집합에 추가
	Zone& zone = _grid.GetZone(zoneIndex);
	zone.monsters.insert(monster);

	printf("    Zone[%d] now has %zu monsters\n",
		zoneIndex, zone.monsters.size());

	// 주변 유저들에게 몬스터 스폰 알림
	{
		Protocol::S_SPAWN spawnPkt;
		Protocol::MonsterInfo* mInfo = spawnPkt.add_monsters();
		*mInfo = *monster->GetMonsterInfo();

		SendBufferRef sendBuffer = ClientPacketHandler::MakeSendBuffer(spawnPkt);
		BroadcastToZone(sendBuffer, zoneIndex);
	}
}

void GameRoom::LeaveMonster(uint64 objectId)
{
	auto it = _monsters.find(objectId);
	if (it == _monsters.end()) return;

	MonsterRef monster = it->second;
	int32 zoneIndex = monster->GetZoneIndex();

	// [CHANGED] AOI: Zone에서 제거
	int32 totalZones = _grid.GetGridSizeX() * _grid.GetGridSizeY();
	if (zoneIndex >= 0 && zoneIndex < totalZones)
	{
		Zone& zone = _grid.GetZone(zoneIndex);
		zone.monsters.erase(monster);
	}

	_monsters.erase(objectId);
	monster->SetRoom(nullptr);

	// 주변 유저들에게 몬스터 사라짐 알림
	// 주변 유저들에게 몬스터 사라짐 알림 (9-grid 기준)
	{
		Protocol::S_DESPAWN despawnPkt;
		despawnPkt.add_objectids(objectId);
		SendBufferRef sendBuffer = ClientPacketHandler::MakeSendBuffer(despawnPkt);

		Vector<Zone*> nearbyZones;
		_grid.GetNearbyZones(zoneIndex, nearbyZones);

		for (Zone* zone : nearbyZones)
		{
			for (const PlayerRef& p : zone->players)
			{
				SendToPlayer(p->GetPlayerId(), sendBuffer);
			}
		}
	}

}

void GameRoom::OnMonsterMoved(MonsterRef monster)
{
	if (monster == nullptr)
		return;

	int32 oldZoneIndex = monster->GetZoneIndex();
	int32 newZoneIndex = _grid.GetZoneIndex(*monster->GetPosInfo());

	int32 totalZones = _grid.GetGridSizeX() * _grid.GetGridSizeY();

	// 존 변경 처리
	if (newZoneIndex != oldZoneIndex)
	{
		if (oldZoneIndex >= 0 && oldZoneIndex < totalZones)
		{
			Zone& oldZone = _grid.GetZone(oldZoneIndex);
			oldZone.monsters.erase(monster);
		}

		Zone& newZone = _grid.GetZone(newZoneIndex);
		newZone.monsters.insert(monster);
		monster->SetZoneIndex(newZoneIndex);
	}

	// 위치 브로드캐스트
	Protocol::S_MOVE movePkt;
	movePkt.set_objectid(monster->GetObjectId());
	*movePkt.mutable_posinfo() = *monster->GetPosInfo();

	SendBufferRef sendBuffer = ClientPacketHandler::MakeSendBuffer(movePkt);

	int32 zoneIndex = monster->GetZoneIndex();
	BroadcastToZone(sendBuffer, zoneIndex);
}

PlayerRef GameRoom::FindNearestPlayer(Protocol::PositionInfo* pos, float range)
{
	// [CHANGED] AOI: 그리드에 직접 문의
	int32 zoneIndex = _grid.GetZoneIndex(*pos);

	Vector<Zone*> zones;
	_grid.GetNearbyZones(zoneIndex, zones);

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

