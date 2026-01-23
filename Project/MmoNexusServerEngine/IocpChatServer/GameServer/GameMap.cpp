#include "pch.h"
#include "GameMap.h"
#include "NavSystem.h" // NavSystem 구현체는 cpp에서만 포함해서 의존성 줄임

GameMap::GameMap()
{
    // 맵 생성 시 네비게이션 시스템도 같이 준비
    _navSystem = MakeShared<NavSystem>();
}

GameMap::~GameMap()
{
}

bool GameMap::Init(const MapConfig* config)
{
    if (config == nullptr) return false;

    _mapId = config->mapId;
    _sizeX = config->sizeX;
    _sizeY = config->sizeY;

    // 네비메쉬 파일 경로가 있다면 로딩 시도
    // 서버에서도 네비메쉬를 들고 있어야 클라이언트의 이동 검증이 가능함
    if (!config->navMeshPath.empty())
    {
        if (_navSystem->Load(config->navMeshPath))
        {
            std::cout << " [GameMap] NavMesh Loaded: " << config->navMeshPath << std::endl;
        }
        else
        {
            std::cout << " [GameMap] NavMesh Load Failed: " << config->navMeshPath << std::endl;
            // 로드 실패해도 일단 서버는 돌아가게 둠 (대신 이동 검증이 안 됨)
        }
    }

    return true;
}

bool GameMap::CanGo(const Protocol::PositionInfo& posInfo)
{
    // [Deprecated] 예전에 쓰던 단순 이동 가능 여부 체크 함수
    // 지금은 ValidateMove를 주로 쓰지만 하위 호환성을 위해 남겨둠
    if (!_navSystem) return false;

    float dummyY = 0.f;
    // 해당 좌표가 네비메쉬 위에 있는지 판별
    return _navSystem->ResolvePoint(posInfo.x(), posInfo.y(), posInfo.z(), dummyY);
}


bool GameMap::ValidateMove(const Protocol::PositionInfo& current, const Protocol::PositionInfo& next, Protocol::PositionInfo& outFixed)
{
    // 클라이언트가 보낸 이동 패킷을 검증하는 핵심 로직
    // 해킹이나 렉으로 인해 벽을 뚫거나 갈 수 없는 곳으로 가는 것을 방지
    // outFixed에 서버가 보정한 올바른 좌표를 담아줌
    return _navSystem->ValidateMove(current, next, outFixed);
}

uint32 GameMap::GetConnectivityId(float x, float y, float z)
{
    // 현재 위치가 네비메쉬 상의 어떤 폴리곤 구역(Island)에 있는지 식별
    // 갈 수 있는 구역인지 미리 판단할 때 사용
    return _navSystem->GetConnectivityId(x, y, z);
}

bool GameMap::FindPathWaypoints(const Protocol::PositionInfo& start,
    const Protocol::PositionInfo& end,
    Vector<Vector3>& outWaypoints)
{
    // 몬스터 AI 등을 위한 길찾기 함수
    // NavSystem(Recast & Detour)을 이용해 경로점들을 구해옴
    if (!_navSystem) return false;
    return _navSystem->FindPathWaypoints(start, end, outWaypoints);
}

bool GameMap::RaycastNav(const Protocol::PositionInfo& start,
    const Protocol::PositionInfo& end,
    float& outT)
{
    if (_navSystem == nullptr)
        return false;

    // 네비메쉬 상에서 레이캐스트 수행 (벽에 막히는지 체크)
    // outT는 충돌 지점까지의 비율 (0.0 ~ 1.0)
    return _navSystem->RaycastNav(start, end, outT);
}

bool GameMap::HasLineOfSight(const Protocol::PositionInfo& start,
    const Protocol::PositionInfo& end)
{
    // 두 지점 사이에 장애물이 없는지 확인 (시야 체크, 원거리 공격 가능 여부 등)
    if (!_navSystem) return false;

    float t = 0.0f;
    // 레이캐스트가 끝까지(1.0) 도달했으면 장애물이 없다는 뜻
    if (_navSystem->RaycastNav(start, end, t) == false)
        return false;

    return (t >= 1.0f);
}