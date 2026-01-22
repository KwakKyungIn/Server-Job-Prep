#pragma once
#include "Protocol.pb.h"
#include "DataManager.h" // MapConfig
#include "ObjectUtils.h"

class NavSystem; // Forward Declaration (��� ������ �ּ�ȭ)

class GameMap
{
public:
    GameMap();
    ~GameMap();

    // MapConfig ��ü�� �޵��� ����
    bool Init(const MapConfig* config);

    // [Collision Check] -> [Move Validation]
    // �ܼ��� bool�� �ƴ϶�, ������ ��ġ�� ��ȯ�� �� �־�� ��
    // (������ �������̽� ������ ���� ���� CanGo�� �����ϵ�, ���������� NavSystem ȣ��)
    bool CanGo(const Protocol::PositionInfo& posInfo);

    // [New] Role B: �̵� ���� & ���� API
    bool ValidateMove(const Protocol::PositionInfo& current, const Protocol::PositionInfo& next, Protocol::PositionInfo& outFixed);

    // [New] Role A�� �� Connectivity API
    uint32 GetConnectivityId(float x, float y, float z);

    int32 GetMapId() const { return _mapId; }
    int32 GetSizeX() const { return _sizeX; }
    int32 GetSizeY() const { return _sizeY; }

    // ===============================
    // [B] Pathfinding Wrappers
    // ===============================
    bool FindPathWaypoints(const Protocol::PositionInfo& start,
        const Protocol::PositionInfo& end,
        Vector<Vector3>& outWaypoints);

    // [New] NavMesh raycast wrapper (t in [0,1], <1 means hit boundary/wall)
    bool RaycastNav(const Protocol::PositionInfo& start,
        const Protocol::PositionInfo& end,
        float& outT);

    bool HasLineOfSight(const Protocol::PositionInfo& start,
        const Protocol::PositionInfo& end);

private:
    int32 _mapId = 0;
    int32 _sizeX = 0;
    int32 _sizeY = 0;

    std::shared_ptr<NavSystem> _navSystem;
};