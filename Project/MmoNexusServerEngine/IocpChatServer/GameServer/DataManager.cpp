#include "pch.h"
#include "DataManager.h"

void DataManager::LoadFromPacket(const Protocol::S2S_RES_LOAD_GAME_DATA& pkt)
{
	// 기존 데이터 클리어 (리로드 대비)
	_statTemplates.clear();
	_itemTemplates.clear();

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

	std::cout << "📚 [DataManager] Load Complete. Stats: " << _statTemplates.size()
		<< ", Items: " << _itemTemplates.size() << std::endl;
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