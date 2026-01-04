#include "pch.h"
#include "SpatialGrid.h"
//#include "GameMap.h"          // min/max 정의된 곳 (필요 없으면 빼도 됨)
#include "Protocol.pb.h"      // PositionInfo 실제 정의

void SpatialGrid::Init(int32 minX, int32 minY, int32 maxX, int32 maxY, int32 cellSize)
{
    _minX = minX;
    _minY = minY;
    _maxX = maxX;
    _maxY = maxY;
    _cellSize = cellSize;

    int32 sizeX = _maxX - _minX;
    int32 sizeY = _maxY - _minY;

    _gridSizeX = (sizeX + _cellSize - 1) / _cellSize;
    _gridSizeY = (sizeY + _cellSize - 1) / _cellSize;

    _zones.clear();
    _zones.resize(_gridSizeX * _gridSizeY);
}

int32 SpatialGrid::GetZoneIndex(const Protocol::PositionInfo& posInfo) const
{
    int32 x = static_cast<int32>(posInfo.x());
    int32 y = static_cast<int32>(posInfo.z()); // Unity (x, z) → 서버 (x, y)

    // 맵 범위 Clamp
    if (x < _minX) x = _minX;
    if (x >= _maxX) x = _maxX - 1;
    if (y < _minY) y = _minY;
    if (y >= _maxY) y = _maxY - 1;

    int32 zoneX = (x - _minX) / _cellSize;
    int32 zoneY = (y - _minY) / _cellSize;

    return zoneY * _gridSizeX + zoneX;
}

void SpatialGrid::GetNearbyZoneIndices(int32 zoneIndex, Vector<int32>& outIndices) const
{
    outIndices.clear();
    if (zoneIndex < 0 || zoneIndex >= static_cast<int32>(_zones.size()))
        return;

    int32 x = zoneIndex % _gridSizeX;
    int32 y = zoneIndex / _gridSizeX;

    for (int32 dy = -1; dy <= 1; dy++)
    {
        for (int32 dx = -1; dx <= 1; dx++)
        {
            int32 nx = x + dx;
            int32 ny = y + dy;
            if (nx >= 0 && nx < _gridSizeX && ny >= 0 && ny < _gridSizeY)
            {
                int32 index = ny * _gridSizeX + nx;
                outIndices.push_back(index);
            }
        }
    }
}

void SpatialGrid::GetNearbyZones(int32 zoneIndex, Vector<Zone*>& outZones)
{
    outZones.clear();
    if (zoneIndex < 0 || zoneIndex >= static_cast<int32>(_zones.size()))
        return;

    int32 x = zoneIndex % _gridSizeX;
    int32 y = zoneIndex / _gridSizeX;

    for (int32 dy = -1; dy <= 1; dy++)
    {
        for (int32 dx = -1; dx <= 1; dx++)
        {
            int32 nx = x + dx;
            int32 ny = y + dy;
            if (nx >= 0 && nx < _gridSizeX && ny >= 0 && ny < _gridSizeY)
            {
                int32 index = ny * _gridSizeX + nx;
                outZones.push_back(&_zones[index]);
            }
        }
    }
}

void SpatialGrid::GetNearbyZones(int32 zoneIndex, int32 radiusCells, Vector<Zone*>& outZones)
{
    outZones.clear();
    if (zoneIndex < 0 || zoneIndex >= (int32)_zones.size()) return;

    int32 x = zoneIndex % _gridSizeX;
    int32 y = zoneIndex / _gridSizeX;

    for (int32 dy = -radiusCells; dy <= radiusCells; dy++)
        for (int32 dx = -radiusCells; dx <= radiusCells; dx++)
        {
            int32 nx = x + dx, ny = y + dy;
            if (nx < 0 || nx >= _gridSizeX || ny < 0 || ny >= _gridSizeY) continue;
            outZones.push_back(&_zones[ny * _gridSizeX + nx]);
        }
}

Zone& SpatialGrid::GetZone(int32 zoneIndex)
{
    ASSERT_CRASH(zoneIndex >= 0 && zoneIndex < static_cast<int32>(_zones.size()));
    return _zones[zoneIndex];
}

const Zone& SpatialGrid::GetZone(int32 zoneIndex) const
{
    ASSERT_CRASH(zoneIndex >= 0 && zoneIndex < static_cast<int32>(_zones.size()));
    return _zones[zoneIndex];
}
