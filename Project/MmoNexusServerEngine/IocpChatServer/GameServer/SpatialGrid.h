#pragma once
#include "Zone.h"

namespace Protocol
{
    class PositionInfo;
}

// 맵 하나당 존재하는 공간 분할 관리자 (AOI - Area Of Interest)
// 맵을 격자(Grid)로 나누고 유저 위치 기반으로 주변 오브젝트를 빠르게 찾을 때 사용함
class SpatialGrid
{
public:
    SpatialGrid() = default;
    ~SpatialGrid() = default;

    // 맵 데이터(GameMap)에서 읽어온 최소/최대 좌표와 셀 크기로 초기화
    void Init(int32 minX, int32 minY, int32 maxX, int32 maxY, int32 cellSize);

    // 좌표(Pos)를 주면 몇 번째 Zone인지 인덱스 반환
    int32 GetZoneIndex(const Protocol::PositionInfo& posInfo) const;

    // 특정 Zone 주변(3x3)의 인덱스 목록 반환
    void GetNearbyZoneIndices(int32 zoneIndex, Vector<int32>& outIndices) const;

    // 특정 Zone 주변의 실제 Zone 객체 포인터 반환
    void GetNearbyZones(int32 zoneIndex, Vector<Zone*>& outZones);

    // 시야 범위(Radius)를 지정해서 주변 Zone 가져오기 (확장형)
    void GetNearbyZones(int32 zoneIndex, int32 radiusCells, Vector<Zone*>& outZones);

    // 인덱스로 Zone에 직접 접근 (경계 체크 포함)
    Zone& GetZone(int32 zoneIndex);
    const Zone& GetZone(int32 zoneIndex) const;

    // 그리드 정보 게터
    int32 GetGridSizeX() const { return _gridSizeX; }
    int32 GetGridSizeY() const { return _gridSizeY; }
    int32 GetCellSize()  const { return _cellSize; }

private:
    int32 _minX = 0;
    int32 _minY = 0;
    int32 _maxX = 0;
    int32 _maxY = 0;
    int32 _cellSize = 1; // 격자 한 칸의 크기

    int32 _gridSizeX = 0;
    int32 _gridSizeY = 0;

    // 1차원 벡터로 관리하는 Zone 목록
    Vector<Zone> _zones;
};