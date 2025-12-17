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

GameRoom::GameRoom()
{
	_jobQueue = MakeShared<JobQueue>();
	_battle = std::make_unique<BattleSystem>(&_grid);
}

GameRoom::~GameRoom()
{
}

void GameRoom::Init(int32 channelId, int32 mapId, int32 sizeX, int32 sizeY, int32 zoneSize)
{
	_channelId = channelId;
	_mapId = mapId;

	_map = MakeShared<GameMap>();
	_map->Init(mapId, sizeX, sizeY);

	// [Spatial Partitioning Init] → SpatialGrid 사용
	// AOI 초기화
	_grid.Init(0, 0, sizeX, sizeY, zoneSize);

	printf("[GameRoom] Init MapId: %d, Grid: (%d, %d), CellSize: %d\n",
		mapId, sizeX, sizeY, zoneSize);
	/*
	// [Test Spawn] 테스트용 몬스터 1마리 소환
	MonsterRef slime = ObjectPool<Monster>::MakeShared();
	slime->Init(1); // 템플릿 ID 1번 (슬라임 킹)

	slime->GetPosInfo()->set_x(5.0f);
	slime->GetPosInfo()->set_y(0.0f);
	slime->GetPosInfo()->set_z(5.0f);
	slime->GetPosInfo()->set_yaw(0.0f);

	// 방에 입장 (이때 EnterMonster가 호출되면서 Zone에 등록됨)
	EnterMonster(slime);

	printf("👾 [Test] Slime_King Spawned at (5, 0, 5)\n");

	// 디버그용: SpatialGrid 기준으로 확인
	int32 debugZoneIndex = _grid.GetZoneIndex(*slime->GetPosInfo());
	Zone& debugZone = _grid.GetZone(debugZoneIndex);

	printf("🔍 [DEBUG] Monster Check: Slime ID %llu is in Zone [%d]. Players in Zone: %zu, Monsters in Zone: %zu\n",
		slime->GetObjectId(),
		debugZoneIndex,
		debugZone.players.size(),
		debugZone.monsters.size());

		*/
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

bool GameRoom::EnterRegister(PlayerSessionRef session)
{
	PlayerRef player = session->GetPlayer();
	if (player == nullptr) return false;

	// 이미 들어와 있으면 실패 (중복 Enter 방지)
	if (_players.find(player->GetPlayerId()) != _players.end())
		return false;

	// 1) 룸 소속 설정
	player->SetRoom(shared_from_this());
	_players.insert({ player->GetPlayerId(), player });

	// 2) AOI Zone 계산 및 등록
	int32 zoneIndex = _grid.GetZoneIndex(*player->GetPosInfo());
	player->SetZoneIndex(zoneIndex);

	Zone& enterZone = _grid.GetZone(zoneIndex);
	enterZone.players.insert(player);

	printf("🎮 [EnterRegister] Player %llu Zone[%d] at (%.1f, %.1f, %.1f)\n",
		player->GetPlayerId(), zoneIndex,
		player->GetPosInfo()->x(),
		player->GetPosInfo()->y(),
		player->GetPosInfo()->z());

	return true;
}

void GameRoom::SendEnterSpawns(PlayerSessionRef session)
{
	PlayerRef player = session->GetPlayer();
	if (player == nullptr) return;

	const int32 zoneIndex = player->GetZoneIndex();

	// 1) 주변 플레이어들에게 "나 등장" 브로드캐스트
	{
		Protocol::S_SPAWN spawnPkt;
		Protocol::PlayerInfo* pInfo = spawnPkt.add_players();
		*pInfo = *player->GetPlayerInfo();
		SendBufferRef sendBuffer = ClientPacketHandler::MakeSendBuffer(spawnPkt);

		Vector<Zone*> nearbyZones;
		_grid.GetNearbyZones(zoneIndex, nearbyZones);

		for (Zone* zone : nearbyZones)
		{
			for (const PlayerRef& other : zone->players)
			{
				if (other != player)
					other->GetSession()->Send(sendBuffer);
			}
		}
	}

	// 2) 나에게 주변 정보 스폰
	{
		Vector<Zone*> nearbyZones;
		_grid.GetNearbyZones(zoneIndex, nearbyZones);

		Protocol::S_SPAWN spawnPkt;

		for (Zone* zone : nearbyZones)
		{
			for (const PlayerRef& other : zone->players)
			{
				if (other != player)
				{
					Protocol::PlayerInfo* pInfo = spawnPkt.add_players();
					*pInfo = *other->GetPlayerInfo();
				}
			}

			for (const MonsterRef& monster : zone->monsters)
			{
				Protocol::MonsterInfo* mInfo = spawnPkt.add_monsters();
				*mInfo = *monster->GetMonsterInfo();
			}
		}

		if (spawnPkt.players_size() > 0 || spawnPkt.monsters_size() > 0)
		{
			SendBufferRef sendBuffer = ClientPacketHandler::MakeSendBuffer(spawnPkt);
			session->Send(sendBuffer);
		}
	}
}

// [로그인 입장]
void GameRoom::Enter(PlayerSessionRef session)
{
	if (EnterRegister(session) == false)
		return;

	PlayerRef player = session->GetPlayer();
	if (player == nullptr) return;

	// 1) 응답 먼저
	Protocol::S_ENTER_GAME enterPkt;
	enterPkt.set_success(true);
	enterPkt.mutable_myplayer()->CopyFrom(*player->GetPlayerInfo());
	// NOTE: 네 proto에 mapid가 실제로 있으면 유지, 없으면 이 줄 삭제
	// enterPkt.set_mapid(_mapId);

	session->Send(ClientPacketHandler::MakeSendBuffer(enterPkt));

	// 2) 스폰 전송은 그 다음
	SendEnterSpawns(session);

	printf("✅ [Enter-Login] Player %llu\n", player->GetPlayerId());
}

// [맵 이동 입장]
void GameRoom::EnterMapChange(PlayerSessionRef session)
{
	if (EnterRegister(session) == false)
		return;

	PlayerRef player = session->GetPlayer();
	if (player == nullptr) return;

	// 1) END 응답 먼저
	Protocol::S_MAP_CHANGE_END endPkt;
	endPkt.set_token(session->GetMapChangeToken());
	endPkt.set_mapid(_mapId);
	endPkt.mutable_pos()->CopyFrom(*player->GetPosInfo()); // proto: PositionInfo pos = 3

	session->Send(ClientPacketHandler::MakeSendBuffer(endPkt));

	// 2) 입력락 해제
	session->EndMapChange();

	// 3) 스폰은 그 다음
	SendEnterSpawns(session);

	printf("✅ [MapChange-END] Player %llu -> Map %d (Token=%llu)\n",
		player->GetPlayerId(), _mapId, endPkt.token());
}



void GameRoom::Leave(PlayerSessionRef session)
{
	PlayerRef player = session->GetPlayer();
	if (player == nullptr) return;

	uint64 playerId = player->GetPlayerId();
	if (_players.find(playerId) == _players.end()) return;

	int32 zoneIndex = player->GetZoneIndex();

	// 1. Zone에서 제거 (AOI)
	int32 totalZones = _grid.GetGridSizeX() * _grid.GetGridSizeY();
	if (zoneIndex >= 0 && zoneIndex < totalZones)
	{
		Zone& zone = _grid.GetZone(zoneIndex);
		zone.players.erase(player);
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

	printf("[GameRoom::HandleMove] Player %llu Move -> (%.1f,%.1f,%.1f)\n",
		playerId,
		pkt.posinfo().x(), pkt.posinfo().y(), pkt.posinfo().z());


	// 1. [Validation] 맵 충돌 체크
	if (_map->CanGo(pkt.posinfo()) == false)
		return;

	// 2. [Zone Check] AOI 그리드 기준
	int32 oldZoneIndex = player->GetZoneIndex();
	int32 newZoneIndex = _grid.GetZoneIndex(pkt.posinfo());

	// 3. [Update] 위치 정보 갱신
	player->SetPosInfo(pkt.posinfo());

	// [Case A] 같은 Zone 내 이동
	if (oldZoneIndex == newZoneIndex)
	{
		Protocol::S_MOVE movePkt;
		movePkt.set_objectid(playerId);
		*movePkt.mutable_posinfo() = pkt.posinfo();
		SendBufferRef sendBuffer = ClientPacketHandler::MakeSendBuffer(movePkt);


		printf("📢 [HandleMove] Broadcasting to Zone[%d], except Player %llu\n",
			newZoneIndex, playerId);  // ← 추가


		BroadcastToZone(sendBuffer, newZoneIndex, playerId);

		printf("✅ [HandleMove] Broadcast complete\n");  // ← 추가
	}
	// [Case B] Zone 변경 발생
	else
	{
		Vector<int32> oldZones;
		_grid.GetNearbyZoneIndices(oldZoneIndex, oldZones);
		std::sort(oldZones.begin(), oldZones.end());

		Vector<int32> newZones;
		_grid.GetNearbyZoneIndices(newZoneIndex, newZones);
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
				Zone& zone = _grid.GetZone(zoneIdx);

				// 해당 존에 있는 플레이어 처리
				for (auto& p : zone.players)
				{
					if (p->GetPlayerId() != playerId)
					{
						p->GetSession()->Send(sendBuffer);              // 걔네한테 내 정보 삭제
						despawnToMePkt.add_objectids(p->GetPlayerId()); // 내 목록에서 걔네 삭제
					}
				}
				// 몬스터 처리
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
				Zone& zone = _grid.GetZone(zoneIdx);

				// 플레이어 처리
				for (auto& p : zone.players)
				{
					if (p->GetPlayerId() != playerId)
					{
						p->GetSession()->Send(mySpawnBuffer); // 걔네에게 나를 보냄
						auto* otherInfo = othersSpawnPkt.add_players();
						*otherInfo = *p->GetPlayerInfo();
					}
				}
				// 몬스터 처리
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
				Zone& zone = _grid.GetZone(zoneIdx);
				for (auto& p : zone.players)
				{
					if (p->GetPlayerId() != playerId)
						p->GetSession()->Send(sendBuffer);
				}
			}
		}

		// 서버 내 Zone 이동 반영
		{
			Zone& oldZone = _grid.GetZone(oldZoneIndex);
			Zone& newZone = _grid.GetZone(newZoneIndex);

			oldZone.players.erase(player);
			newZone.players.insert(player);
			player->SetZoneIndex(newZoneIndex);
		}
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
	{
		Protocol::S_DESPAWN despawnPkt;
		despawnPkt.add_objectids(objectId);
		SendBufferRef sendBuffer = ClientPacketHandler::MakeSendBuffer(despawnPkt);
		BroadcastToZone(sendBuffer, zoneIndex);
	}
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

void GameRoom::HandleSkill(std::shared_ptr<Creature> attacker, int32 skillId)
{
	if (attacker == nullptr)
		return;

	// 방 검증
	if (attacker->GetRoom().get() != this)
		return;

	if (_battle == nullptr)
		return;

	// 1. BattleSystem에 전투 판정 위임
	SkillResult result;
	if (_battle->ResolveSkill(attacker, skillId, result) == false)
		return;

	// 2. 스킬 모션 브로드캐스트
	{
		Protocol::S_SKILL skillPkt;
		skillPkt.set_objectid(attacker->GetObjectId());
		skillPkt.set_skillid(skillId);

		SendBufferRef skillBuffer = ClientPacketHandler::MakeSendBuffer(skillPkt);
		BroadcastToZone(skillBuffer, result.zoneIndex);
	}

	// 3. 피격 결과 브로드캐스트 (HP 변경)
	for (const HitInfo& hit : result.hits)
	{
		auto victim = hit.target;
		if (victim == nullptr) continue;

		Protocol::S_CHANGE_HP changePkt;
		changePkt.set_objectid(victim->GetObjectId());
		changePkt.set_attackerid(attacker->GetObjectId());
		changePkt.set_currenthp(victim->GetStatInfo()->hp()); // OnDamaged 후 HP
		changePkt.set_damage(hit.damage);

		SendBufferRef sendBuffer = ClientPacketHandler::MakeSendBuffer(changePkt);
		BroadcastToZone(sendBuffer, result.zoneIndex);
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


void GameRoom::BroadcastToZone(SendBufferRef sendBuffer, int32 zoneIndex, uint64 exceptId)
{
	Vector<Zone*> nearbyZones;
	_grid.GetNearbyZones(zoneIndex, nearbyZones);   // [CHANGED]

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
