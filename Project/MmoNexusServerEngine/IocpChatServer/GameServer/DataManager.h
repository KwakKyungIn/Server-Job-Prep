// ===============================
// DataManager.h (FINAL)
// ===============================
#pragma once
#include "Protocol.pb.h"
#include "Protocol_S2S.pb.h"
#include <map>
#include <string>

enum class MapType : uint8
{
	World,
	Dungeon,
};

// 구조체만 변경, 나머지는 그대로
struct MapConfig
{
	int32 mapId = 1;
	int32 sizeX = 100;
	int32 sizeY = 100;

	// [AOI & Zone]
	int32 zoneSize = 64;       // Grid Cell Size와 동일하게 맞추는 게 정석
	int32 aoiCellSize = 64;    // (New) AOI 계산용
	float interestRadius = 150.f; // (New) 시야 거리

	// [Spawn]
	float spawnX = 50.f;
	float spawnY = 0.f;
	float spawnZ = 50.f;

	// [Path]
	std::string navMeshPath;   // (New) .bin 파일 경로

	MapType type = MapType::World;
};

class DataManager
{
public:
	// [Singleton] 어디서든 접근 가능하게
	static DataManager* Instance()
	{
		static DataManager instance;
		return &instance;
	}

	// [Load] 패킷을 받아서 메모리에 저장
	void LoadFromPacket(const Protocol::S2S_RES_LOAD_GAME_DATA& pkt);

	// Map Registry
	bool IsValidMapId(int32 mapId) const;
	int32 GetDefaultMapId() const { return 1; } // (기존 유지)
	const MapConfig* GetMapConfig(int32 mapId) const;

	// [NEW] JSON 기반 MapConfig 로드
	bool LoadMapConfigsFromJson(const std::string& path);

	// [NEW] 월드/던전 구분용
	int32 GetDefaultWorldMapId() const { return _defaultWorldMapId; }
	bool IsDungeonMapId(int32 mapId) const;
	bool IsWorldMapId(int32 mapId) const;

	// [Getter] 데이터 조회 (없으면 nullptr 반환)
	const Protocol::StatTemplateInfo* GetStatTemplate(int32 level);
	const Protocol::ItemTemplateInfo* GetItemTemplate(int32 templateId);
	const Protocol::SkillTemplateInfo* GetSkillTemplate(int32 skillId);

private:
	DataManager();                // 생성 시 맵 등록
	void InitMapRegistry();       // fallback용(기존 유지)

private:
	int32 _defaultWorldMapId = 1; // JSON 로드 시 갱신

	// 빠른 검색을 위한 Map (Key: ID, Value: Data)
	Map<int32, Protocol::StatTemplateInfo> _statTemplates;
	Map<int32, Protocol::ItemTemplateInfo> _itemTemplates;
	Map<int32, Protocol::SkillTemplateInfo> _skillTemplates;

	Map<int32, MapConfig> _mapConfigs;
};
