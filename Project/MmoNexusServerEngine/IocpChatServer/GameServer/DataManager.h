#pragma once
#include "Protocol.pb.h"
#include "Protocol_S2S.pb.h"

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

	// [Getter] 데이터 조회 (없으면 nullptr 반환)
	const Protocol::StatTemplateInfo* GetStatTemplate(int32 level);
	const Protocol::ItemTemplateInfo* GetItemTemplate(int32 templateId);

private:
	// 빠른 검색을 위한 Map (Key: ID, Value: Data)
	std::map<int32, Protocol::StatTemplateInfo> _statTemplates;
	std::map<int32, Protocol::ItemTemplateInfo> _itemTemplates;
};