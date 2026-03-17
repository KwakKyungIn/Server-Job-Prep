#include "pch.h"
#include "CoreGlobal.h"

#include "DeadLockProfiler.h"
#include "GlobalQueue.h"
#include "Memory.h"
#include "MetricsSystem.h"
#include "RedisManager.h"
#include "SendBuffer.h"
#include "SocketUtils.h"
#include "ThreadManager.h"

#include <cstdint>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <vector>

using json = nlohmann::json;

namespace
{
	int32 ReadInt(const json& object, const char* key, int32 defaultValue)
	{
		if (object.is_object() && object.contains(key) && object[key].is_number_integer())
			return static_cast<int32>(object[key].get<int64_t>());
		return defaultValue;
	}

	bool ReadBool(const json& object, const char* key, bool defaultValue)
	{
		if (object.is_object() && object.contains(key) && object[key].is_boolean())
			return object[key].get<bool>();
		return defaultValue;
	}

	float ReadFloat(const json& object, const char* key, float defaultValue)
	{
		if (object.is_object() && object.contains(key) && object[key].is_number())
			return object[key].get<float>();
		return defaultValue;
	}

	std::string ReadString(const json& object, const char* key, const std::string& defaultValue)
	{
		if (object.is_object() && object.contains(key) && object[key].is_string())
			return object[key].get<std::string>();
		return defaultValue;
	}

	HotRoomAoiMode ReadHotRoomAoiMode(const json& object, const char* key, HotRoomAoiMode defaultValue)
	{
		if (object.is_object() == false || object.contains(key) == false || object[key].is_string() == false)
			return defaultValue;

		const std::string value = object[key].get<std::string>();
		if (value == "final")
			return HotRoomAoiMode::Final;
		if (value == "room_wide_baseline")
			return HotRoomAoiMode::RoomWideBaseline;

		std::cout << "[Config][WARN] Unknown Experiment." << key << "(" << value
			<< "). Fallback to final." << std::endl;
		return HotRoomAoiMode::Final;
	}

	PersistenceMode ReadPersistenceMode(const json& object, const char* key, PersistenceMode defaultValue)
	{
		if (object.is_object() == false || object.contains(key) == false || object[key].is_string() == false)
			return defaultValue;

		const std::string value = object[key].get<std::string>();
		if (value == "writeback")
			return PersistenceMode::Writeback;
		if (value == "immediate_quickslot")
			return PersistenceMode::ImmediateQuickslot;

		std::cout << "[Config][WARN] Unknown Experiment." << key << "(" << value
			<< "). Fallback to writeback." << std::endl;
		return PersistenceMode::Writeback;
	}

	void NormalizeMetricsConfig(MetricsConfig& metrics)
	{
		if (metrics.Path.empty())
			metrics.Path = "/metrics";
		if (metrics.Path.front() != '/')
			metrics.Path = "/" + metrics.Path;

		if (metrics.BindAddress.empty())
			metrics.BindAddress = "127.0.0.1";

		if (metrics.Port < 0 || metrics.Port > 65535)
		{
			std::cout << "[Config][WARN] Invalid Metrics.Port(" << metrics.Port
				<< "). Metrics disabled." << std::endl;
			metrics.Enabled = false;
			metrics.Port = 0;
		}
	}

	void NormalizeExperimentConfig(ServerConfig::ExperimentConfig& experiment)
	{
		if (experiment.AutoCommitIntervalSec <= 0)
			experiment.AutoCommitIntervalSec = 120;

		if (experiment.ForceEnterWorldMapId < 0)
			experiment.ForceEnterWorldMapId = 0;

		if (experiment.RandomSpawnRadius < 0.0f)
			experiment.RandomSpawnRadius = 0.0f;
	}

	std::string WideToUtf8(const std::wstring& value)
	{
#if defined(_WIN32)
		if (value.empty())
			return std::string();

		const int requiredSize = ::WideCharToMultiByte(
			CP_UTF8,
			0,
			value.c_str(),
			static_cast<int>(value.size()),
			nullptr,
			0,
			nullptr,
			nullptr);
		if (requiredSize <= 0)
			return std::string();

		std::string utf8(static_cast<size_t>(requiredSize), '\0');
		const int convertedSize = ::WideCharToMultiByte(
			CP_UTF8,
			0,
			value.c_str(),
			static_cast<int>(value.size()),
			&utf8[0],
			requiredSize,
			nullptr,
			nullptr);
		if (convertedSize <= 0)
			return std::string();

		return utf8;
#else
		return std::string(value.begin(), value.end());
#endif
	}

	std::string GetExecutableBaseName()
	{
#if defined(_WIN32)
		wchar_t modulePath[MAX_PATH] = {};
		const DWORD length = ::GetModuleFileNameW(nullptr, modulePath, MAX_PATH);
		if (length == 0 || length >= MAX_PATH)
			return std::string();

		std::wstring filePath(modulePath, length);
		const size_t lastSlash = filePath.find_last_of(L"\\/");
		std::wstring fileName = (lastSlash == std::wstring::npos)
			? filePath
			: filePath.substr(lastSlash + 1);

		const size_t lastDot = fileName.find_last_of(L'.');
		if (lastDot != std::wstring::npos)
			fileName = fileName.substr(0, lastDot);

		return WideToUtf8(fileName);
#else
		return std::string();
#endif
	}

	std::string ResolveServerConfigPath()
	{
		const std::string executableName = GetExecutableBaseName();
		std::vector<std::string> candidates;

		if (executableName.empty() == false)
		{
			candidates.push_back("ServerConfig." + executableName + ".json");
		}
		candidates.push_back("ServerConfig.json");

		for (const std::string& path : candidates)
		{
			std::ifstream file(path);
			if (file.is_open())
				return path;
		}

		return std::string();
	}

	void LoadServerConfig(ServerConfig& outConfig)
	{
		outConfig = ServerConfig();

		const std::string configPath = ResolveServerConfigPath();
		if (configPath.empty())
		{
			std::cout << "[Config][WARN] ServerConfig.json not found. Defaults will be used." << std::endl;
			NormalizeMetricsConfig(outConfig.Metrics);
			return;
		}

		std::ifstream file(configPath);
		if (file.is_open() == false)
		{
			std::cout << "[Config][WARN] ServerConfig.json not found. Defaults will be used." << std::endl;
			NormalizeMetricsConfig(outConfig.Metrics);
			return;
		}

		try
		{
			json data;
			file >> data;

			if (data.contains("DB") && data["DB"].is_object())
			{
				const json& db = data["DB"];
				const std::string connStr = ReadString(db, "ConnectionString", std::string());
				outConfig.DBConnectionString.assign(connStr.begin(), connStr.end());
			}

			if (data.contains("Server") && data["Server"].is_object())
			{
				const json& server = data["Server"];
				outConfig.Port = ReadInt(server, "Port", outConfig.Port);
				outConfig.MaxUser = ReadInt(server, "MaxUser", outConfig.MaxUser);
			}

			if (data.contains("Metrics") && data["Metrics"].is_object())
			{
				const json& metrics = data["Metrics"];
				outConfig.Metrics.Enabled = ReadBool(metrics, "Enabled", outConfig.Metrics.Enabled);
				outConfig.Metrics.Port = ReadInt(metrics, "Port", outConfig.Metrics.Port);
				outConfig.Metrics.Prefix = ReadString(metrics, "Prefix", outConfig.Metrics.Prefix);
				outConfig.Metrics.Path = ReadString(metrics, "Path", outConfig.Metrics.Path);
				outConfig.Metrics.BindAddress = ReadString(metrics, "BindAddress", outConfig.Metrics.BindAddress);
			}

			if (data.contains("Experiment") && data["Experiment"].is_object())
			{
				const json& experiment = data["Experiment"];
				outConfig.Experiment.Enabled = ReadBool(experiment, "Enabled", outConfig.Experiment.Enabled);
				outConfig.Experiment.HotRoomMode = ReadHotRoomAoiMode(experiment, "HotRoomAoiMode", outConfig.Experiment.HotRoomMode);
				outConfig.Experiment.Persistence = ReadPersistenceMode(experiment, "PersistenceMode", outConfig.Experiment.Persistence);
				outConfig.Experiment.AutoCommitIntervalSec = ReadInt(experiment, "AutoCommitIntervalSec", outConfig.Experiment.AutoCommitIntervalSec);
				outConfig.Experiment.ForceEnterWorldMapId = ReadInt(experiment, "ForceEnterWorldMapId", outConfig.Experiment.ForceEnterWorldMapId);
				outConfig.Experiment.RandomSpawnOnEnter = ReadBool(experiment, "RandomSpawnOnEnter", outConfig.Experiment.RandomSpawnOnEnter);
				outConfig.Experiment.RandomSpawnOnRespawn = ReadBool(experiment, "RandomSpawnOnRespawn", outConfig.Experiment.RandomSpawnOnRespawn);
				outConfig.Experiment.RandomSpawnRadius = ReadFloat(experiment, "RandomSpawnRadius", outConfig.Experiment.RandomSpawnRadius);
			}

			NormalizeMetricsConfig(outConfig.Metrics);
			NormalizeExperimentConfig(outConfig.Experiment);
			std::cout << "[Config] Loaded Successfully: " << configPath << std::endl;
		}
		catch (const std::exception& e)
		{
			std::cout << "[Config][WARN] Failed to parse " << configPath << ": " << e.what()
				<< ". Defaults will be used." << std::endl;
			outConfig = ServerConfig();
			NormalizeMetricsConfig(outConfig.Metrics);
			NormalizeExperimentConfig(outConfig.Experiment);
		}
	}
}

ThreadManager* GThreadManager = nullptr;
Memory* GMemory = nullptr;
SendBufferManager* GSendBufferManager = nullptr;
DeadLockProfiler* GDeadLockProfiler = nullptr;
GlobalQueue* GGlobalQueue = nullptr;
std::atomic<bool> GIsRunning = true;
RedisManager* GRedisManager = nullptr;

ServerConfig GServerConfig;

CoreGlobal::CoreGlobal()
{
	SocketUtils::Init();
	LoadServerConfig(GServerConfig);

	GThreadManager = new ThreadManager();
	GMemory = new Memory();
	GSendBufferManager = new SendBufferManager();
	GDeadLockProfiler = new DeadLockProfiler();
	GGlobalQueue = new GlobalQueue();
	GRedisManager = new RedisManager();

	GRedisManager->Connect("127.0.0.1", 6379);

	MetricsSystem::Instance().Initialize(GServerConfig.Metrics);

	std::cout << "CoreGlobal Initialized" << std::endl;
}

CoreGlobal::~CoreGlobal()
{
	MetricsSystem::Instance().Shutdown();

	delete GThreadManager;
	delete GMemory;
	delete GSendBufferManager;
	delete GDeadLockProfiler;
	delete GGlobalQueue;
	delete GRedisManager;

	SocketUtils::Clear();
}

CoreGlobal GCoreGlobal;
