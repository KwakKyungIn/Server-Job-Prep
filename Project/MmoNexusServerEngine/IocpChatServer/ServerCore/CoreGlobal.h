#pragma once

#include "Types.h"

#include <atomic>
#include <string>

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

	std::wstring DBConnectionString;
	int32 Port = 0;
	int32 MaxUser = 0;
	MetricsConfig Metrics;
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
