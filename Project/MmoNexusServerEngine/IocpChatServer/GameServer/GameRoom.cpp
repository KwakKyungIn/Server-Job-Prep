#include "pch.h"
#include "GameRoom.h"
#include "GameMap.h"
#include "Player.h"
#include "PlayerSession.h"
#include "ClientPacketHandler.h"
#include "Monster.h"
#include "DataManager.h"
#include "ObjectUtils.h"

GameRoom::GameRoom()
{
	_jobQueue = MakeShared<JobQueue>();
}

GameRoom::~GameRoom()
{
}

void GameRoom::Init(int32 mapId, int32 sizeX, int32 sizeY, int32 zoneSize)
{
	_map = MakeShared<GameMap>();
	_map->Init(mapId, sizeX, sizeY);

	// [Spatial Partitioning Init]
	_zoneCellSize = zoneSize;
	_gridSizeX = (sizeX + zoneSize - 1) / zoneSize;
	_gridSizeY = (sizeY + zoneSize - 1) / zoneSize;

	_zones.resize(_gridSizeX * _gridSizeY);

	printf("[GameRoom] Init MapId: %d, Grid: %dx%d, CellSize: %d\n", mapId, _gridSizeX, _gridSizeY, zoneSize);





	// [Test Spawn] 테스트용 몬스터 1마리 소환
	// 나중엔 GenFile(배치 파일)이나 DB에서 읽어서 루프 돌려야 함
	MonsterRef slime = ObjectPool<Monster>::MakeShared();
	slime->Init(1); // 템플릿 ID 1번 (슬라임 킹)

	// 위치 설정 (플레이어 스폰 위치 근처인 55, 0, 55 정도로 설정)
	slime->GetPosInfo()->set_x(55.0f);
	slime->GetPosInfo()->set_y(0.0f);
	slime->GetPosInfo()->set_z(55.0f);
	slime->GetPosInfo()->set_yaw(0.0f);

	// 방에 입장 (이때 EnterMonster가 호출되면서 Zone에 등록됨)
	EnterMonster(slime);

	printf("👾 [Test] Slime_King Spawned at (55, 0, 55)\n");


}

void GameRoom::Update()
{
	// 몬스터 AI 구동
	for (auto& item : _monsters)
	{
		MonsterRef monster = item.second;
		monster->Update();
	}
}

void GameRoom::Enter(PlayerSessionRef session)
{
	PlayerRef player = session->GetPlayer();
	if (player == nullptr) return;

	if (_players.find(player->GetPlayerId()) != _players.end()) return;

	player->SetRoom(shared_from_this());
	_players.insert({ player->GetPlayerId(), player });

	int32 zoneIndex = GetZoneIndex(*player->GetPosInfo());
	player->SetZoneIndex(zoneIndex);
	_zones[zoneIndex].players.insert(player);

	// 1. 주변 플레이어들에게 "나(플레이어) 등장" 알림
	{
		Protocol::S_SPAWN spawnPkt;
		Protocol::PlayerInfo* pInfo = spawnPkt.add_players();
		*pInfo = *player->GetPlayerInfo();
		SendBufferRef sendBuffer = ClientPacketHandler::MakeSendBuffer(spawnPkt);

		Vector<Zone*> nearbyZones;
		GetNearbyZones(zoneIndex, nearbyZones);

		for (Zone* zone : nearbyZones)
		{
			for (const PlayerRef& other : zone->players)
			{
				if (other != player)
					other->GetSession()->Send(sendBuffer);
			}
		}
	}

	// 2. 나에게 "주변 정보(플레이어 + 몬스터)" 알림
	{
		Vector<Zone*> nearbyZones;
		GetNearbyZones(zoneIndex, nearbyZones);

		Protocol::S_SPAWN spawnPkt;

		for (Zone* zone : nearbyZones)
		{
			// 플레이어 추가
			for (const PlayerRef& other : zone->players)
			{
				if (other != player)
				{
					Protocol::PlayerInfo* pInfo = spawnPkt.add_players();
					*pInfo = *other->GetPlayerInfo();
				}
			}
			// [New] 몬스터 추가
			for (const MonsterRef& monster : zone->monsters)
			{
				Protocol::MonsterInfo* mInfo = spawnPkt.add_monsters();
				*mInfo = *monster->GetMonsterInfo();
			}
		}

		// 주변에 뭐라도(플레이어든 몬스터든) 있으면 전송
		if (spawnPkt.players_size() > 0 || spawnPkt.monsters_size() > 0)
		{
			SendBufferRef sendBuffer = ClientPacketHandler::MakeSendBuffer(spawnPkt);
			session->Send(sendBuffer);
		}
	}
}

void GameRoom::Leave(PlayerSessionRef session)
{
	PlayerRef player = session->GetPlayer();
	if (player == nullptr) return;

	uint64 playerId = player->GetPlayerId();

	if (_players.find(playerId) == _players.end()) return;

	int32 zoneIndex = player->GetZoneIndex();

	// 1. Zone에서 제거
	if (zoneIndex >= 0 && zoneIndex < _zones.size())
	{
		_zones[zoneIndex].players.erase(player);
	}

	// 2. 전체 명단 제거
	_players.erase(playerId);
	player->SetRoom(nullptr);

	// 3. [Broadcast] 주변 유저들에게 "나 나갔음" 알림 (S_DESPAWN)
	{
		Protocol::S_DESPAWN despawnPkt;
		despawnPkt.add_objectids(playerId);
		SendBufferRef sendBuffer = ClientPacketHandler::MakeSendBuffer(despawnPkt);

		BroadcastToZone(sendBuffer, zoneIndex, 0);
	}

	printf("[ROOM] Player %llu Left Zone[%d].\n", playerId, zoneIndex);
}

void GameRoom::HandleMove(PlayerSessionRef session, Protocol::C_MOVE pkt)
{
	PlayerRef player = session->GetPlayer();
	if (player == nullptr) return;

	uint64 playerId = player->GetPlayerId();

	if (_players.find(playerId) == _players.end()) return;

	// 1. [Validation]
	if (_map->CanGo(pkt.posinfo()) == false)
		return;

	// 2. [Zone Check]
	int32 oldZoneIndex = player->GetZoneIndex();
	int32 newZoneIndex = GetZoneIndex(pkt.posinfo());

	// 3. [Update] 정보 갱신
	player->SetPosInfo(pkt.posinfo());

	// [Case A] 같은 Zone 내 이동
	if (oldZoneIndex == newZoneIndex)
	{
		Protocol::S_MOVE movePkt;
		movePkt.set_objectid(playerId);
		*movePkt.mutable_posinfo() = pkt.posinfo();
		SendBufferRef sendBuffer = ClientPacketHandler::MakeSendBuffer(movePkt);

		BroadcastToZone(sendBuffer, newZoneIndex, playerId);
	}
	// [Case B] Zone 변경 발생
	else
	{
		Vector<int32> oldZones;
		GetNearbyZoneIndices(oldZoneIndex, oldZones);
		std::sort(oldZones.begin(), oldZones.end());

		Vector<int32> newZones;
		GetNearbyZoneIndices(newZoneIndex, newZones);
		std::sort(newZones.begin(), newZones.end());

		// (Old - New) : Despawn Group (사라져야 할 놈들)
		{
			Vector<int32> removedZones;
			std::set_difference(oldZones.begin(), oldZones.end(),
				newZones.begin(), newZones.end(),
				std::back_inserter(removedZones));

			// 나 -> 다른 사람들에게 "나 사라짐" 알림
			Protocol::S_DESPAWN despawnPkt;
			despawnPkt.add_objectids(playerId);
			SendBufferRef sendBuffer = ClientPacketHandler::MakeSendBuffer(despawnPkt);

			// 나에게 "너네 사라짐" 알림
			Protocol::S_DESPAWN despawnToMePkt;

			for (int32 zoneIdx : removedZones)
			{
				Zone& zone = _zones[zoneIdx];

				// 해당 존에 있는 플레이어 처리
				for (auto& p : zone.players)
				{
					if (p->GetPlayerId() != playerId)
					{
						p->GetSession()->Send(sendBuffer); // 걔네한테 내 정보 삭제 요청
						despawnToMePkt.add_objectids(p->GetPlayerId()); // 내 목록에 걔네 추가
					}
				}
				// [New] 해당 존에 있는 몬스터 처리
				for (auto& m : zone.monsters)
				{
					despawnToMePkt.add_objectids(m->GetObjectId());
				}
			}

			if (despawnToMePkt.objectids_size() > 0)
			{
				SendBufferRef despawnToMeBuffer = ClientPacketHandler::MakeSendBuffer(despawnToMePkt);
				session->Send(despawnToMeBuffer);
			}
		}

		// (New - Old) : Spawn Group (새로 나타날 놈들)
		{
			Vector<int32> addedZones;
			std::set_difference(newZones.begin(), newZones.end(),
				oldZones.begin(), oldZones.end(),
				std::back_inserter(addedZones));

			// 나 -> 다른 사람들에게 "나 나타남"
			Protocol::S_SPAWN mySpawnPkt;
			auto* myInfo = mySpawnPkt.add_players();
			*myInfo = *player->GetPlayerInfo();
			SendBufferRef mySpawnBuffer = ClientPacketHandler::MakeSendBuffer(mySpawnPkt);

			// 나에게 "너네 나타남" (플레이어 + 몬스터)
			Protocol::S_SPAWN othersSpawnPkt;

			for (int32 zoneIdx : addedZones)
			{
				Zone& zone = _zones[zoneIdx];

				// 플레이어 처리
				for (auto& p : zone.players)
				{
					if (p->GetPlayerId() != playerId)
					{
						p->GetSession()->Send(mySpawnBuffer); // 걔네한테 나 보냄
						auto* otherInfo = othersSpawnPkt.add_players();
						*otherInfo = *p->GetPlayerInfo(); // 내 목록에 걔네 추가
					}
				}
				// [New] 몬스터 처리
				for (auto& m : zone.monsters)
				{
					auto* mInfo = othersSpawnPkt.add_monsters();
					*mInfo = *m->GetMonsterInfo();
				}
			}

			if (othersSpawnPkt.players_size() > 0 || othersSpawnPkt.monsters_size() > 0)
			{
				SendBufferRef othersSpawnBuffer = ClientPacketHandler::MakeSendBuffer(othersSpawnPkt);
				session->Send(othersSpawnBuffer);
			}
		}

		// (Old ∩ New) : Move Group (같이 이동 중인 놈들)
		{
			Vector<int32> commonZones;
			std::set_intersection(oldZones.begin(), oldZones.end(),
				newZones.begin(), newZones.end(),
				std::back_inserter(commonZones));

			Protocol::S_MOVE movePkt;
			movePkt.set_objectid(playerId);
			*movePkt.mutable_posinfo() = pkt.posinfo();
			SendBufferRef sendBuffer = ClientPacketHandler::MakeSendBuffer(movePkt);

			for (int32 zoneIdx : commonZones)
			{
				Zone& zone = _zones[zoneIdx];
				for (auto& p : zone.players)
				{
					if (p->GetPlayerId() != playerId)
						p->GetSession()->Send(sendBuffer);
				}
			}
		}

		// 서버 내 Zone 이동 반영
		_zones[oldZoneIndex].players.erase(player);
		_zones[newZoneIndex].players.insert(player);
		player->SetZoneIndex(newZoneIndex);
	}
}

void GameRoom::EnterMonster(MonsterRef monster)
{
	if (monster == nullptr) return;

	if (_monsters.find(monster->GetObjectId()) != _monsters.end())
		return;

	_monsters.insert({ monster->GetObjectId(), monster });
	monster->SetRoom(shared_from_this());

	int32 zoneIndex = GetZoneIndex(*monster->GetPosInfo());
	monster->SetZoneIndex(zoneIndex);
	_zones[zoneIndex].monsters.insert(monster);

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

	if (zoneIndex >= 0 && zoneIndex < _zones.size())
		_zones[zoneIndex].monsters.erase(monster);

	_monsters.erase(objectId);
	monster->SetRoom(nullptr);

	// 주변 유저들에게 몬스터 사라짐 알림
	{
		Protocol::S_DESPAWN despawnPkt;
		despawnPkt.add_objectids(objectId); // 여기는 변경된 이름 그대로 사용
		SendBufferRef sendBuffer = ClientPacketHandler::MakeSendBuffer(despawnPkt);
		BroadcastToZone(sendBuffer, zoneIndex);
	}
}

PlayerRef GameRoom::FindNearestPlayer(Protocol::PositionInfo* pos, float range)
{
	int32 zoneIndex = GetZoneIndex(*pos);
	Vector<Zone*> zones;
	GetNearbyZones(zoneIndex, zones);

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

// Helper 함수들
void GameRoom::GetNearbyZoneIndices(int32 zoneIndex, Vector<int32>& outIndices)
{
	outIndices.clear();
	if (zoneIndex < 0 || zoneIndex >= _zones.size()) return;

	int32 x = zoneIndex % _gridSizeX;
	int32 y = zoneIndex / _gridSizeX;

	for (int32 dy = -1; dy <= 1; dy++)
	{
		for (int32 dx = -1; dx <= 1; dx++)
		{
			int32 nx = x + dx;
			int32 ny = y + dy;
			if (nx >= 0 && nx < _gridSizeX && ny >= 0 && ny < _gridSizeY)
			{
				int32 index = ny * _gridSizeX + nx;
				outIndices.push_back(index);
			}
		}
	}
}

int32 GameRoom::GetZoneIndex(const Protocol::PositionInfo& posInfo)
{
	int32 x = static_cast<int32>(posInfo.x());
	int32 y = static_cast<int32>(posInfo.z());

	int32 minX = _map->GetMinX();
	int32 minY = _map->GetMinY();
	int32 maxX = _map->GetMaxX();
	int32 maxY = _map->GetMaxY();

	if (x < minX) x = minX;
	if (x >= maxX) x = maxX - 1;
	if (y < minY) y = minY;
	if (y >= maxY) y = maxY - 1;

	int32 zoneX = (x - minX) / _zoneCellSize;
	int32 zoneY = (y - minY) / _zoneCellSize;

	return zoneY * _gridSizeX + zoneX;
}

void GameRoom::GetNearbyZones(int32 zoneIndex, Vector<Zone*>& outZones)
{
	outZones.clear();
	if (zoneIndex < 0 || zoneIndex >= _zones.size()) return;

	int32 x = zoneIndex % _gridSizeX;
	int32 y = zoneIndex / _gridSizeX;

	for (int32 dy = -1; dy <= 1; dy++)
	{
		for (int32 dx = -1; dx <= 1; dx++)
		{
			int32 nx = x + dx;
			int32 ny = y + dy;
			if (nx >= 0 && nx < _gridSizeX && ny >= 0 && ny < _gridSizeY)
			{
				int32 index = ny * _gridSizeX + nx;
				outZones.push_back(&_zones[index]);
			}
		}
	}
}

void GameRoom::HandleSkill(std::shared_ptr<Creature> attacker, int32 skillId)
{
	if (attacker == nullptr) return;

	// 1. 데이터 검증
	const Protocol::SkillTemplateInfo* skillData = DataManager::Instance()->GetSkillTemplate(skillId);
	if (skillData == nullptr) return;

	// 방 검증
	if (attacker->GetRoom() != shared_from_this()) return;

	Protocol::SkillType type = skillData->skilltype();
	float range = skillData->range();
	int32 damage = skillData->damage();

	bool isMonster = (attacker->GetObjectType() == Protocol::OBJECT_TYPE_MONSTER);

	// 2. 타겟 탐색 (Broad Phase)
	int32 zoneIndex = GetZoneIndex(*attacker->GetPosInfo()); // 여기서 에러나면 GetZoneIndex가 멤버 함수인지 확인
	Vector<Zone*> zones;
	GetNearbyZones(zoneIndex, zones);

	Vector<std::shared_ptr<Creature>> victims;
	for (Zone* zone : zones)
	{
		if (isMonster)
		{
			for (auto& p : zone->players) victims.push_back(p);
		}
		else
		{
			for (auto& m : zone->monsters) victims.push_back(m);
		}
	}

	// 3. 판정 (Narrow Phase)
	for (auto& victim : victims)
	{
		if (victim->GetStatInfo()->hp() <= 0) continue;

		bool isHit = false;

		switch (type)
		{
		case Protocol::SKILL_AUTO:
		{
			if (ObjectUtils::CheckCircle(*attacker->GetPosInfo(), range, *victim->GetPosInfo()))
			{
				isHit = true;
			}
		}
		break;
		// ... (다른 케이스들) ...
		}

		if (isHit)
		{
			victim->OnDamaged(attacker, damage);

			Protocol::S_CHANGE_HP changePkt;
			changePkt.set_objectid(victim->GetObjectId());
			changePkt.set_attackerid(attacker->GetObjectId());
			changePkt.set_currenthp(victim->GetStatInfo()->hp());
			changePkt.set_damage(damage);

			SendBufferRef sendBuffer = ClientPacketHandler::MakeSendBuffer(changePkt);
			BroadcastToZone(sendBuffer, zoneIndex);

			if (type == Protocol::SKILL_AUTO) break;
		}
	}
}
void GameRoom::BroadcastToZone(SendBufferRef sendBuffer, int32 zoneIndex, uint64 exceptId)
{
	Vector<Zone*> nearbyZones;
	GetNearbyZones(zoneIndex, nearbyZones);

	for (Zone* zone : nearbyZones)
	{
		for (const PlayerRef& p : zone->players)
		{
			if (p->GetPlayerId() == exceptId) continue;
			p->GetSession()->Send(sendBuffer);
		}
	}
}

void GameRoom::Broadcast(SendBufferRef sendBuffer, uint64 exceptId)
{
	for (auto& item : _players)
	{
		PlayerRef p = item.second;
		if (p->GetPlayerId() == exceptId) continue;
		p->GetSession()->Send(sendBuffer);
	}
}