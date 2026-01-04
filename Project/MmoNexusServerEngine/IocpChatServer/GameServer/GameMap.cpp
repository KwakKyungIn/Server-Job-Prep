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
    // Deprecated: legacy check (NavMesh 위인지 여부만)
    if (!_navSystem) return false;

    float dummyY = 0.f;
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