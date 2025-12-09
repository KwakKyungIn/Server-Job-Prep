#include "pch.h"
#include "DataManager.h"

void DataManager::LoadFromPacket(const Protocol::S2S_RES_LOAD_GAME_DATA& pkt)
{
	_statTemplates.clear();
	_itemTemplates.clear();
	_skillTemplates.clear(); // [New] 초기화

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

	// 3. [New] Skill Template 로딩
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

// [New] 스킬 정보 조회 구현
const Protocol::SkillTemplateInfo* DataManager::GetSkillTemplate(int32 skillId)
{
	auto it = _skillTemplates.find(skillId);
	if (it == _skillTemplates.end())
		return nullptr;
	return &(it->second);
}