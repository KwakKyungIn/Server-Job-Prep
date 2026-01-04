#include "pch.h"
#include "GameRoom.h"
#include "GameMap.h"
#include "Monster.h"

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

	// [New] DataManager에서 MapConfig 가져오기 (Role B의 핵심)
	const MapConfig* config = DataManager::Instance()->GetMapConfig(mapId);

	_map = MakeShared<GameMap>();

	if (config)
	{
		// 1. Config가 있으면 그걸로 초기화 (권장 경로)
		// NavMesh 경로, AOI 셀 크기 등이 모두 포함됨
		_map->Init(config);

		// Grid도 Config의 aoiCellSize에 맞춰 초기화
		_grid.Init(0, 0, config->sizeX, config->sizeY, config->aoiCellSize);

		_interestRadius = config->interestRadius;   // A의 AOI 필터 반경이 맵별로 달라짐

		printf("✅ [GameRoom] Init with Config - MapId: %d, Size: (%d, %d), Cell: %d, Nav: %s\n",
			mapId, config->sizeX, config->sizeY, config->aoiCellSize,
			config->navMeshPath.empty() ? "None" : "Load");
	}
	else
	{
		// 2. Fallback: Config가 없을 경우 인자값으로 임시 처리
		// (GameMap::Init이 Config 포인터를 받으므로 임시 객체 생성)
		MapConfig tempConfig;
		tempConfig.mapId = mapId;
		tempConfig.sizeX = sizeX;
		tempConfig.sizeY = sizeY;
		tempConfig.aoiCellSize = zoneSize; // 인자로 받은 zoneSize 사용
		tempConfig.zoneSize = zoneSize;

		_map->Init(&tempConfig);
		_grid.Init(0, 0, sizeX, sizeY, zoneSize);

		printf("⚠️ [GameRoom] Init Fallback (No Config) - MapId: %d, Size: (%d, %d), Cell: %d\n",
			mapId, sizeX, sizeY, zoneSize);
	}


	// [Test Spawn] 테스트용 몬스터 1마리 소환
	MonsterRef slime = ObjectPool<Monster>::MakeShared();
	slime->Init(1); // 템플릿 ID 1번 (슬라임 킹)

	// 위치 설정: Config에 스폰 좌표가 있다면 우대, 없으면 기존 하드코딩
	if (config)
	{
		slime->GetPosInfo()->set_x(config->spawnX);
		slime->GetPosInfo()->set_y(config->spawnY);
		slime->GetPosInfo()->set_z(config->spawnZ);
	}
	else
	{
		slime->GetPosInfo()->set_x(52.0f);
		slime->GetPosInfo()->set_y(0.0f);
		slime->GetPosInfo()->set_z(52.0f);
	}
	slime->GetPosInfo()->set_yaw(0.0f);

	// 방에 입장 (이때 EnterMonster가 호출되면서 Zone에 등록됨)
	EnterMonster(slime);

	printf("👾 [Test] Slime_King Spawned at (%.1f, %.1f, %.1f)\n",
		slime->GetPosInfo()->x(), slime->GetPosInfo()->y(), slime->GetPosInfo()->z());

	// 디버그용: SpatialGrid 기준으로 확인
	int32 debugZoneIndex = _grid.GetZoneIndex(*slime->GetPosInfo());
	Zone& debugZone = _grid.GetZone(debugZoneIndex);

	printf("🔍 [DEBUG] Monster Check: Slime ID %llu is in Zone [%d]. Players in Zone: %zu, Monsters in Zone: %zu\n",
		slime->GetObjectId(),
		debugZoneIndex,
		debugZone.players.size(),
		debugZone.monsters.size());
}
bool GameRoom::ShouldPurge(uint64 nowMs) const
{
	if (IsInstanceRoom() == false)
		return false;

	if (IsClosing() == false)
		return false;

	if (GetPlayerCountApprox() != 0)
		return false;

	const uint64 emptySince = _emptySinceMs.load(std::memory_order_acquire);
	if (emptySince == 0)
		return false;

	constexpr uint64 GRACE_MS = 10'000; // 10초 지연 purge(안정성)
	return (nowMs - emptySince) >= GRACE_MS;
}
