#pragma once
#include "Protocol.pb.h" // PositionInfo 사용을 위해 필요

class GameMap
{
public:
	GameMap();
	~GameMap();

	void Init(int32 mapId, int32 sizeX, int32 sizeY);

	// [Collision Check]
	// 좌표 유효성 및 벽 충돌 체크
	// Read-Only 함수이므로 const 멤버 함수로 만드는 것이 좋음 (나중을 위해)
	bool CanGo(const Protocol::PositionInfo& posInfo);

	int32 GetMinX() { return _minX; }
	int32 GetMaxX() { return _maxX; }
	int32 GetMinY() { return _minY; }
	int32 GetMaxY() { return _maxY; }

	int32 GetMapId() { return _mapId; }

private:
	int32 _mapId = 0;
	int32 _minX = 0;
	int32 _maxX = 0;
	int32 _minY = 0;
	int32 _maxY = 0;

	// 2D 격자 충돌 배열 (0: 이동가능, 1: 벽)
	// vector<vector>는 메모리가 파편화되지만, 구현이 쉬우므로 프로토타입 단계에선 OK.
	// 최적화 단계에선 1차원 vector (y * width + x)로 바꾸는 게 정석이다.
	vector<vector<int32>> _collision;
};