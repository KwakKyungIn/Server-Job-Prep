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


// util: 네트워크로 나가는 ID는 "플레이어=playerId, 몬스터=objectId"로 통일
static uint64 NetId(const std::shared_ptr<Creature>& c)
{
	if (c == nullptr) return 0;

	if (c->GetObjectType() == Protocol::OBJECT_TYPE_PLAYER)
		return std::static_pointer_cast<Player>(c)->GetPlayerId();

	return c->GetObjectId(); // 몬스터/투사체 등
}

static PlayerSessionRef FindSessionByPlayerId(uint64 playerId)
{
	return GameSessionManager::GSessionManager->FindByPlayerId(playerId);
}

static void SendToPlayer(uint64 playerId, SendBufferRef sb)
{
	if (!sb) return;
	if (auto s = FindSessionByPlayerId(playerId))
		s->Send(sb);
}


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
	
	// [Test Spawn] 테스트용 몬스터 1마리 소환
	MonsterRef slime = ObjectPool<Monster>::MakeShared();
	slime->Init(1); // 템플릿 ID 1번 (슬라임 킹)

	slime->GetPosInfo()->set_x(52.0f);
	slime->GetPosInfo()->set_y(0.0f);
	slime->GetPosInfo()->set_z(52.0f);
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

bool GameRoom::EnterRegister(PlayerSessionRef session, PlayerRef player)
{

	if (player == nullptr) return false;

	// 이미 들어와 있으면 실패 (중복 Enter 방지)
	if (_players.find(player->GetPlayerId()) != _players.end())
		return false;

	// 1) 룸 소속 설정
	player->SetRoom(shared_from_this());

	auto room = shared_from_this();
	session->Post([room](PlayerSessionRef ps)
		{
			ps->SetCurrentRoom(room);
		});

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

void GameRoom::SendEnterSpawns(PlayerSessionRef session, PlayerRef player)
{
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
					SendToPlayer(other->GetPlayerId(), sendBuffer);
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
void GameRoom::Enter(PlayerSessionRef session, PlayerRef player)
{
	if (!session || !player)
		return;

	if (EnterRegister(session,player) == false)
		return;

	
	if (player == nullptr) return;

	// 1) 응답 먼저
	Protocol::S_ENTER_GAME enterPkt;
	enterPkt.set_success(true);
	enterPkt.mutable_myplayer()->CopyFrom(*player->GetPlayerInfo());
	// NOTE: 네 proto에 mapid가 실제로 있으면 유지, 없으면 이 줄 삭제
	// enterPkt.set_mapid(_mapId);

	session->Send(ClientPacketHandler::MakeSendBuffer(enterPkt));

	// 2) 스폰 전송은 그 다음
	SendEnterSpawns(session,player);

	printf("✅ [Enter-Login] Player %llu\n", player->GetPlayerId());
}

// [맵 이동 입장]
void GameRoom::EnterMapChange(PlayerSessionRef session, PlayerRef player)
{
	if (!session || !player)
		return;

	if (EnterRegister(session,player) == false)
		return;

	if (player == nullptr) return;

	// 1) END 응답 먼저
	Protocol::S_MAP_CHANGE_END endPkt;
	endPkt.set_token(session->GetMapChangeToken());
	endPkt.set_mapid(_mapId);
	endPkt.mutable_pos()->CopyFrom(*player->GetPosInfo()); // proto: PositionInfo pos = 3
	endPkt.set_instanceid(player->GetInstanceId());
	session->Send(ClientPacketHandler::MakeSendBuffer(endPkt));

	// 2) 입력락 해제
	session->EndMapChange();

	// 3) 스폰은 그 다음
	SendEnterSpawns(session,player);

	printf("✅ [MapChange-END] Player %llu -> Map %d (Token=%llu)\n",
		player->GetPlayerId(), _mapId, endPkt.token());
}



void GameRoom::Leave(PlayerSessionRef session, PlayerRef player)
{
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

	auto room = shared_from_this();
	session->Post([room](PlayerSessionRef ps)
		{
			ps->ClearCurrentRoom(room);
		});

	// 3. [Broadcast] 주변 유저들에게 "나 나갔음" 알림 (S_DESPAWN)
	{
		Protocol::S_DESPAWN despawnPkt;
		despawnPkt.add_objectids(playerId);
		SendBufferRef sendBuffer = ClientPacketHandler::MakeSendBuffer(despawnPkt);

		BroadcastToZone(sendBuffer, zoneIndex, 0);
	}

	printf("[ROOM] Player %llu Left Zone[%d].\n", playerId, zoneIndex);
}

void GameRoom::HandleMove(PlayerSessionRef session, PlayerRef player,Protocol::C_MOVE pkt)
{
	
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
						SendToPlayer(p->GetPlayerId(), sendBuffer);              // 걔네한테 내 정보 삭제
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
						SendToPlayer(p->GetPlayerId(), mySpawnBuffer); // 걔네에게 나를 보냄
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
						SendToPlayer(p->GetPlayerId(), sendBuffer);
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

//몬스터 죽이고, 경험치 및 템 루팅
static std::atomic<uint64> GItemUidGen{ 1000000 }; // 서버 발급 itemUid (DB가 identity면 나중에 정책 바꿔야 함)

static int32 FindEmptySlot(const std::vector<Protocol::ItemInfo>& items, int32 maxSlots)
{
	std::vector<bool> used(maxSlots, false);
	for (const auto& it : items)
	{
		if (it.slot() >= 0 && it.slot() < maxSlots)
			used[it.slot()] = true;
	}
	for (int32 i = 0; i < maxSlots; i++)
		if (used[i] == false)
			return i;
	return -1;
}

static void AddExpAndLevelUp(PlayerRef player, int64 addExp)
{
	Protocol::StatInfo* stat = player->GetStatInfo();
	if (stat == nullptr) return;

	stat->set_totalexp(stat->totalexp() + addExp);

	// 레벨업: "다음 레벨 템플릿의 totalExp"를 달성 조건으로 사용
	while (true)
	{
		int32 curLv = stat->level();
		const Protocol::StatTemplateInfo* nextTpl = DataManager::Instance()->GetStatTemplate(curLv + 1);
		if (nextTpl == nullptr)
			break;

		if (stat->totalexp() < nextTpl->totalexp())
			break;

		stat->set_level(curLv + 1);

		// 레벨 바뀌었으니 스탯 리프레시
		player->RefreshStats();

		// 레벨업하면 풀피로 (원하면 비율 유지로 바꿔도 됨)
		stat->set_hp(stat->maxhp());
	}
}

void GameRoom::HandleMonsterDead(std::shared_ptr<Creature> attacker, MonsterRef monster)
{
	if (monster == nullptr) return;
	if (monster->GetRoom().get() != this) return;

	const uint64 monsterId = monster->GetObjectId();
	if (_monsters.find(monsterId) == _monsters.end())
	{
		// 이미 처리됐으면 중복 방지
		return;
	}

	// 1) 킬러가 플레이어인지 확인
	PlayerRef killer = nullptr;
	if (attacker && attacker->GetObjectType() == Protocol::OBJECT_TYPE_PLAYER)
		killer = std::static_pointer_cast<Player>(attacker);

	// 2) 경험치 지급
	if (killer)
	{
		const int64 exp = 10; // TODO: 몬스터 템플릿에서 읽기
		AddExpAndLevelUp(killer, exp);

		Protocol::S_CHANGE_STAT st;
		st.mutable_statinfo()->CopyFrom(*killer->GetStatInfo());
		SendToPlayer(killer->GetPlayerId(), ClientPacketHandler::MakeSendBuffer(st));
	}

	// 3) 드랍(루팅) = “즉시 인벤 지급” 방식 (바닥 루프 만들기용)
	if (killer)
	{
		// TODO: 드랍 테이블 생기면 여기 교체
		const int32 dropTemplateId = 103; // 예: 포션 템플릿 ID (네 ITEM_TEMPLATE에 맞춰서 바꿔)

		const Protocol::ItemTemplateInfo* tpl = DataManager::Instance()->GetItemTemplate(dropTemplateId);
		if (tpl && static_cast<Protocol::ItemType>(tpl->itemtype()) != Protocol::ITEM_TYPE_NONE)
		{
			auto& items = killer->GetItems();
			const int32 maxSlots = 24; // 너 인벤 고정이면 그대로
			int32 emptySlot = FindEmptySlot(items, maxSlots);

			if (emptySlot >= 0)
			{
				Protocol::ItemInfo newItem;
				newItem.set_itemuid(GItemUidGen.fetch_add(1));
				newItem.set_templateid(dropTemplateId);
				newItem.set_count(1);
				newItem.set_slot(emptySlot);
				newItem.set_isequipped(false);

				items.push_back(newItem);

				Protocol::S_CHANGE_ITEM ch;
				ch.mutable_item()->CopyFrom(newItem);
				SendToPlayer(killer->GetPlayerId(), ClientPacketHandler::MakeSendBuffer(ch));


				// TODO: DB 저장(S2S INSERT ITEMS)
			}
			else
			{
				// 인벤 꽉 참: 지금은 그냥 드랍 폐기 or TODO: 월드 드랍 오브젝트
			}
		}
	}

	// 4) 마지막에 몬스터 제거(브로드캐스트 despawn 포함)
	LeaveMonster(monsterId);
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
		skillPkt.set_objectid(NetId(attacker));
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
		changePkt.set_objectid(NetId(victim));
		changePkt.set_attackerid(NetId(attacker));
		changePkt.set_currenthp(victim->GetStatInfo()->hp()); // OnDamaged 후 HP
		changePkt.set_damage(hit.damage);

		SendBufferRef sendBuffer = ClientPacketHandler::MakeSendBuffer(changePkt);
		BroadcastToZone(sendBuffer, result.zoneIndex);
	}
}

void GameRoom::HandleUseItem(PlayerSessionRef session, PlayerRef player, Protocol::C_USE_ITEM pkt)
{
	
	if (player == nullptr) return;

	const uint64 playerId = player->GetPlayerId();
	if (_players.find(playerId) == _players.end()) return;

	// 인벤에서 아이템 찾기
	auto& items = player->GetItems();
	auto it = std::find_if(items.begin(), items.end(),
		[&](const Protocol::ItemInfo& item) { return item.itemuid() == pkt.itemuid(); });

	if (it == items.end())
		return;

	// 템플릿 검증
	const Protocol::ItemTemplateInfo* tpl = DataManager::Instance()->GetItemTemplate(it->templateid());
	if (tpl == nullptr) return;

	const Protocol::ItemType itemType = static_cast<Protocol::ItemType>(tpl->itemtype());
	if (itemType != Protocol::ITEM_TYPE_CONSUMABLE)
		return;

	if (it->count() <= 0)
		return;

	// 힐량(지금은 hp_bonus로 처리)
	const int32 heal = tpl->hpbonus();
	if (heal <= 0)
		return;

	Protocol::StatInfo* stat = player->GetStatInfo();
	if (stat == nullptr) return;

	// 풀피면 소비 안 하게(추천)
	if (stat->hp() >= stat->maxhp())
		return;

	// 1) HP 적용
	const int32 newHp = min(stat->hp() + heal, stat->maxhp());
	stat->set_hp(newHp);

	// 2) 아이템 카운트 감소
	it->set_count(it->count() - 1);

	// 3) 아이템 패킷(변경/삭제)
	if (it->count() <= 0)
	{
		const uint64 removedUid = it->itemuid();
		items.erase(it);

		Protocol::S_REMOVE_ITEM rm;
		rm.set_itemuid(removedUid);
		session->Send(ClientPacketHandler::MakeSendBuffer(rm));
	}
	else
	{
		Protocol::S_CHANGE_ITEM ch;
		ch.mutable_item()->CopyFrom(*it);
		session->Send(ClientPacketHandler::MakeSendBuffer(ch));
	}

	// 4) 스탯 패킷
	{
		Protocol::S_CHANGE_STAT st;
		st.mutable_statinfo()->CopyFrom(*stat);
		session->Send(ClientPacketHandler::MakeSendBuffer(st));
	}

	// TODO: DB 반영(S2S 아이템 count 업데이트, hp 저장 정책)
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
			SendToPlayer(p->GetPlayerId(), sendBuffer);
		}
	}
}

void GameRoom::Broadcast(SendBufferRef sendBuffer, uint64 exceptId)
{
	for (auto& item : _players)
	{
		PlayerRef p = item.second;
		if (!p) continue;
		if (p->GetPlayerId() == exceptId) continue;
		SendToPlayer(p->GetPlayerId(), sendBuffer);
	}
}

void GameRoom::BroadcastChat(const Protocol::S_CHAT_NTF& ntf)
{
	// GameRoom은 JobQueue로 직렬 실행되는 전제라 별도 락 없이 간다.
	for (auto it = _players.begin(); it != _players.end(); ++it)
	{
		PlayerRef player = it->second;
		if (!player) continue;

		Protocol::S_CHAT_NTF pkt;
		pkt.CopyFrom(ntf);

		auto sb = ClientPacketHandler::MakeSendBuffer(pkt);
		SendToPlayer(player->GetPlayerId(), sb);
	}
}