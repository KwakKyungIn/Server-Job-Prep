#include "pch.h"
#include "GameMap.h"
#include "NavSystem.h" // 구현부에선 포함

GameMap::GameMap()
{
    _navSystem = std::make_shared<NavSystem>();
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

    // NavMesh 로딩
    if (!config->navMeshPath.empty())
    {
        if (_navSystem->Load(config->navMeshPath))
        {
            std::cout << "✅ [GameMap] NavMesh Loaded: " << config->navMeshPath << std::endl;
        }
        else
        {
            std::cout << "❌ [GameMap] NavMesh Load Failed: " << config->navMeshPath << std::endl;
            // 실패 시 로직? (일단 진행하되 이동 불가 처리되겠지)
        }
    }

    return true;
}

bool GameMap::CanGo(const Protocol::PositionInfo& posInfo)
{
    // Deprecated: 구식 코드 호환용
    // 현재 위치가 NavMesh 위인지 체크
    float x = posInfo.x();
    float y = posInfo.z(); // 서버 좌표계 주의 (y가 높이인지 z가 높이인지 프로젝트마다 다름. 유니티는 y가 높이)
    // *주의*: 유니티(x,y,z) -> 서버(x,y,z) 그대로 쓴다면:

    // 유효성 체크용으로만 씀
    float dummyY;
    return _navSystem->ResolvePoint(posInfo.x(), posInfo.y(), posInfo.z(), dummyY);
}

bool GameMap::ValidateMove(const Protocol::PositionInfo& current, const Protocol::PositionInfo& next, Protocol::PositionInfo& outFixed)
{
    return _navSystem->ValidateMove(current, next, outFixed);
}

uint32 GameMap::GetConnectivityId(float x, float y, float z)
{
    return _navSystem->GetConnectivityId(x, y, z);
}