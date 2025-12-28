#include "pch.h"
#include "GameRoom.h"
#include "GameMap.h"
#include "Player.h"
#include "PlayerSession.h"
#include "ClientPacketHandler.h"
#include "Monster.h"
#include "DataManager.h"
#include "ObjectUtils.h"
#include "BattleSystem.h"
#include "Zone.h"
#include "Creature.h"
#include "GameSessionManager.h"
#include "RoomManager.h"
#include "GameRoom.Net.h"

static std::atomic<uint64> GItemUidGen{ 1000000 }; // 서버 발급 itemUid (DB가 identity면 나중에 정책 바꿔야 함)

static int32 FindEmptySlot(const std::vector<Protocol::ItemInfo>& items, int32 maxSlots)
{
	std::vector<bool> used(maxSlots, false);
	for (const auto& it : items)
	{
		if (it.slot() >= 0 && it.slot() < maxSlots)
			used[it.slot()] = true;
	}
	for (int32 i = 0; i < maxSlots; i++)
		if (used[i] == false)
			return i;
	return -1;
}

static void AddExpAndLevelUp(PlayerRef player, int64 addExp)
{
	Protocol::StatInfo* stat = player->GetStatInfo();
	if (stat == nullptr) return;

	stat->set_totalexp(stat->totalexp() + addExp);

	// 레벨업: "다음 레벨 템플릿의 totalExp"를 달성 조건으로 사용
	while (true)
	{
		int32 curLv = stat->level();
		const Protocol::StatTemplateInfo* nextTpl = DataManager::Instance()->GetStatTemplate(curLv + 1);
		if (nextTpl == nullptr)
			break;

		if (stat->totalexp() < nextTpl->totalexp())
			break;

		stat->set_level(curLv + 1);

		// 레벨 바뀌었으니 스탯 리프레시
		player->RefreshStats();

		// 레벨업하면 풀피로 (원하면 비율 유지로 바꿔도 됨)
		stat->set_hp(stat->maxhp());
	}
}

void GameRoom::HandleUseItem(PlayerSessionRef session, PlayerRef player, Protocol::C_USE_ITEM pkt)
{

	if (player == nullptr) return;

	const uint64 playerId = player->GetPlayerId();
	if (_players.find(playerId) == _players.end()) return;

	// 인벤에서 아이템 찾기
	auto& items = player->GetItems();
	auto it = std::find_if(items.begin(), items.end(),
		[&](const Protocol::ItemInfo& item) { return item.itemuid() == pkt.itemuid(); });

	if (it == items.end())
		return;

	// 템플릿 검증
	const Protocol::ItemTemplateInfo* tpl = DataManager::Instance()->GetItemTemplate(it->templateid());
	if (tpl == nullptr) return;

	const Protocol::ItemType itemType = static_cast<Protocol::ItemType>(tpl->itemtype());
	if (itemType != Protocol::ITEM_TYPE_CONSUMABLE)
		return;

	if (it->count() <= 0)
		return;

	// 힐량(지금은 hp_bonus로 처리)
	const int32 heal = tpl->hpbonus();
	if (heal <= 0)
		return;

	Protocol::StatInfo* stat = player->GetStatInfo();
	if (stat == nullptr) return;

	// 풀피면 소비 안 하게(추천)
	if (stat->hp() >= stat->maxhp())
		return;

	// 1) HP 적용
	const int32 newHp = min(stat->hp() + heal, stat->maxhp());
	stat->set_hp(newHp);

	// 2) 아이템 카운트 감소
	it->set_count(it->count() - 1);

	// 3) 아이템 패킷(변경/삭제)
	if (it->count() <= 0)
	{
		const uint64 removedUid = it->itemuid();
		items.erase(it);

		Protocol::S_REMOVE_ITEM rm;
		rm.set_itemuid(removedUid);
		session->Send(ClientPacketHandler::MakeSendBuffer(rm));
	}
	else
	{
		Protocol::S_CHANGE_ITEM ch;
		ch.mutable_item()->CopyFrom(*it);
		session->Send(ClientPacketHandler::MakeSendBuffer(ch));
	}

	// 4) 스탯 패킷
	{
		Protocol::S_CHANGE_STAT st;
		st.mutable_statinfo()->CopyFrom(*stat);
		session->Send(ClientPacketHandler::MakeSendBuffer(st));
	}

	// TODO: DB 반영(S2S 아이템 count 업데이트, hp 저장 정책)
}

void GameRoom::HandleUseItemById(PlayerSessionRef session, uint64 playerId, Protocol::C_USE_ITEM pkt)
{
	auto it = _players.find(playerId);
	if (it == _players.end())
		return;

	HandleUseItem(session, it->second, pkt); // 기존 로직 재사용
}

void GameRoom::HandleEquipItemById(PlayerSessionRef session, uint64 playerId, Protocol::C_EQUIP_ITEM pkt)
{
	auto it = _players.find(playerId);
	if (it == _players.end())
		return;

	PlayerRef player = it->second;
	if (!player) return;

	Protocol::ItemInfo* targetItem = nullptr;
	auto& items = player->GetItems();

	for (auto& item : items)
	{
		if (item.itemuid() == pkt.itemuid())
		{
			targetItem = &item;
			break;
		}
	}

	if (!targetItem)
		return;

	targetItem->set_isequipped(pkt.equip());
	player->RefreshStats();

	// 장착 결과
	{
		Protocol::S_EQUIP_ITEM res;
		res.set_itemuid(pkt.itemuid());
		res.set_equipped(pkt.equip());
		res.set_slotindex(pkt.slotindex());
		session->Send(ClientPacketHandler::MakeSendBuffer(res));
	}

	// 스탯 갱신
	{
		Protocol::S_CHANGE_STAT st;
		st.mutable_statinfo()->CopyFrom(*player->GetStatInfo());
		session->Send(ClientPacketHandler::MakeSendBuffer(st));
	}

	// TODO: DB 저장은 Step 뒤에서 붙이면 됨
}

void GameRoom::HandleMonsterDead(std::shared_ptr<Creature> attacker, MonsterRef monster)
{
	if (monster == nullptr) return;
	if (monster->GetRoom().get() != this) return;

	const uint64 monsterId = monster->GetObjectId();
	if (_monsters.find(monsterId) == _monsters.end())
	{
		// 이미 처리됐으면 중복 방지
		return;
	}

	// 1) 킬러가 플레이어인지 확인
	PlayerRef killer = nullptr;
	if (attacker && attacker->GetObjectType() == Protocol::OBJECT_TYPE_PLAYER)
		killer = std::static_pointer_cast<Player>(attacker);

	// 2) 경험치 지급
	if (killer)
	{
		const int64 exp = 10; // TODO: 몬스터 템플릿에서 읽기
		AddExpAndLevelUp(killer, exp);

		Protocol::S_CHANGE_STAT st;
		st.mutable_statinfo()->CopyFrom(*killer->GetStatInfo());
		SendToPlayer(killer->GetPlayerId(), ClientPacketHandler::MakeSendBuffer(st));
	}

	// 3) 드랍(루팅) = “즉시 인벤 지급” 방식 (바닥 루프 만들기용)
	if (killer)
	{
		// TODO: 드랍 테이블 생기면 여기 교체
		const int32 dropTemplateId = 103; // 예: 포션 템플릿 ID (네 ITEM_TEMPLATE에 맞춰서 바꿔)

		const Protocol::ItemTemplateInfo* tpl = DataManager::Instance()->GetItemTemplate(dropTemplateId);
		if (tpl && static_cast<Protocol::ItemType>(tpl->itemtype()) != Protocol::ITEM_TYPE_NONE)
		{
			auto& items = killer->GetItems();
			const int32 maxSlots = 24; // 너 인벤 고정이면 그대로
			int32 emptySlot = FindEmptySlot(items, maxSlots);

			if (emptySlot >= 0)
			{
				Protocol::ItemInfo newItem;
				newItem.set_itemuid(GItemUidGen.fetch_add(1));
				newItem.set_templateid(dropTemplateId);
				newItem.set_count(1);
				newItem.set_slot(emptySlot);
				newItem.set_isequipped(false);

				items.push_back(newItem);

				Protocol::S_CHANGE_ITEM ch;
				ch.mutable_item()->CopyFrom(newItem);
				SendToPlayer(killer->GetPlayerId(), ClientPacketHandler::MakeSendBuffer(ch));


				// TODO: DB 저장(S2S INSERT ITEMS)
			}
			else
			{
				// 인벤 꽉 참: 지금은 그냥 드랍 폐기 or TODO: 월드 드랍 오브젝트
			}
		}
	}

	// 4) 마지막에 몬스터 제거(브로드캐스트 despawn 포함)
	LeaveMonster(monsterId);
}

