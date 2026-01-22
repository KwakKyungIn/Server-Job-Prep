#pragma once
#include "DetourNavMesh.h"
#include "DetourNavMeshQuery.h"
#include "Protocol.pb.h"
#include "ObjectUtils.h"

class NavSystem
{
public:
    NavSystem();
    ~NavSystem();

    bool Load(const std::string& path);
    bool ValidateMove(const Protocol::PositionInfo& current, const Protocol::PositionInfo& target, Protocol::PositionInfo& outAdjusted);
    bool ResolvePoint(float x, float y, float z, float& outY);
    uint32 GetConnectivityId(float x, float y, float z);

    // ===============================
    // [B] Pathfinding / LOS
    // ===============================
    bool FindPathWaypoints(const Protocol::PositionInfo& start,
        const Protocol::PositionInfo& end,
        Vector<Vector3>& outWaypoints);

    // outT: 1.0이면 직선 통과, 0~1이면 중간에 막힘
    bool RaycastNav(const Protocol::PositionInfo& start,
        const Protocol::PositionInfo& end,
        float& outT);

private:
    // [Delete] void ComputeConnectivity(); -> 삭제함
    dtPolyRef FindNearestPoly(const float* center, const float* extents, float* nearestPt = nullptr);

private:
    class dtNavMesh* _navMesh = nullptr;
    class dtNavMeshQuery* _navQuery = nullptr;

    Map<dtPolyRef, uint32> _polyGroups;

    // [New] 멤버 변수로 관리 (맵마다 독립적인 ID 카운트)
    uint32 _nextGroupId = 1;

    float _polyPickExt[3] = { 2.0f, 4.0f, 2.0f };
    dtQueryFilter _filter;
};