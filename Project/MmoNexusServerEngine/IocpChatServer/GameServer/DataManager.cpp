// ===============================
// DataManager.cpp (FINAL)
// ===============================
#include "pch.h"
#include "DataManager.h"

#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

static MapType ParseMapType(const std::string& s)
{
	if (s == "dungeon") return MapType::Dungeon;
	return MapType::World;
}

DataManager::DataManager()
{
	// 기존 동작 유지: fallback 하드코딩 맵 등록
	InitMapRegistry();
}

void DataManager::InitMapRegistry()
{
	_mapConfigs.clear();

	for (int32 id = 1; id <= 4; ++id)
	{
		MapConfig cfg;
		cfg.mapId = id;
		cfg.sizeX = 100;
		cfg.sizeY = 100;
		cfg.zoneSize = 10;
		cfg.spawnX = 50.f;
		cfg.spawnY = 0.f;
		cfg.spawnZ = 50.f;
		cfg.type = MapType::World;

		_mapConfigs[id] = cfg;
	}

	_defaultWorldMapId = 1;
}

bool DataManager::IsValidMapId(int32 mapId) const
{
	return _mapConfigs.find(mapId) != _mapConfigs.end();
}

const MapConfig* DataManager::GetMapConfig(int32 mapId) const
{
	auto it = _mapConfigs.find(mapId);
	if (it == _mapConfigs.end()) return nullptr;
	return &it->second;
}

void DataManager::LoadFromPacket(const Protocol::S2S_RES_LOAD_GAME_DATA& pkt)
{
	_statTemplates.clear();
	_itemTemplates.clear();
	_skillTemplates.clear();

	// 1. Stat Template 로딩
	for (const auto& stat : pkt.stats())
	{
		_statTemplates[stat.level()] = stat;
	}

	// 2. Item Template 로딩
	for (const auto& item : pkt.items())
	{
		_itemTemplates[item.templateid()] = item;
	}

	// 3. Skill Template 로딩
	for (const auto& skill : pkt.skills())
	{
		_skillTemplates[skill.skillid()] = skill;
	}

	std::cout << "📚 [DataManager] Load Complete. Stats: " << _statTemplates.size()
		<< ", Items: " << _itemTemplates.size()
		<< ", Skills: " << _skillTemplates.size() << std::endl;
}

const Protocol::StatTemplateInfo* DataManager::GetStatTemplate(int32 level)
{
	auto it = _statTemplates.find(level);
	if (it == _statTemplates.end())
		return nullptr;
	return &(it->second);
}

const Protocol::ItemTemplateInfo* DataManager::GetItemTemplate(int32 templateId)
{
	auto it = _itemTemplates.find(templateId);
	if (it == _itemTemplates.end())
		return nullptr;
	return &(it->second);
}

const Protocol::SkillTemplateInfo* DataManager::GetSkillTemplate(int32 skillId)
{
	auto it = _skillTemplates.find(skillId);
	if (it == _skillTemplates.end())
		return nullptr;
	return &(it->second);
}

bool DataManager::LoadMapConfigsFromJson(const std::string& path)
{
	std::ifstream ifs(path);
	if (!ifs.is_open())
	{
		std::cout << "❌ [DataManager] Failed to open: " << path << std::endl;
		return false;
	}

	json j;
	try { ifs >> j; }
	catch (const std::exception& e)
	{
		std::cout << "❌ [DataManager] JSON parse error: " << e.what() << std::endl;
		return false;
	}

	_mapConfigs.clear();
	_defaultWorldMapId = j.value("defaultWorldMapId", 1);

	if (!j.contains("maps") || !j["maps"].is_array())
	{
		std::cout << "❌ [DataManager] maps array missing" << std::endl;
		return false;
	}

	for (const auto& m : j["maps"])
	{
		MapConfig cfg;
		cfg.mapId = m.value("mapId", 0);
		cfg.sizeX = m.value("sizeX", 100);
		cfg.sizeY = m.value("sizeY", 100);
		cfg.zoneSize = m.value("zoneSize", 10);

		cfg.spawnX = m.value("spawnX", 50.0f);
		cfg.spawnY = m.value("spawnY", 0.0f);
		cfg.spawnZ = m.value("spawnZ", 50.0f);

		cfg.type = ParseMapType(m.value("type", "world"));

		if (cfg.mapId <= 0) continue;
		_mapConfigs[cfg.mapId] = cfg;
	}

	// 방어: defaultWorldMapId가 월드가 아니면, 첫 월드맵으로 교정
	if (!IsWorldMapId(_defaultWorldMapId))
	{
		for (auto& kv : _mapConfigs)
		{
			if (kv.second.type == MapType::World)
			{
				_defaultWorldMapId = kv.first;
				break;
			}
		}
	}

	std::cout << "✅ [DataManager] MapConfigs loaded: " << _mapConfigs.size()
		<< " (defaultWorldMapId=" << _defaultWorldMapId << ")\n";
	return true;
}

bool DataManager::IsDungeonMapId(int32 mapId) const
{
	auto it = _mapConfigs.find(mapId);
	if (it == _mapConfigs.end()) return false;
	return it->second.type == MapType::Dungeon;
}

bool DataManager::IsWorldMapId(int32 mapId) const
{
	auto it = _mapConfigs.find(mapId);
	if (it == _mapConfigs.end()) return false;
	return it->second.type == MapType::World;
}
