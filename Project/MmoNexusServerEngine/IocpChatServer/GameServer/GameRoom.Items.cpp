#include "pch.h"
#include "GameRoom.h"
#include "Player.h"
#include "PlayerSession.h"
#include "Monster.h"
#include "DataManager.h"
#include "RoomManager.h"
#include "GameRoom.Net.h"
#include "PersistenceService.h"
#include "GameItemUidGen.h"
#include <limits>

static int32 FindEmptySlot(const Vector<Protocol::ItemInfo>& items, int32 maxSlots)
{
	Vector<bool> used(maxSlots, false);
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

	// ������: "���� ���� ���ø��� totalExp"�� �޼� �������� ���
	while (true)
	{
		int32 curLv = stat->level();
		const Protocol::StatTemplateInfo* nextTpl = DataManager::Instance()->GetStatTemplate(curLv + 1);
		if (nextTpl == nullptr)
			break;

		if (stat->totalexp() < nextTpl->totalexp())
			break;

		stat->set_level(curLv + 1);

		// ���� �ٲ������ ���� ��������
		player->RefreshStats();

		// �������ϸ� Ǯ�Ƿ� (���ϸ� ���� ������ �ٲ㵵 ��)
		stat->set_hp(stat->maxhp());
	}
}

// ============================================================
// Equipment Slot Policy (templateId range based)
//  - 1000~1999 : Weapon
//  - 2000~2999 : Body
//  - 4000~4999 : Head
// ============================================================
enum class EquipSlot : int32
{
	None = 0,
	Weapon = 1,
	Body = 2,
	Head = 3,
};

static EquipSlot GetEquipSlotFromTemplate(int32 templateId)
{
	if (templateId >= 1000 && templateId < 2000) return EquipSlot::Weapon;
	if (templateId >= 2000 && templateId < 3000) return EquipSlot::Body;
	if (templateId >= 4000 && templateId < 5000) return EquipSlot::Head;
	return EquipSlot::None;
}

// ============================================================
// Inventory helpers (move / swap / merge)
// ============================================================
static Protocol::ItemInfo* FindItemByUid(Vector<Protocol::ItemInfo>& items, uint64 uid)
{
	for (auto& it : items)
	{
		if (it.itemuid() == uid)
			return &it;
	}
	return nullptr;
}

static Protocol::ItemInfo* FindItemBySlot(Vector<Protocol::ItemInfo>& items, int32 slot)
{
	for (auto& it : items)
	{
		if (it.slot() == slot)
			return &it;
	}
	return nullptr;
}

static bool IsStackableTemplate(int32 templateId)
{
	const Protocol::ItemTemplateInfo* tpl = DataManager::Instance()->GetItemTemplate(templateId);
	if (tpl == nullptr)
		return false;
	const Protocol::ItemType itemType = static_cast<Protocol::ItemType>(tpl->itemtype());
	return (itemType == Protocol::ITEM_TYPE_CONSUMABLE);
}

void GameRoom::HandleUseItem(PlayerSessionRef session, PlayerRef player, Protocol::C_USE_ITEM pkt)
{

	if (player == nullptr) return;

	const uint64 playerId = player->GetPlayerId();
	if (_players.find(playerId) == _players.end()) return;

	// [Trade] trading -> block item actions (to avoid desync)
	if (player->ActiveTradeId_ActorOnly() != 0)
	{
		// TODO: send item action fail message if needed
		return;
	}

	// �κ����� ������ ã��
	auto& items = player->GetItems();
	auto it = std::find_if(items.begin(), items.end(),
		[&](const Protocol::ItemInfo& item) { return item.itemuid() == pkt.itemuid(); });

	if (it == items.end())
		return;

	// ���ø� ����
	const Protocol::ItemTemplateInfo* tpl = DataManager::Instance()->GetItemTemplate(it->templateid());
	if (tpl == nullptr) return;

	const Protocol::ItemType itemType = static_cast<Protocol::ItemType>(tpl->itemtype());
	if (itemType != Protocol::ITEM_TYPE_CONSUMABLE)
		return;

	if (it->count() <= 0)
		return;

	// ����(������ hp_bonus�� ó��)
	const int32 heal = tpl->hpbonus();
	if (heal <= 0)
		return;

	Protocol::StatInfo* stat = player->GetStatInfo();
	if (stat == nullptr) return;

	// Ǯ�Ǹ� �Һ� �� �ϰ�(��õ)
	if (stat->hp() >= stat->maxhp())
		return;

	// 1) HP ����
	const int32 newHp = min(stat->hp() + heal, stat->maxhp());
	stat->set_hp(newHp);

	// ===== Persistence (HP dirty) =====
	Persistence::PersistenceService::I().UpdatePlayerCore(
		playerId,
		stat->level(),
		stat->hp(),
		stat->totalexp(),
		true
	);

	// 2) ������ ī��Ʈ ����
	it->set_count(it->count() - 1);

	// 3) ������ ��Ŷ(����/����)
	if (it->count() <= 0)
	{
		const uint64 removedUid = it->itemuid();
		items.erase(it);

		// ===== Persistence (item removed tombstone + inv dirty) =====
		Persistence::PersistenceService::I().RemoveInventoryItem(playerId, removedUid, true);

		Protocol::S_REMOVE_ITEM rm;
		rm.set_itemuid(removedUid);
		session->Send(ClientPacketHandler::MakeSendBuffer(rm));
	}
	else
	{
		Protocol::S_CHANGE_ITEM ch;
		ch.mutable_item()->CopyFrom(*it);

		Persistence::PersistenceService::I().UpdateInventoryItem(
			playerId,
			it->itemuid(),
			it->templateid(),
			it->slot(),
			it->count(),
			it->isequipped(),
			true
		);

		session->Send(ClientPacketHandler::MakeSendBuffer(ch));
	}

	// 4) ���� ��Ŷ
	{
		Protocol::S_CHANGE_STAT st;
		st.mutable_statinfo()->CopyFrom(*stat);
		session->Send(ClientPacketHandler::MakeSendBuffer(st));
	}

	// TODO: DB �ݿ�(S2S ������ count ������Ʈ, hp ���� ��å)
}

void GameRoom::HandleUseItemById(PlayerSessionRef session, uint64 playerId, Protocol::C_USE_ITEM pkt)
{
	auto itPlayer = _players.find(playerId);
	if (itPlayer == _players.end())
		return;

	PlayerRef player = itPlayer->second;
	if (!player) return;

	// [Trade] trading -> block item actions (to avoid desync)
	if (player->ActiveTradeId_ActorOnly() != 0)
		return;

	HandleUseItem(session, player, pkt);
}

void GameRoom::HandleEquipItemById(PlayerSessionRef session, uint64 playerId, Protocol::C_EQUIP_ITEM pkt)
{
	auto itPlayer = _players.find(playerId);
	if (itPlayer == _players.end())
		return;

	PlayerRef player = itPlayer->second;
	if (!player) return;

	// [Trade] trading -> block item actions (to avoid desync)
	if (player->ActiveTradeId_ActorOnly() != 0)
	{
		// TODO: send item action fail message if needed
		return;
	}

	// 1) 대상 아이템 찾기 (UID 우선)
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

	const EquipSlot targetSlot = GetEquipSlotFromTemplate(targetItem->templateid());

	// 2) Equip 요청일 때만 "3슬롯 정책" 적용
	if (pkt.equip())
	{
		// 지정된 장비 슬롯(1000/2000/4000대)이 아니면 장착 불가
		if (targetSlot == EquipSlot::None)
			return;

		// 이미 장착 중이면 그냥 반환(중복 요청)
		if (targetItem->isequipped())
			return;

		// 같은 슬롯(Weapon/Body/Head)에 다른 아이템이 장착되어 있으면 자동 해제
		for (auto& item : items)
		{
			if (!item.isequipped())
				continue;

			if (item.itemuid() == targetItem->itemuid())
				continue;

			if (GetEquipSlotFromTemplate(item.templateid()) != targetSlot)
				continue;

			//  auto-unequip
			item.set_isequipped(false);

			Persistence::PersistenceService::I().UpdateInventoryItem(
				playerId,
				item.itemuid(),
				item.templateid(),
				item.slot(),
				item.count(),
				item.isequipped(),
				true
			);

			// 클라 동기화(unequip)
			if (session)
			{
				Protocol::S_EQUIP_ITEM resOther;
				resOther.set_itemuid(item.itemuid());
				resOther.set_equipped(false);
				resOther.set_slotindex(item.slot()); // inventory slot
				session->Send(ClientPacketHandler::MakeSendBuffer(resOther));
			}
		}

		//  target equip
		targetItem->set_isequipped(true);
	}
	else
	{
		// Unequip: 장착 중이 아니면 무시
		if (!targetItem->isequipped())
			return;

		targetItem->set_isequipped(false);
	}

	// 3) Persistence
	Persistence::PersistenceService::I().UpdateInventoryItem(
		playerId,
		targetItem->itemuid(),
		targetItem->templateid(),
		targetItem->slot(),
		targetItem->count(),
		targetItem->isequipped(),
		true
	);

	// 4) Stat refresh
	player->RefreshStats();

	// 5) 장착 결과
	if (session)
	{
		Protocol::S_EQUIP_ITEM res;
		res.set_itemuid(targetItem->itemuid());
		res.set_equipped(targetItem->isequipped());
		res.set_slotindex(targetItem->slot()); // inventory slot (클라 캐시 반영용)
		session->Send(ClientPacketHandler::MakeSendBuffer(res));
	}

	// 6) 스탯 갱신
	if (session)
	{
		Protocol::S_CHANGE_STAT st;
		st.mutable_statinfo()->CopyFrom(*player->GetStatInfo());
		session->Send(ClientPacketHandler::MakeSendBuffer(st));
	}

	// TODO: 장비 종류별(무기/방어구/머리) 추가 효과, 외형 동기화, 장비 프리셋 등
}

// ============================================================
// Inventory drag & drop (Move / Swap / Merge)
//  - Move to empty: src.slot = toSlot
//  - Drop on occupied:
//      * same template + stackable(consumable) => merge counts into dst, remove src
//      * otherwise => swap slot index
// ============================================================
void GameRoom::HandleInvDragDrop(PlayerSessionRef session, PlayerRef player, Protocol::C_INV_DRAG_DROP pkt)
{
	if (!player) return;
	if (!session) return;

	const uint64 playerId = player->GetPlayerId();
	if (_players.find(playerId) == _players.end()) return;

	// [Trade] block inventory mutation while trading
	if (player->ActiveTradeId_ActorOnly() != 0)
		return;

	const int32 maxSlots = 24;
	const int32 fromSlot = pkt.fromslot();
	const int32 toSlot = pkt.toslot();
	const uint64 itemUid = pkt.itemuid();

	if (itemUid == 0) return;
	if (fromSlot < 0 || fromSlot >= maxSlots) return;
	if (toSlot < 0 || toSlot >= maxSlots) return;
	if (fromSlot == toSlot) return;

	auto& items = player->GetItems();

	Protocol::ItemInfo* src = FindItemByUid(items, itemUid);
	if (!src) return;
	// stale / tampered client (slot mismatch)
	if (src->slot() != fromSlot) return;
	if (src->isequipped()) return;

	Protocol::ItemInfo* dst = FindItemBySlot(items, toSlot);

	// 1) Move to empty
	if (dst == nullptr)
	{
		src->set_slot(toSlot);

		Persistence::PersistenceService::I().UpdateInventoryItem(
			playerId,
			src->itemuid(),
			src->templateid(),
			src->slot(),
			src->count(),
			src->isequipped(),
			true
		);

		Protocol::S_CHANGE_ITEM ch;
		ch.mutable_item()->CopyFrom(*src);
		session->Send(ClientPacketHandler::MakeSendBuffer(ch));
		return;
	}

	// 2) Drop on occupied
	if (dst->isequipped())
		return;
	if (dst->itemuid() == src->itemuid())
		return;

	// 2-A) Merge (only stackable templates)
	if (src->templateid() == dst->templateid() && IsStackableTemplate(src->templateid()))
	{
		const int64 merged = static_cast<int64>(dst->count()) + static_cast<int64>(src->count());
		if (merged > (std::numeric_limits<int32>::max)())
			return;

		dst->set_count(static_cast<int32>(merged));

		// remove src item
		const uint64 removedUid = src->itemuid();
		auto itErase = std::find_if(items.begin(), items.end(),
			[&](const Protocol::ItemInfo& it) { return it.itemuid() == removedUid; });
		if (itErase != items.end())
			items.erase(itErase);

		Persistence::PersistenceService::I().RemoveInventoryItem(playerId, removedUid, true);
		Persistence::PersistenceService::I().UpdateInventoryItem(
			playerId,
			dst->itemuid(),
			dst->templateid(),
			dst->slot(),
			dst->count(),
			dst->isequipped(),
			true
		);

		Protocol::S_REMOVE_ITEM rm;
		rm.set_itemuid(removedUid);
		session->Send(ClientPacketHandler::MakeSendBuffer(rm));

		Protocol::S_CHANGE_ITEM ch;
		ch.mutable_item()->CopyFrom(*dst);
		session->Send(ClientPacketHandler::MakeSendBuffer(ch));
		return;
	}

	// 2-B) Swap slots
	{
		const int32 srcSlot = src->slot();
		const int32 dstSlot = dst->slot();
		src->set_slot(dstSlot);
		dst->set_slot(srcSlot);

		Persistence::PersistenceService::I().UpdateInventoryItem(
			playerId,
			src->itemuid(),
			src->templateid(),
			src->slot(),
			src->count(),
			src->isequipped(),
			true
		);
		Persistence::PersistenceService::I().UpdateInventoryItem(
			playerId,
			dst->itemuid(),
			dst->templateid(),
			dst->slot(),
			dst->count(),
			dst->isequipped(),
			true
		);

		Protocol::S_CHANGE_ITEM ch1;
		ch1.mutable_item()->CopyFrom(*src);
		session->Send(ClientPacketHandler::MakeSendBuffer(ch1));

		Protocol::S_CHANGE_ITEM ch2;
		ch2.mutable_item()->CopyFrom(*dst);
		session->Send(ClientPacketHandler::MakeSendBuffer(ch2));
	}
}

void GameRoom::HandleInvDragDropById(PlayerSessionRef session, uint64 playerId, Protocol::C_INV_DRAG_DROP pkt)
{
	auto itPlayer = _players.find(playerId);
	if (itPlayer == _players.end())
		return;

	PlayerRef player = itPlayer->second;
	if (!player) return;

	// [Trade] block inventory mutation while trading
	if (player->ActiveTradeId_ActorOnly() != 0)
		return;

	HandleInvDragDrop(session, player, pkt);
}

void GameRoom::HandleMonsterDead(std::shared_ptr<Creature> attacker, MonsterRef monster)
{
	if (monster == nullptr) return;
	if (monster->GetRoom().get() != this) return;

	const uint64 monsterId = monster->GetObjectId();
	if (_monsters.find(monsterId) == _monsters.end())
	{
		// �̹� ó�������� �ߺ� ����
		return;
	}

	// 1) ų���� �÷��̾����� Ȯ��
	PlayerRef killer = nullptr;
	if (attacker && attacker->GetObjectType() == Protocol::OBJECT_TYPE_PLAYER)
		killer = std::static_pointer_cast<Player>(attacker);

	// 2) ����ġ ����
	if (killer)
	{
		const int64 exp = 1000; // TODO: ���� ���ø����� �б�
		AddExpAndLevelUp(killer, exp);

		auto* stInfo = killer->GetStatInfo();
		if (stInfo)
		{
			Persistence::PersistenceService::I().UpdatePlayerCore(
				killer->GetPlayerId(),
				stInfo->level(),
				stInfo->hp(),
				stInfo->totalexp(),
				true
			);
		}

		Protocol::S_CHANGE_STAT st;
		st.mutable_statinfo()->CopyFrom(*killer->GetStatInfo());
		SendToPlayer(killer->GetPlayerId(), ClientPacketHandler::MakeSendBuffer(st));
	}

	// 3) ���(����) = ����� �κ� ���ޡ� ��� (�ٴ� ���� ������)
	// 3) ���(����) = ����� �κ� ���ޡ� ��� (���� �׽�Ʈ��)
	if (killer)
	{
		const int32 dropTemplateId = 2001;
		const int32 dropCount = 10;

		const Protocol::ItemTemplateInfo* tpl = DataManager::Instance()->GetItemTemplate(dropTemplateId);
		if (tpl == nullptr || static_cast<Protocol::ItemType>(tpl->itemtype()) == Protocol::ITEM_TYPE_NONE)
		{
			// ���ø� ������ �׳� ����
		}
		else
		{
			auto& items = killer->GetItems();
			const int32 maxSlots = 24;

			// 1) ���� ���ø� ������ ������ ���� ����(�κ� �� ���� �׽�Ʈ ����)
			auto stackIt = std::find_if(items.begin(), items.end(),
				[&](const Protocol::ItemInfo& it)
				{
					return it.templateid() == dropTemplateId && it.isequipped() == false;
				});

			if (stackIt != items.end())
			{
				stackIt->set_count(stackIt->count() + dropCount);

				// Redis dirty
				Persistence::PersistenceService::I().UpdateInventoryItem(
					killer->GetPlayerId(),
					stackIt->itemuid(),     // game_item_uid �ǹ�
					stackIt->templateid(),
					stackIt->slot(),
					stackIt->count(),
					stackIt->isequipped(),
					true
				);

				Protocol::S_CHANGE_ITEM ch;
				ch.mutable_item()->CopyFrom(*stackIt);
				SendToPlayer(killer->GetPlayerId(), ClientPacketHandler::MakeSendBuffer(ch));
			}
			else
			{
				// 2) ������ �� ���� ����
				int32 emptySlot = FindEmptySlot(items, maxSlots);
				if (emptySlot >= 0)
				{
					Protocol::ItemInfo newItem;
					newItem.set_itemuid(GameItemUidGen::Alloc()); // game_item_uid
					newItem.set_templateid(dropTemplateId);
					newItem.set_count(dropCount);
					newItem.set_slot(emptySlot);
					newItem.set_isequipped(false);

					items.push_back(newItem);

					// Redis dirty
					Persistence::PersistenceService::I().UpdateInventoryItem(
						killer->GetPlayerId(),
						newItem.itemuid(),
						newItem.templateid(),
						newItem.slot(),
						newItem.count(),
						newItem.isequipped(),
						true
					);

					Protocol::S_CHANGE_ITEM ch;
					ch.mutable_item()->CopyFrom(newItem);
					SendToPlayer(killer->GetPlayerId(), ClientPacketHandler::MakeSendBuffer(ch));
				}
				else
				{
					// �κ� �� á���� �׳� ��� ���(�׽�Ʈ�ϱ� OK)
				}
			}
		}
	}

	// 4) �������� ���� ����(��ε�ĳ��Ʈ despawn ����)
	LeaveMonster(monsterId);
}

