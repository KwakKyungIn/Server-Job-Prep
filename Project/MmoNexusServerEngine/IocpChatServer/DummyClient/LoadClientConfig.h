#pragma once
#include <string>

struct LoadClientConfig
{
	struct ServerEndpoint
	{
		std::string ip = "127.0.0.1";
		int port = 0;
	};

	struct SpawnCluster
	{
		float centerX = 0.0f;
		float centerY = 0.0f;
		float centerZ = 0.0f;
		float radius = 0.0f;
	};

	struct AccountConfig
	{
		std::string prefix = "test_";
		int start = 1;
		int count = 1;
		int padWidth = 6;
		std::string password = "pw";
	};

	struct Timeouts
	{
		int connectMs = 3000;
		int loginMs = 5000;
		int enterMs = 5000;
	};

	struct Options
	{
		bool keepLoginConnection = false;
		bool replaceOnFail = true;
		int logIntervalSec = 5;
		bool csvOutput = true;
		std::string csvPath = "docs/perf_results/run.csv";
		int ioThreads = 2;
	};

	struct NavMeshOptions
	{
		bool enabled = true;
		std::string mapsPath = "Maps.json";
		std::string navMeshPath;
		int maxGoalAttempts = 8;
		int repathIntervalSec = 5;
		float goalRadius = 0.0f;      // 0이면 spawn_cluster.radius 사용
		float waypointReachDist = 0.5f;
	};

	ServerEndpoint loginServer;
	ServerEndpoint gameServer;

	int ccuTarget = 0;
	int rampStep = 0;
	int rampIntervalSec = 0;
	int holdSec = 0;

	int channelId = 1;
	int mapId = 1;

	std::string scenario = "idle";
	float moveHz = 0.0f;
	float skillHz = 0.0f;
	float heartbeatHz = 0.0f;
	float quickslotHz = 0.0f;

	SpawnCluster spawnCluster;
	AccountConfig account;
	Timeouts timeouts;
	Options options;
	NavMeshOptions navMesh;

	static bool LoadFromFile(const std::string& path, LoadClientConfig& outConfig, std::string& outError);
};
