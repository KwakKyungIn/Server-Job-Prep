#include "pch.h"
#include "GameRoom.h"
#include "GameMap.h"
#include "Monster.h"
#include "DataManager.h"

GameRoom::GameRoom()
{
	// 작업 큐랑 전투 시스템은 방 생성할 때 같이 만들어둠
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

	// 기획 데이터에서 맵 설정 정보를 긁어온다.
	// NavMesh 경로랑 구역 크기 같은 중요한 정보가 여기 다 들어있음
	const MapConfig* config = DataManager::Instance()->GetMapConfig(mapId);

	_map = MakeShared<GameMap>();

	if (config)
	{
		// 설정 파일이 제대로 있으면 그걸로 초기화 진행
		// NavMesh 로딩하고 그리드 셀 크기도 설정값에 맞춤
		_map->Init(config);

		// 그리드 시스템 초기화. 여기서 aoiCellSize가 시야 처리 성능에 영향을 줌
		_grid.Init(0, 0, config->sizeX, config->sizeY, config->aoiCellSize);

		// AOI 반경 설정. 맵마다 시야 거리가 다를 수 있으니까 config에서 가져옴
		_interestRadius = config->interestRadius;

		printf(" [GameRoom] Init with Config - MapId: %d, Size: (%d, %d), Cell: %d, Nav: %s\n",
			mapId, config->sizeX, config->sizeY, config->aoiCellSize,
			config->navMeshPath.empty() ? "None" : "Load");
	}
	else
	{
		// 설정 파일이 없을 경우를 대비한 하드코딩 (테스트 용도)
		// 임시 설정 객체 만들어서 기본값으로 초기화함
		MapConfig tempConfig;
		tempConfig.mapId = mapId;
		tempConfig.sizeX = sizeX;
		tempConfig.sizeY = sizeY;
		tempConfig.aoiCellSize = zoneSize;
		tempConfig.zoneSize = zoneSize;

		_map->Init(&tempConfig);
		_grid.Init(0, 0, sizeX, sizeY, zoneSize);

		printf(" [GameRoom] Init Fallback (No Config) - MapId: %d, Size: (%d, %d), Cell: %d\n",
			mapId, sizeX, sizeY, zoneSize);
	}

	// JSON 스폰 테이블 기반으로 몬스터 초기 스폰
	InitSpawnPoints_ActorOnly();
}

void GameRoom::InitSpawnPoints_ActorOnly()
{
	_spawnPoints.clear();

	DataManager* dm = DataManager::Instance();
	const Vector<SpawnEntry>* entries = (dm ? dm->GetSpawnEntries(_mapId) : nullptr);
	if (!entries || entries->empty())
	{
		std::cout << " [GameRoom] SpawnTables empty for MapId=" << _mapId << std::endl;
		return;
	}

	for (const auto& e : *entries)
	{
		SpawnPointRuntime sp;
		sp.spawnId = e.spawnId;
		sp.monsterId = e.monsterId;
		sp.pos.set_x(e.x);
		sp.pos.set_y(e.y);
		sp.pos.set_z(e.z);
		sp.pos.set_yaw(0.0f);
		sp.maxAlive = e.maxAlive;
		sp.respawnMs = static_cast<uint64>(e.respawnSec) * 1000;

		_spawnPoints[sp.spawnId] = sp;
	}

	const uint64 now = ::GetTickCount64();
	for (auto& kv : _spawnPoints)
	{
		auto& sp = kv.second;
		for (int32 i = 0; i < sp.maxAlive; ++i)
			SpawnFromPoint_ActorOnly(sp, now);
	}

	std::cout << " [GameRoom] SpawnPoints initialized: " << _spawnPoints.size() << std::endl;
}

void GameRoom::UpdateSpawns_ActorOnly(uint64 nowMs)
{
	for (auto& kv : _spawnPoints)
	{
		auto& sp = kv.second;
		if (sp.aliveCount >= sp.maxAlive)
			continue;

		if (sp.nextSpawnMs == 0)
			sp.nextSpawnMs = nowMs;

		if (nowMs < sp.nextSpawnMs)
			continue;

		const int32 missing = sp.maxAlive - sp.aliveCount;
		const int32 spawnCount = (sp.respawnMs == 0) ? missing : 1;

		for (int32 i = 0; i < spawnCount; ++i)
		{
			if (sp.aliveCount >= sp.maxAlive)
				break;
			SpawnFromPoint_ActorOnly(sp, nowMs);
			if (sp.respawnMs > 0)
				break;
		}
	}
}

void GameRoom::SpawnFromPoint_ActorOnly(SpawnPointRuntime& sp, uint64 nowMs)
{
	MonsterRef monster = ObjectPool<Monster>::MakeShared();
	monster->Init(sp.monsterId, sp.spawnId);

	auto* pos = monster->GetPosInfo();
	pos->set_x(sp.pos.x());
	pos->set_y(sp.pos.y());
	pos->set_z(sp.pos.z());
	pos->set_yaw(sp.pos.yaw());

	EnterMonster(monster);

	sp.aliveCount++;
	if (sp.aliveCount >= sp.maxAlive)
		sp.nextSpawnMs = 0;
	else
		sp.nextSpawnMs = nowMs + sp.respawnMs;
}

void GameRoom::OnMonsterDespawned_ActorOnly(MonsterRef monster)
{
	if (!monster) return;

	const int32 spawnId = monster->GetSpawnId();
	if (spawnId == 0) return;

	auto it = _spawnPoints.find(spawnId);
	if (it == _spawnPoints.end())
		return;

	auto& sp = it->second;
	if (sp.aliveCount > 0)
		sp.aliveCount--;

	if (sp.aliveCount < sp.maxAlive)
	{
		const uint64 nowMs = ::GetTickCount64();
		const uint64 next = nowMs + sp.respawnMs;

		if (sp.respawnMs == 0)
			sp.nextSpawnMs = nowMs;
		else if (sp.nextSpawnMs == 0 || sp.nextSpawnMs > next)
			sp.nextSpawnMs = next;
	}
}

bool GameRoom::ShouldPurge(uint64 nowMs) const
{
	// 인스턴스 던전 아니면 방 폭파 안 함 (마을 같은 곳)
	if (IsInstanceRoom() == false)
		return false;

	// 이미 닫히고 있는 중이면 패스
	if (IsClosing() == false)
		return false;

	// 아직 안에 사람 있으면 폭파하면 안 됨
	if (GetPlayerCountApprox() != 0)
		return false;

	// 방이 언제부터 비었는지 확인
	const uint64 emptySince = _emptySinceMs.load(std::memory_order_acquire);
	if (emptySince == 0)
		return false;

	// 사람이 다 나가고 나서 바로 없애지 않고 10초 정도 유예 시간 줌
	// 혹시 재입장하거나 네트워크 렉 때문에 튕긴 유저 배려
	constexpr uint64 GRACE_MS = 10'000;
	return (nowMs - emptySince) >= GRACE_MS;
}
