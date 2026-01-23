#include "pch.h"
#include "GameRoom.h"
#include "GameMap.h"
#include "Monster.h"

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


	// 서버 켜지면 테스트용으로 슬라임 킹 한 마리 소환해봄
	MonsterRef slime = ObjectPool<Monster>::MakeShared();
	slime->Init(1); // 템플릿 ID 1번

	// 스폰 위치 잡기. Config에 지정된 위치가 있으면 거기다 찍고 아니면 그냥 (52, 0, 52)에 박음
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

	// 몬스터 방 입장 처리. 여기서 Grid Zone에 등록되고 주변 플레이어한테 패킷 날아감
	EnterMonster(slime);

	printf("[Test] Slime_King Spawned at (%.1f, %.1f, %.1f)\n",
		slime->GetPosInfo()->x(), slime->GetPosInfo()->y(), slime->GetPosInfo()->z());

	// 디버깅용으로 Grid에 잘 들어갔나 확인
	int32 debugZoneIndex = _grid.GetZoneIndex(*slime->GetPosInfo());
	Zone& debugZone = _grid.GetZone(debugZoneIndex);

	printf(" [DEBUG] Monster Check: Slime ID %llu is in Zone [%d]. Players in Zone: %zu, Monsters in Zone: %zu\n",
		slime->GetObjectId(),
		debugZoneIndex,
		debugZone.players.size(),
		debugZone.monsters.size());
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