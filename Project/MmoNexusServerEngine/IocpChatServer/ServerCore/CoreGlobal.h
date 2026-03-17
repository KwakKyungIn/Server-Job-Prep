#pragma once

#include "Types.h"

#include <atomic>
#include <cstdint>
#include <string>

enum class HotRoomAoiMode : std::uint8_t
{
	Final = 0,
	RoomWideBaseline,
};

enum class PersistenceMode : std::uint8_t
{
	Writeback = 0,
	ImmediateQuickslot,
};

struct ServerConfig
{
	struct MetricsConfig
	{
		bool Enabled = false;
		int32 Port = 8080;
		std::string Prefix;
		std::string Path = "/metrics";
		std::string BindAddress = "127.0.0.1";
	};

	struct ExperimentConfig
	{
		bool Enabled = false;
		HotRoomAoiMode HotRoomMode = HotRoomAoiMode::Final;
		PersistenceMode Persistence = PersistenceMode::Writeback;
		int32 AutoCommitIntervalSec = 120;
		int32 ForceEnterWorldMapId = 0;
		bool RandomSpawnOnEnter = false;
		bool RandomSpawnOnRespawn = false;
		float RandomSpawnRadius = 0.0f;
	};

	std::wstring DBConnectionString;
	int32 Port = 0;
	int32 MaxUser = 0;
	MetricsConfig Metrics;
	ExperimentConfig Experiment;
};

using MetricsConfig = ServerConfig::MetricsConfig;

extern ServerConfig GServerConfig;

extern class ThreadManager* GThreadManager;
extern class Memory* GMemory;
extern class SendBufferManager* GSendBufferManager;
extern class DeadLockProfiler* GDeadLockProfiler;
extern class GlobalQueue* GGlobalQueue;
extern std::atomic<bool> GIsRunning;
extern class RedisManager* GRedisManager;

class CoreGlobal
{
public:
	CoreGlobal();
	~CoreGlobal();
};

extern class CoreGlobal GCoreGlobal;
