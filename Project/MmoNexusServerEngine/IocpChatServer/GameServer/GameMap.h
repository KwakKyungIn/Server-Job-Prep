#pragma once
#include "Protocol.pb.h"

class GameMap
{
public:
	GameMap();
	~GameMap();

	void Init(int32 mapId, int32 sizeX, int32 sizeY);

	// [Collision Check]
	// 좌표 유효성 및 벽 충돌 체크
	bool CanGo(const Protocol::PositionInfo& posInfo);

	// [Getter]
	int32 GetMinX() const { return _minX; }
	int32 GetMaxX() const { return _maxX; }
	int32 GetMinY() const { return _minY; }
	int32 GetMaxY() const { return _maxY; }

	// [New] AOI 그리드 계산용 사이즈 반환
	int32 GetSizeX() const { return _maxX - _minX; }
	int32 GetSizeY() const { return _maxY - _minY; }

	int32 GetMapId() const { return _mapId; }

private:
	int32 _mapId = 0;
	int32 _minX = 0;
	int32 _maxX = 0;
	int32 _minY = 0;
	int32 _maxY = 0;

	// 2D 격자 충돌 배열 (0: 이동가능, 1: 벽)
	vector<vector<int32>> _collision;
};