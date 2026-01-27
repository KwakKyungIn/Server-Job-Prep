#include "pch.h"
#include "SpatialGrid.h"
//#include "GameMap.h"          
#include "Protocol.pb.h"      

// 그리드 초기화
// 맵 전체를 작은 사각형(Zone)으로 잘게 쪼개서 관리함
// 이렇게 안 하면 유저 찾을 때 O(N)으로 전체 검색해야 해서 서버 터짐
void SpatialGrid::Init(int32 minX, int32 minY, int32 maxX, int32 maxY, int32 cellSize)
{
    _minX = minX;
    _minY = minY;
    _maxX = maxX;
    _maxY = maxY;
    _cellSize = cellSize;

    // 맵 전체 크기 계산
    int32 sizeX = _maxX - _minX;
    int32 sizeY = _maxY - _minY;

    // 격자 개수 계산 (올림 처리)
    _gridSizeX = (sizeX + _cellSize - 1) / _cellSize;
    _gridSizeY = (sizeY + _cellSize - 1) / _cellSize;

    // 2차원 배열 대신 1차원 배열로 평탄화해서 캐시 적중률 높임
    _zones.clear();
    _zones.resize(_gridSizeX * _gridSizeY);
}

// 특정 좌표가 몇 번째 Zone에 속하는지 계산
int32 SpatialGrid::GetZoneIndex(const Protocol::PositionInfo& posInfo) const
{
    int32 x = static_cast<int32>(posInfo.x());
    int32 y = static_cast<int32>(posInfo.z()); // Unity는 Y가 높이지만, 서버 2D 그리드에선 Z를 Y축으로 씀

    // 유저가 맵 밖으로 나가거나 버그로 이상한 좌표에 갔을 때 
    // 인덱스 에러 안 나게 맵 범위 안으로 강제 고정(Clamp)
    if (x < _minX) x = _minX;
    if (x >= _maxX) x = _maxX - 1;
    if (y < _minY) y = _minY;
    if (y >= _maxY) y = _maxY - 1;

    // 상대 좌표로 변환 후 셀 크기로 나눔
    int32 zoneX = (x - _minX) / _cellSize;
    int32 zoneY = (y - _minY) / _cellSize;

    // 2D 인덱스 -> 1D 인덱스 변환 공식
    return zoneY * _gridSizeX + zoneX;
}

// 내 주변 Zone들의 인덱스 목록을 구함 (9-Grid 알고리즘)
// 나를 포함해서 상하좌우 대각선까지 총 9개 구역을 검사
void SpatialGrid::GetNearbyZoneIndices(int32 zoneIndex, Vector<int32>& outIndices) const
{
    outIndices.clear();
    if (zoneIndex < 0 || zoneIndex >= static_cast<int32>(_zones.size()))
        return;

    int32 x = zoneIndex % _gridSizeX;
    int32 y = zoneIndex / _gridSizeX;

    // -1, 0, 1 루프 돌면서 3x3 영역 체크
    for (int32 dy = -1; dy <= 1; dy++)
    {
        for (int32 dx = -1; dx <= 1; dx++)
        {
            int32 nx = x + dx;
            int32 ny = y + dy;

            // 맵 경계 벗어나는지 체크
            if (nx >= 0 && nx < _gridSizeX && ny >= 0 && ny < _gridSizeY)
            {
                int32 index = ny * _gridSizeX + nx;
                outIndices.push_back(index);
            }
        }
    }
}

// 위 함수랑 똑같은데 인덱스 대신 Zone 포인터를 바로 줌
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

// 시야 범위를 조절할 수 있는 버전 (radiusCells가 2면 5x5 영역)
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
            // 유효 범위 체크
            if (nx < 0 || nx >= _gridSizeX || ny < 0 || ny >= _gridSizeY) continue;
            outZones.push_back(&_zones[ny * _gridSizeX + nx]);
        }
}

Zone& SpatialGrid::GetZone(int32 zoneIndex)
{
    // 범위 체크 실패하면 바로 크래시 내서 버그 잡기 (Assert)
    ASSERT_CRASH(zoneIndex >= 0 && zoneIndex < static_cast<int32>(_zones.size()));
    return _zones[zoneIndex];
}

const Zone& SpatialGrid::GetZone(int32 zoneIndex) const
{
    ASSERT_CRASH(zoneIndex >= 0 && zoneIndex < static_cast<int32>(_zones.size()));
    return _zones[zoneIndex];
}