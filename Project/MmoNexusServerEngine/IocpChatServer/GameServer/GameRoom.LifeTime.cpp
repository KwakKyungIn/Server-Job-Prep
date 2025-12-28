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

	printf("[GameRoom] Init MapId: %d, MapSize: (%d, %d), CellSize: %d, Grid: (%d, %d)\n",
		mapId,
		sizeX, sizeY,
		zoneSize,
		_grid.GetGridSizeX(), _grid.GetGridSizeY());


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
