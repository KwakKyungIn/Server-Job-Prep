#include "pch.h"
#include "DataManager.h"

#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

// 문자열로 된 맵 타입을 enum으로 변환해주는 헬퍼 함수
static MapType ParseMapType(const std::string& s)
{
	if (s == "dungeon") return MapType::Dungeon;
	return MapType::World;
}

DataManager::DataManager()
{
	// 생성자에서는 일단 하드코딩된 기본 맵 정보를 넣어둠
	// JSON 로딩 실패했을 때를 대비한 안전장치
	InitMapRegistry();
}

void DataManager::InitMapRegistry()
{
	_mapConfigs.clear();

	// 테스트용으로 1~4번 맵을 강제로 만듦
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

	// 기본 시작 맵은 1번
	_defaultWorldMapId = 1;
}

bool DataManager::IsValidMapId(int32 mapId) const
{
	// 맵 ID가 실제 map 컨테이너에 존재하는지 확인
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
	// DB 서버에서 보내준 기획 데이터를 받아서 메모리에 적재하는 함수
	// 기존 데이터 싹 밀고 새로 채워넣음
	_statTemplates.clear();
	_itemTemplates.clear();
	_skillTemplates.clear();

	// 레벨별 스탯 정보 로딩
	for (const auto& stat : pkt.stats())
	{
		_statTemplates[stat.level()] = stat;
	}

	// 아이템 정보 로딩 (ID로 검색하기 위해 맵에 저장)
	for (const auto& item : pkt.items())
	{
		_itemTemplates[item.templateid()] = item;
	}

	// 스킬 정보 로딩
	for (const auto& skill : pkt.skills())
	{
		_skillTemplates[skill.skillid()] = skill;
	}

	std::cout << " [DataManager] Load Complete. Stats: " << _statTemplates.size()
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
	// 외부 JSON 파일에서 맵 설정값들을 읽어오는 기능
	// 맵 크기나 스폰 위치 같은건 하드코딩하면 나중에 수정하기 힘드니까 파일로 뺌
	std::ifstream ifs(path);
	if (!ifs.is_open())
	{
		std::cout << " [DataManager] Failed to open: " << path << std::endl;
		return false;
	}

	json j;
	try { ifs >> j; }
	catch (const std::exception& e)
	{
		// JSON 형식이 잘못되었을 경우 예외 처리
		std::cout << " [DataManager] JSON parse error: " << e.what() << std::endl;
		return false;
	}

	_mapConfigs.clear();
	_defaultWorldMapId = j.value("defaultWorldMapId", 1);

	// maps 배열이 없거나 형식이 안맞으면 실패 처리
	if (!j.contains("maps") || !j["maps"].is_array())
	{
		std::cout << " [DataManager] maps array missing" << std::endl;
		return false;
	}

	// JSON 배열 순회하면서 맵 정보 파싱
	for (const auto& m : j["maps"])
	{
		MapConfig cfg;
		cfg.mapId = m.value("mapId", 0);
		cfg.sizeX = m.value("sizeX", 100);
		cfg.sizeY = m.value("sizeY", 100);

		// 지역(Zone) 관리랑 시야(AOI) 처리를 위한 설정값들
		// 클라이언트랑 서버랑 이 값이 일치해야 동기화가 잘 됨
		cfg.zoneSize = m.value("zoneSize", 64);
		cfg.aoiCellSize = m.value("aoiCellSize", 64);
		cfg.interestRadius = m.value("interestRadius", 150.0f);
		cfg.navMeshPath = m.value("navMeshPath", ""); // 네비메쉬 파일 경로

		// 초기 스폰 좌표
		cfg.spawnX = m.value("spawnX", 50.0f);
		cfg.spawnY = m.value("spawnY", 0.0f);
		cfg.spawnZ = m.value("spawnZ", 50.0f);

		cfg.type = ParseMapType(m.value("type", "world"));

		if (cfg.mapId <= 0) continue;
		_mapConfigs[cfg.mapId] = cfg;
	}

	// 만약 설정파일에서 읽은 시작 맵 ID가 월드 타입이 아니면 곤란하니까
	// 그냥 첫번째 월드맵 찾아서 그걸로 세팅함
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

	std::cout << " [DataManager] MapConfigs loaded: " << _mapConfigs.size()
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