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

// 맵 하나에 대한 설정 정보를 담는 구조체
// JSON 파일이나 하드코딩된 값들이 여기로 들어옴
struct MapConfig
{
	int32 mapId = 1;
	int32 sizeX = 100;
	int32 sizeY = 100;

	// [AOI & Zone]
	// 그리드 방식 시야 처리를 위해 필요한 값들
	int32 zoneSize = 64;       // 존 하나당 크기
	int32 aoiCellSize = 64;    // AOI 계산할 때 쓰는 셀 크기
	float interestRadius = 150.f; // 플레이어 시야 반경

	// [Spawn]
	// 맵 진입 시 초기 위치
	float spawnX = 50.f;
	float spawnY = 0.f;
	float spawnZ = 50.f;

	// [Path]
	std::string navMeshPath;   // 길찾기용 네비메쉬 바이너리 파일 경로

	MapType type = MapType::World;
};

// 몬스터 템플릿 정보 (JSON 기반 기획 데이터)
struct MonsterTemplate
{
	int32 monsterId = 0;
	std::string name;
	Protocol::StatInfo stat;
	int64 exp = 0;
	int32 dropGroupId = 0;

	float searchRange = 10.0f;
	float attackRange = 1.5f;
	float leashRange = 25.0f;
};

// 스폰 테이블 엔트리 (고정 좌표 기반)
struct SpawnEntry
{
	int32 spawnId = 0;
	int32 monsterId = 0;
	float x = 0.0f;
	float y = 0.0f;
	float z = 0.0f;

	int32 maxAlive = 1;
	int32 respawnSec = 30;
};

// 드랍 테이블 엔트리
struct DropEntry
{
	int32 itemId = 0;
	int32 weight = 0;
	int32 minCount = 1;
	int32 maxCount = 1;
};

struct DropGroup
{
	int32 dropGroupId = 0;
	int32 rolls = 1;
	int32 noDropWeight = 0;
	int32 totalWeight = 0;
	Vector<DropEntry> entries;
};

class DataManager
{
public:
	// 싱글톤 패턴 적용해서 어디서든 접근 가능하게 만듦
	static DataManager* Instance()
	{
		static DataManager instance;
		return &instance;
	}

	// DB 서버에서 받은 패킷으로 데이터 초기화하는 함수
	void LoadFromPacket(const Protocol::S2S_RES_LOAD_GAME_DATA& pkt);

	// 맵 정보 관련 함수들
	bool IsValidMapId(int32 mapId) const;
	int32 GetDefaultMapId() const { return 1; }
	const MapConfig* GetMapConfig(int32 mapId) const;

	// JSON 파일 파싱해서 맵 설정 로드
	bool LoadMapConfigsFromJson(const std::string& path);

	// JSON 파일 파싱해서 몬스터/스폰/드랍 데이터 로드
	bool LoadMonsterTemplatesFromJson(const std::string& path);
	bool LoadSpawnTablesFromJson(const std::string& path);
	bool LoadDropTablesFromJson(const std::string& path);

	// 월드맵인지 던전인지 구분할 때 사용
	int32 GetDefaultWorldMapId() const { return _defaultWorldMapId; }
	bool IsDungeonMapId(int32 mapId) const;
	bool IsWorldMapId(int32 mapId) const;

	// ID로 스탯, 아이템, 스킬 정보 찾아서 포인터 반환
	// 없으면 nullptr 리턴하니까 사용할 때 체크 필수
	const Protocol::StatTemplateInfo* GetStatTemplate(int32 level);
	const Protocol::ItemTemplateInfo* GetItemTemplate(int32 templateId);
	const Protocol::SkillTemplateInfo* GetSkillTemplate(int32 skillId);
	const MonsterTemplate* GetMonsterTemplate(int32 monsterId) const;
	const Vector<SpawnEntry>* GetSpawnEntries(int32 mapId) const;
	const DropGroup* GetDropGroup(int32 dropGroupId) const;

private:
	DataManager();                // 생성자에서 기본 맵 등록함
	void InitMapRegistry();       // 만약 파일 로드 실패하면 이거라도 씀

private:
	int32 _defaultWorldMapId = 1; // JSON에서 읽어온 기본 월드 맵 ID

	// 데이터 검색 속도를 위해 Map 자료구조 사용
	// Key: ID, Value: 데이터 구조체
	Map<int32, Protocol::StatTemplateInfo> _statTemplates;
	Map<int32, Protocol::ItemTemplateInfo> _itemTemplates;
	Map<int32, Protocol::SkillTemplateInfo> _skillTemplates;

	Map<int32, MapConfig> _mapConfigs;

	Map<int32, MonsterTemplate> _monsterTemplates;
	Map<int32, Vector<SpawnEntry>> _spawnTables;
	Map<int32, DropGroup> _dropGroups;
};
