#pragma once

#include "CoreGlobal.h"
#include "DataManager.h"
#include "GameMap.h"
#include "Protocol.pb.h"

#include <algorithm>
#include <cmath>
#include <random>

namespace ExperimentUtils
{
	inline float ClampFloat(float value, float minValue, float maxValue)
	{
		if (value < minValue)
			return minValue;
		if (value > maxValue)
			return maxValue;
		return value;
	}

	inline bool IsEnabled()
	{
		return GServerConfig.Experiment.Enabled;
	}

	inline HotRoomAoiMode GetHotRoomAoiMode()
	{
		if (IsEnabled() == false)
			return HotRoomAoiMode::Final;

		return GServerConfig.Experiment.HotRoomMode;
	}

	inline bool IsHotRoomRoomWideBaseline()
	{
		return GetHotRoomAoiMode() == HotRoomAoiMode::RoomWideBaseline;
	}

	inline bool IsHotRoomFinal()
	{
		return GetHotRoomAoiMode() == HotRoomAoiMode::Final;
	}

	inline PersistenceMode GetPersistenceMode()
	{
		if (IsEnabled() == false)
			return PersistenceMode::Writeback;

		return GServerConfig.Experiment.Persistence;
	}

	inline bool IsPersistenceWriteback()
	{
		return GetPersistenceMode() == PersistenceMode::Writeback;
	}

	inline bool IsPersistenceImmediateQuickslot()
	{
		return GetPersistenceMode() == PersistenceMode::ImmediateQuickslot;
	}

	inline int32 GetAutoCommitIntervalSec()
	{
		if (IsEnabled() == false)
			return 120;

		const int32 intervalSec = GServerConfig.Experiment.AutoCommitIntervalSec;
		return (intervalSec > 0) ? intervalSec : 120;
	}

	inline int32 ResolveForcedWorldMapId(int32 fallbackMapId)
	{
		if (IsEnabled() == false)
			return fallbackMapId;

		const int32 forcedMapId = GServerConfig.Experiment.ForceEnterWorldMapId;
		if (forcedMapId <= 0)
			return fallbackMapId;

		DataManager* dm = DataManager::Instance();
		if (dm == nullptr)
			return fallbackMapId;

		if (dm->IsValidMapId(forcedMapId) == false)
			return fallbackMapId;
		if (dm->IsWorldMapId(forcedMapId) == false)
			return fallbackMapId;
		if (dm->GetMapConfig(forcedMapId) == nullptr)
			return fallbackMapId;

		return forcedMapId;
	}

	inline bool ShouldRandomizeEnterSpawn()
	{
		return IsEnabled()
			&& GServerConfig.Experiment.RandomSpawnOnEnter
			&& GServerConfig.Experiment.RandomSpawnRadius > 0.0f;
	}

	inline bool ShouldRandomizeRespawnSpawn()
	{
		return IsEnabled()
			&& GServerConfig.Experiment.RandomSpawnOnRespawn
			&& GServerConfig.Experiment.RandomSpawnRadius > 0.0f;
	}

	inline bool TryRandomizeSpawn(int32 mapId, Protocol::PositionInfo& inOutSpawn, GameMap* gameMap = nullptr)
	{
		DataManager* dm = DataManager::Instance();
		const MapConfig* cfg = (dm ? dm->GetMapConfig(mapId) : nullptr);
		const float radius = GServerConfig.Experiment.RandomSpawnRadius;
		if (cfg == nullptr || radius <= 0.0f)
			return false;

		const float centerX = inOutSpawn.x();
		const float centerY = inOutSpawn.y();
		const float centerZ = inOutSpawn.z();

		const float minX = 0.0f;
		const float minZ = 0.0f;
		const float maxX = (cfg->sizeX > 1) ? static_cast<float>(cfg->sizeX - 1) : 0.0f;
		const float maxZ = (cfg->sizeY > 1) ? static_cast<float>(cfg->sizeY - 1) : 0.0f;

		constexpr float kTwoPi = 6.28318530718f;
		constexpr int32 kMaxAttempts = 16;

		thread_local std::mt19937 rng{ std::random_device{}() };
		std::uniform_real_distribution<float> angleDist(0.0f, kTwoPi);
		std::uniform_real_distribution<float> radiusDist(0.0f, 1.0f);

		for (int32 attempt = 0; attempt < kMaxAttempts; ++attempt)
		{
			const float sampledRadius = std::sqrt(radiusDist(rng)) * radius;
			const float sampledAngle = angleDist(rng);

			Protocol::PositionInfo candidate = inOutSpawn;
			candidate.set_x(ClampFloat(centerX + std::cos(sampledAngle) * sampledRadius, minX, maxX));
			candidate.set_y(centerY);
			candidate.set_z(ClampFloat(centerZ + std::sin(sampledAngle) * sampledRadius, minZ, maxZ));

			if (gameMap && gameMap->CanGo(candidate) == false)
				continue;

			inOutSpawn.CopyFrom(candidate);
			return true;
		}

		return false;
	}
}
