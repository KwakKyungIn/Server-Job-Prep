#include "pch.h"
#include "GameMap.h"

GameMap::GameMap()
{
}

GameMap::~GameMap()
{
}

void GameMap::Init(int32 mapId, int32 sizeX, int32 sizeY)
{
	_mapId = mapId;
	_minX = 0;
	_maxX = sizeX;
	_minY = 0;
	_maxY = sizeY;

	// [Init] 2D 배열 할당 (0으로 초기화)
	_collision = vector<vector<int32>>(sizeY, vector<int32>(sizeX, 0));

	// [Test] 맵 테두리에 벽(1) 세우기
	for (int y = 0; y < sizeY; y++)
	{
		for (int x = 0; x < sizeX; x++)
		{
			if (x == 0 || x == sizeX - 1 || y == 0 || y == sizeY - 1)
				_collision[y][x] = 1;
		}
	}
}

bool GameMap::CanGo(const Protocol::PositionInfo& posInfo)
{
	// Unity 좌표(Float) -> 서버 그리드(Int) 변환
	// Unity(x, y, z) -> Server(x, z) 매핑 (y는 높이이므로 무시)
	int32 x = static_cast<int32>(posInfo.x());
	int32 y = static_cast<int32>(posInfo.z());

	// 1. 맵 범위 체크
	if (x < _minX || x >= _maxX) return false;
	if (y < _minY || y >= _maxY) return false;

	// 2. 충돌체(벽) 체크
	// 0이면 Pass, 1이면 Block
	// 배열 인덱스는 [y][x] 순서임에 주의
	if (_collision[y][x] == 1) return false;

	return true;
}