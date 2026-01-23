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

    // 네비메쉬 데이터 파일 로드
    // MSET 포맷(커스텀)과 일반 Detour 포맷 둘 다 지원하도록 처리함
    bool Load(const std::string& path);

    // 이동 가능한지 검증하는 핵심 함수
    // 클라에서 보낸 좌표가 갈 수 있는 곳인지 체크하고, 막혔으면 벽 타고 미끄러지는 좌표(Slide)를 계산해줌
    bool ValidateMove(const Protocol::PositionInfo& current, const Protocol::PositionInfo& target, Protocol::PositionInfo& outAdjusted);

    // 좌표가 네비메쉬 위에 있는지 확인하고, y값을 네비메쉬 높이에 맞춰줌 (스냅핑)
    bool ResolvePoint(float x, float y, float z, float& outY);

    // 현재 위치가 속한 구역(Island)의 ID를 반환
    // 맵이 끊겨있을 때 서로 도달 가능한지 판별하기 위해 사용함
    uint32 GetConnectivityId(float x, float y, float z);

    // ===============================
    // Pathfinding / LOS
    // ===============================

    // A* 알고리즘 기반으로 목적지까지의 경로점(Waypoints) 리스트를 구함
    // 입력된 시작/끝 점을 네비메쉬 위로 보정해서 길을 찾음
    bool FindPathWaypoints(const Protocol::PositionInfo& start,
        const Protocol::PositionInfo& end,
        Vector<Vector3>& outWaypoints);

    // 시작점에서 끝점까지 직선으로 갈 수 있는지(장애물이 없는지) 체크
    // outT가 1.0이면 장애물 없음, 그 미만이면 중간에 충돌
    bool RaycastNav(const Protocol::PositionInfo& start,
        const Protocol::PositionInfo& end,
        float& outT);

private:
    // 특정 좌표 주변에서 가장 가까운 폴리곤(Poly)을 찾음
    // extents 범위 내에서 검색 수행
    dtPolyRef FindNearestPoly(const float* center, const float* extents, float* nearestPt = nullptr);

private:
    class dtNavMesh* _navMesh = nullptr;
    class dtNavMeshQuery* _navQuery = nullptr;

    // 폴리곤 그룹 ID 캐싱용 맵
    // 매번 전체 맵을 계산하면 무거우니까 방문한 곳만 저장해둠
    Map<dtPolyRef, uint32> _polyGroups;

    // 그룹 ID 발급 카운터 (1부터 시작)
    uint32 _nextGroupId = 1;

    // 폴리곤 검색 범위 (x, y, z)
    // y축을 넉넉하게 잡아야 계단이나 언덕 같은 높이 차이가 있는 곳도 잘 찾음
    float _polyPickExt[3] = { 2.0f, 4.0f, 2.0f };

    dtQueryFilter _filter;
};