#pragma once
#include "Protocol.pb.h"
#include "DataManager.h" // MapConfig
#include "ObjectUtils.h"

class NavSystem; // Forward Declaration (헤더 의존성 최소화)

class GameMap
{
public:
    GameMap();
    ~GameMap();

    // MapConfig 전체를 받도록 변경
    bool Init(const MapConfig* config);

    // [Collision Check] -> [Move Validation]
    // 단순히 bool이 아니라, 보정된 위치를 반환할 수 있어야 함
    // (지금은 인터페이스 유지를 위해 기존 CanGo를 수정하되, 내부적으로 NavSystem 호출)
    bool CanGo(const Protocol::PositionInfo& posInfo);

    // [New] Role B: 이동 검증 & 보정 API
    bool ValidateMove(const Protocol::PositionInfo& current, const Protocol::PositionInfo& next, Protocol::PositionInfo& outFixed);

    // [New] Role A가 쓸 Connectivity API
    uint32 GetConnectivityId(float x, float y, float z);

    int32 GetMapId() const { return _mapId; }
    int32 GetSizeX() const { return _sizeX; }
    int32 GetSizeY() const { return _sizeY; }

    // ===============================
    // [B] Pathfinding Wrappers
    // ===============================
    bool FindPathWaypoints(const Protocol::PositionInfo& start,
        const Protocol::PositionInfo& end,
        std::vector<Vector3>& outWaypoints);

    bool HasLineOfSight(const Protocol::PositionInfo& start,
        const Protocol::PositionInfo& end);

private:
    int32 _mapId = 0;
    int32 _sizeX = 0;
    int32 _sizeY = 0;

    std::shared_ptr<NavSystem> _navSystem;
};