#pragma once
#include "Protocol.pb.h"
#include "DataManager.h" // MapConfig 구조체 사용
#include "ObjectUtils.h"

class NavSystem; // 헤더 의존성을 줄이기 위한 전방 선언 (Pimpl 관용구랑 비슷하게 씀)

// 서버에서 맵 하나를 관리하는 클래스
// 지형 정보, 네비게이션, 맵 크기 등을 담당함
class GameMap
{
public:
    GameMap();
    ~GameMap();

    // DataManager에서 읽어온 맵 설정값(크기, 네비메쉬 경로 등)으로 초기화
    bool Init(const MapConfig* config);

    // 단순 좌표 유효성 체크 (네비메쉬 위에 있는 점인가?)
    bool CanGo(const Protocol::PositionInfo& posInfo);

    // [이동 검증] 클라이언트가 보낸 좌표가 유효한지 검사하고 보정해주는 함수
    // 서버-클라 동기화의 핵심. 이상한 좌표면 outFixed에 갈 수 있는 가장 가까운 위치를 넣어줌
    bool ValidateMove(const Protocol::PositionInfo& current, const Protocol::PositionInfo& next, Protocol::PositionInfo& outFixed);

    // 현재 위치가 속한 네비메쉬 섬(Island) ID 반환
    // 맵이 끊겨있을 때 서로 이동 가능한지 판단하는 용도
    uint32 GetConnectivityId(float x, float y, float z);

    int32 GetMapId() const { return _mapId; }
    int32 GetSizeX() const { return _sizeX; }
    int32 GetSizeY() const { return _sizeY; }

    // ===============================
    // [길찾기 관련 래퍼 함수들]
    // ===============================

    // A* 알고리즘 등으로 시작점부터 도착점까지의 경로(웨이포인트)를 구함
    bool FindPathWaypoints(const Protocol::PositionInfo& start,
        const Protocol::PositionInfo& end,
        Vector<Vector3>& outWaypoints);

    // 네비메쉬 기반 레이캐스팅 (벽 판별)
    // outT: 충돌 지점 비율 (1.0 미만이면 중간에 벽이 있다는 뜻)
    bool RaycastNav(const Protocol::PositionInfo& start,
        const Protocol::PositionInfo& end,
        float& outT);

    // 두 지점 사이에 시야가 트여있는지 확인 (Line of Sight)
    bool HasLineOfSight(const Protocol::PositionInfo& start,
        const Protocol::PositionInfo& end);

private:
    int32 _mapId = 0;
    int32 _sizeX = 0;
    int32 _sizeY = 0;

    // 실제 길찾기 연산을 담당하는 객체 (Recast & Detour 라이브러리 사용)
    std::shared_ptr<NavSystem> _navSystem;
};