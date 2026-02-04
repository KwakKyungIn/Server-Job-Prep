#include "pch.h"
#include "LoadClientConfig.h"

#include <fstream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

static void LoadServerEndpoint(const json& j, const char* key, LoadClientConfig::ServerEndpoint& out)
{
	if (!j.contains(key) || !j[key].is_object())
		return;
	const json& obj = j[key];
	out.ip = obj.value("ip", out.ip);
	out.port = obj.value("port", out.port);
}

bool LoadClientConfig::LoadFromFile(const std::string& path, LoadClientConfig& outConfig, std::string& outError)
{
	std::ifstream ifs(path);
	if (!ifs.is_open())
	{
		outError = "failed to open config: " + path;
		return false;
	}

	json j;
	try
	{
		ifs >> j;
	}
	catch (const std::exception& e)
	{
		outError = std::string("json parse error: ") + e.what();
		return false;
	}

	LoadServerEndpoint(j, "login_server", outConfig.loginServer);
	LoadServerEndpoint(j, "game_server", outConfig.gameServer);

	outConfig.ccuTarget = j.value("ccu_target", outConfig.ccuTarget);
	outConfig.rampStep = j.value("ramp_step", outConfig.rampStep);
	outConfig.rampIntervalSec = j.value("ramp_interval_sec", outConfig.rampIntervalSec);
	outConfig.holdSec = j.value("hold_sec", outConfig.holdSec);

	outConfig.channelId = j.value("channel_id", outConfig.channelId);
	outConfig.mapId = j.value("map_id", outConfig.mapId);

	outConfig.scenario = j.value("scenario", outConfig.scenario);
	outConfig.moveHz = j.value("move_hz", outConfig.moveHz);
	outConfig.skillHz = j.value("skill_hz", outConfig.skillHz);
	outConfig.heartbeatHz = j.value("heartbeat_hz", outConfig.heartbeatHz);

	if (j.contains("spawn_cluster") && j["spawn_cluster"].is_object())
	{
		const json& sc = j["spawn_cluster"];
		outConfig.spawnCluster.centerX = sc.value("center_x", outConfig.spawnCluster.centerX);
		outConfig.spawnCluster.centerY = sc.value("center_y", outConfig.spawnCluster.centerY);
		outConfig.spawnCluster.centerZ = sc.value("center_z", outConfig.spawnCluster.centerZ);
		outConfig.spawnCluster.radius = sc.value("radius", outConfig.spawnCluster.radius);
	}

	if (j.contains("account") && j["account"].is_object())
	{
		const json& ac = j["account"];
		outConfig.account.prefix = ac.value("prefix", outConfig.account.prefix);
		outConfig.account.start = ac.value("start", outConfig.account.start);
		outConfig.account.count = ac.value("count", outConfig.account.count);
		outConfig.account.padWidth = ac.value("pad_width", outConfig.account.padWidth);
		outConfig.account.password = ac.value("password", outConfig.account.password);
	}

	if (j.contains("timeouts") && j["timeouts"].is_object())
	{
		const json& t = j["timeouts"];
		outConfig.timeouts.connectMs = t.value("connect_ms", outConfig.timeouts.connectMs);
		outConfig.timeouts.loginMs = t.value("login_ms", outConfig.timeouts.loginMs);
		outConfig.timeouts.enterMs = t.value("enter_ms", outConfig.timeouts.enterMs);
	}

	if (j.contains("options") && j["options"].is_object())
	{
		const json& opt = j["options"];
		outConfig.options.keepLoginConnection = opt.value("keep_login_connection", outConfig.options.keepLoginConnection);
		outConfig.options.replaceOnFail = opt.value("replace_on_fail", outConfig.options.replaceOnFail);
		outConfig.options.logIntervalSec = opt.value("log_interval_sec", outConfig.options.logIntervalSec);
		outConfig.options.csvOutput = opt.value("csv_output", outConfig.options.csvOutput);
		outConfig.options.csvPath = opt.value("csv_path", outConfig.options.csvPath);
		outConfig.options.ioThreads = opt.value("io_threads", outConfig.options.ioThreads);
	}

	if (j.contains("navmesh") && j["navmesh"].is_object())
	{
		const json& nm = j["navmesh"];
		outConfig.navMesh.enabled = nm.value("enabled", outConfig.navMesh.enabled);
		outConfig.navMesh.mapsPath = nm.value("maps_path", outConfig.navMesh.mapsPath);
		outConfig.navMesh.navMeshPath = nm.value("navmesh_path", outConfig.navMesh.navMeshPath);
		outConfig.navMesh.maxGoalAttempts = nm.value("max_goal_attempts", outConfig.navMesh.maxGoalAttempts);
		outConfig.navMesh.repathIntervalSec = nm.value("repath_interval_sec", outConfig.navMesh.repathIntervalSec);
		outConfig.navMesh.goalRadius = nm.value("goal_radius", outConfig.navMesh.goalRadius);
		outConfig.navMesh.waypointReachDist = nm.value("waypoint_reach_dist", outConfig.navMesh.waypointReachDist);
	}

	if (outConfig.loginServer.port == 0 || outConfig.gameServer.port == 0)
	{
		outError = "invalid server port in config";
		return false;
	}

	if (outConfig.ccuTarget <= 0)
	{
		outError = "ccu_target must be > 0";
		return false;
	}

	if (outConfig.rampStep <= 0)
		outConfig.rampStep = outConfig.ccuTarget;
	if (outConfig.rampIntervalSec <= 0)
		outConfig.rampIntervalSec = 1;
	if (outConfig.account.count <= 0)
		outConfig.account.count = outConfig.ccuTarget;
	if (outConfig.options.ioThreads <= 0)
		outConfig.options.ioThreads = 1;

	return true;
}
