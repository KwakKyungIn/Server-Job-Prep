#pragma once
#include "Protocol.pb.h"
#include "Protocol_S2S.pb.h"

struct MapConfig
{
	int32 mapId = 1;
	int32 sizeX = 100;
	int32 sizeY = 100;
	int32 zoneSize = 10;

	float spawnX = 50.f;
	float spawnY = 0.f;
	float spawnZ = 50.f;
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
	int32 GetDefaultMapId() const { return 1; }
	const MapConfig* GetMapConfig(int32 mapId) const;

	// [Getter] 데이터 조회 (없으면 nullptr 반환)
	const Protocol::StatTemplateInfo* GetStatTemplate(int32 level);
	const Protocol::ItemTemplateInfo* GetItemTemplate(int32 templateId);

	//  스킬 정보 조회
	const Protocol::SkillTemplateInfo* GetSkillTemplate(int32 skillId);
private:

	DataManager();                // 생성 시 맵 등록
	void InitMapRegistry();


	// 빠른 검색을 위한 Map (Key: ID, Value: Data)
	std::map<int32, Protocol::StatTemplateInfo> _statTemplates;
	std::map<int32, Protocol::ItemTemplateInfo> _itemTemplates;

	// [New] 스킬 데이터 저장소
	std::map<int32, Protocol::SkillTemplateInfo> _skillTemplates;

	std::map<int32, MapConfig> _mapConfigs;
};