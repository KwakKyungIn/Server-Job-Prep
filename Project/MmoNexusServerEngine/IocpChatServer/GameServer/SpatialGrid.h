#pragma once
#include "Zone.h"

namespace Protocol
{
    class PositionInfo;
}

// 맵 하나에 대한 AOI(9-Grid) 관리 전담
class SpatialGrid
{
public:
    SpatialGrid() = default;
    ~SpatialGrid() = default;

    // min/max는 GameMap에서 받아온 값 그대로 넣어줄 거다.
    void Init(int32 minX, int32 minY, int32 maxX, int32 maxY, int32 cellSize);

    int32 GetZoneIndex(const Protocol::PositionInfo& posInfo) const;

    void GetNearbyZoneIndices(int32 zoneIndex, Vector<int32>& outIndices) const;

    void GetNearbyZones(int32 zoneIndex, Vector<Zone*>& outZones);

    void GetNearbyZones(int32 zoneIndex, int32 radiusCells, Vector<Zone*>& outZones);

    // Zone 직접 접근이 필요할 때
    Zone& GetZone(int32 zoneIndex);
    const Zone& GetZone(int32 zoneIndex) const;

    int32 GetGridSizeX() const { return _gridSizeX; }
    int32 GetGridSizeY() const { return _gridSizeY; }
    int32 GetCellSize()  const { return _cellSize; }

private:
    int32 _minX = 0;
    int32 _minY = 0;
    int32 _maxX = 0;
    int32 _maxY = 0;
    int32 _cellSize = 1;

    int32 _gridSizeX = 0;
    int32 _gridSizeY = 0;

    Vector<Zone> _zones;
};
