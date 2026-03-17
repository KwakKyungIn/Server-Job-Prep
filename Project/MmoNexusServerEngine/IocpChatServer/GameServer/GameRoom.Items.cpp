#include "pch.h"
#include "GameRoom.h"
#include "Player.h"
#include "PlayerSession.h"
#include "Monster.h"
#include "DataManager.h"
#include "ExperimentUtils.h"
#include "RoomManager.h"
#include "GameRoom.Net.h"
#include "PersistenceService.h"
#include "GameItemUidGen.h"
#include "GameMetrics.h"
#include <limits>
#include <random>

// 인벤토리에서 비어있는 슬롯 인덱스를 찾는 헬퍼 함수
// O(N)이지만 슬롯 개수가 적어서(20~30개) 성능 이슈 없음
static int32 FindEmptySlot(const Vector<Protocol::ItemInfo>& items, int32 maxSlots)
{
	// 사용 중인 슬롯 마킹
	Vector<uint8> used(maxSlots, 0);
	for (const auto& it : items)
	{
		if (it.slot() >= 0 && it.slot() < maxSlots)
			used[it.slot()] = 1;
	}
	// 빈 곳 찾아서 리턴
	for (int32 i = 0; i < maxSlots; i++)
		if (used[i] == 0)
			return i;
	return -1;
}

// 경험치 획득 및 레벨업 처리
// 한 번에 엄청난 경험치를 얻어서 2레벨 이상 오르는 경우(폭업)도 고려해야 함
static void AddExpAndLevelUp(PlayerRef player, int64 addExp)
{
	Protocol::StatInfo* stat = player->GetStatInfo();
	if (stat == nullptr) return;

	stat->set_totalexp(stat->totalexp() + addExp);

	// 레벨업 루프: 경험치가 다음 레벨 요구치보다 적을 때까지 반복
	while (true)
	{
		int32 curLv = stat->level();
		const Protocol::StatTemplateInfo* nextTpl = DataManager::Instance()->GetStatTemplate(curLv + 1);

		// 만렙이거나 데이터가 없으면 중단
		if (nextTpl == nullptr)
			break;

		// 아직 레벨업 조건 충족 못함
		if (stat->totalexp() < nextTpl->totalexp())
			break;

		// 레벨업!
		stat->set_level(curLv + 1);

		// 스탯 재계산 (힘, 민첩 등에 따른 공격력/방어력 갱신)
		player->RefreshStats();

		// 레벨업 시 HP/MP 풀회복 (국룰 보너스)
		stat->set_hp(stat->maxhp());
	}
}

// ============================================================
// [장비 슬롯 정책]
// 기획 데이터 상의 아이템 ID 대역폭으로 장착 부위를 결정함
//  - 1000~1999 : 무기
//  - 2000~2999 : 갑옷
//  - 4000~4999 : 투구
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
// 아이템 검색 헬퍼들
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

// 겹칠 수 있는 아이템인지 확인 (소모품 등)
static bool IsStackableTemplate(int32 templateId)
{
	const Protocol::ItemTemplateInfo* tpl = DataManager::Instance()->GetItemTemplate(templateId);
	if (tpl == nullptr)
		return false;
	const Protocol::ItemType itemType = static_cast<Protocol::ItemType>(tpl->itemtype());
	return (itemType == Protocol::ITEM_TYPE_CONSUMABLE);
}

struct DropRollResult
{
	int32 itemId = 0;
	int32 count = 0;
};

static int32 RandRange(int32 minValue, int32 maxValue)
{
	if (minValue >= maxValue) return minValue;
	thread_local std::mt19937 rng{ std::random_device{}() };
	std::uniform_int_distribution<int32> dist(minValue, maxValue);
	return dist(rng);
}

static void RollDropGroup(const DropGroup* group, Vector<DropRollResult>& outDrops)
{
	if (group == nullptr) return;
	if (group->totalWeight <= 0 || group->rolls <= 0) return;

	for (int32 i = 0; i < group->rolls; ++i)
	{
		int32 r = RandRange(1, group->totalWeight);
		if (r <= group->noDropWeight)
			continue;

		r -= group->noDropWeight;

		for (const auto& e : group->entries)
		{
			if (r <= e.weight)
			{
				const int32 count = RandRange(e.minCount, e.maxCount);
				if (count > 0)
					outDrops.push_back({ e.itemId, count });
				break;
			}
			r -= e.weight;
		}
	}
}

static int32 TryAutoLootItem(PlayerRef player, int32 templateId, int32 count)
{
	if (!player || count <= 0)
		return 0;

	const Protocol::ItemTemplateInfo* tpl = DataManager::Instance()->GetItemTemplate(templateId);
	if (tpl == nullptr || static_cast<Protocol::ItemType>(tpl->itemtype()) == Protocol::ITEM_TYPE_NONE)
		return 0;

	auto& items = player->GetItems();
	const int32 maxSlots = 24;
	const bool stackable = IsStackableTemplate(templateId);

	if (stackable)
	{
		auto stackIt = std::find_if(items.begin(), items.end(),
			[&](const Protocol::ItemInfo& it)
			{
				return it.templateid() == templateId && it.isequipped() == false;
			});

		if (stackIt != items.end())
		{
			stackIt->set_count(stackIt->count() + count);

			Persistence::PersistenceService::I().UpdateInventoryItem(
				player->GetPlayerId(),
				stackIt->itemuid(),
				stackIt->templateid(),
				stackIt->slot(),
				stackIt->count(),
				stackIt->isequipped(),
				true
			);

			Protocol::S_CHANGE_ITEM ch;
			ch.mutable_item()->CopyFrom(*stackIt);
			SendToPlayer(player->GetPlayerId(), ClientPacketHandler::MakeSendBuffer(ch));

			return count;
		}
	}

	int32 remaining = count;
	while (remaining > 0)
	{
		int32 emptySlot = FindEmptySlot(items, maxSlots);
		if (emptySlot < 0)
			break;

		const int32 addCount = stackable ? remaining : 1;

		Protocol::ItemInfo newItem;
		newItem.set_itemuid(GameItemUidGen::Alloc());
		newItem.set_templateid(templateId);
		newItem.set_count(addCount);
		newItem.set_slot(emptySlot);
		newItem.set_isequipped(false);

		items.push_back(newItem);

		Persistence::PersistenceService::I().UpdateInventoryItem(
			player->GetPlayerId(),
			newItem.itemuid(),
			newItem.templateid(),
			newItem.slot(),
			newItem.count(),
			newItem.isequipped(),
			true
		);

		Protocol::S_CHANGE_ITEM ch;
		ch.mutable_item()->CopyFrom(newItem);
		SendToPlayer(player->GetPlayerId(), ClientPacketHandler::MakeSendBuffer(ch));

		remaining -= addCount;
	}

	return count - remaining;
}

// [아이템 사용] 포션 마시기 등
void GameRoom::HandleUseItem(PlayerSessionRef session, PlayerRef player, Protocol::C_USE_ITEM pkt)
{

	if (player == nullptr) return;

	const uint64 playerId = player->GetPlayerId();
	if (_players.find(playerId) == _players.end()) return;

	// [중요] 거래 중에는 아이템 사용 금지
	// 거래 창에 올린 아이템을 먹어버리면 아이템 복사나 증발 버그가 터질 수 있음
	if (player->ActiveTradeId_ActorOnly() != 0)
	{
		// 필요하다면 실패 패킷 전송
		return;
	}

	// 1. 인벤토리에서 아이템 찾기 (UID 검증)
	auto& items = player->GetItems();
	auto it = std::find_if(items.begin(), items.end(),
		[&](const Protocol::ItemInfo& item) { return item.itemuid() == pkt.itemuid(); });

	if (it == items.end())
		return; // 해킹 의심: 없는 아이템 사용 시도

	// 2. 아이템 데이터 조회 및 타입 검증
	const Protocol::ItemTemplateInfo* tpl = DataManager::Instance()->GetItemTemplate(it->templateid());
	if (tpl == nullptr) return;

	const Protocol::ItemType itemType = static_cast<Protocol::ItemType>(tpl->itemtype());

	// 현재는 소모품만 사용 가능 (장비는 Equip 패킷으로 처리)
	if (itemType != Protocol::ITEM_TYPE_CONSUMABLE)
		return;

	if (it->count() <= 0)
		return;

	// 3. 효과 적용 (HP 회복)
	const int32 heal = tpl->hpbonus();
	if (heal <= 0)
		return;

	Protocol::StatInfo* stat = player->GetStatInfo();
	if (stat == nullptr) return;

	const bool wasDead = (stat->hp() <= 0);

	// 이미 풀피면 아까우니까 사용 안 함
	if (stat->hp() >= stat->maxhp())
		return;

	// HP 갱신
	const int32 newHp = min(stat->hp() + heal, stat->maxhp());
	stat->set_hp(newHp);

	// 죽었다가 회복되면 부활 상태 전파
	if (wasDead && newHp > 0)
	{
		auto pos = player->GetPosInfo();
		if (pos)
		{
			pos->set_actionstate(Protocol::ACTION_IDLE);
			pos->set_state(Protocol::MOVE_IDLE);
		}

		Protocol::S_MOVE movePkt;
		movePkt.set_objectid(playerId);
		if (pos)
			*movePkt.mutable_posinfo() = *pos;

		SendBufferRef moveSb = ClientPacketHandler::MakeSendBuffer(movePkt);
		const bool roomWideBaseline = ExperimentUtils::IsHotRoomRoomWideBaseline();
		const int32 moveRecipients = roomWideBaseline
			? Broadcast(moveSb)
			: BroadcastToZone(moveSb, player->GetZoneIndex());
		GameMetrics::OnBroadcastRecipients(
			GameMetrics::HotRoomBroadcastKind::Move,
			roomWideBaseline ? GameMetrics::HotRoomBroadcastMode::Room : GameMetrics::HotRoomBroadcastMode::Aoi,
			static_cast<std::size_t>(moveRecipients));

		Protocol::S_CHANGE_HP hpPkt;
		hpPkt.set_objectid(playerId);
		hpPkt.set_attackerid(0);
		hpPkt.set_currenthp(newHp);
		hpPkt.set_damage(0);
		SendBufferRef hpSb = ClientPacketHandler::MakeSendBuffer(hpPkt);
		const int32 hpRecipients = roomWideBaseline
			? Broadcast(hpSb)
			: BroadcastToZone(hpSb, player->GetZoneIndex());
		GameMetrics::OnBroadcastRecipients(
			GameMetrics::HotRoomBroadcastKind::Hp,
			roomWideBaseline ? GameMetrics::HotRoomBroadcastMode::Room : GameMetrics::HotRoomBroadcastMode::Aoi,
			static_cast<std::size_t>(hpRecipients));
	}

	// DB/Redis에 변경된 HP 즉시 저장 (Write-Back)
	Persistence::PersistenceService::I().UpdatePlayerCore(
		playerId,
		stat->level(),
		stat->hp(),
		stat->totalexp(),
		true
	);

	// 4. 아이템 개수 차감
	it->set_count(it->count() - 1);

	// 개수가 0이 되면 인벤토리에서 삭제, 남았으면 업데이트
	if (it->count() <= 0)
	{
		const uint64 removedUid = it->itemuid();
		items.erase(it);

		// DB에서도 삭제 처리
		Persistence::PersistenceService::I().RemoveInventoryItem(playerId, removedUid, true);

		Protocol::S_REMOVE_ITEM rm;
		rm.set_itemuid(removedUid);
		session->Send(ClientPacketHandler::MakeSendBuffer(rm));
	}
	else
	{
		Protocol::S_CHANGE_ITEM ch;
		ch.mutable_item()->CopyFrom(*it);

		// DB 업데이트
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

	// 5. 변경된 스탯(HP) 클라에 전송
	{
		Protocol::S_CHANGE_STAT st;
		st.mutable_statinfo()->CopyFrom(*stat);
		session->Send(ClientPacketHandler::MakeSendBuffer(st));
	}
}

// 아이템 사용 패킷 핸들러 라우터
void GameRoom::HandleUseItemById(PlayerSessionRef session, uint64 playerId, Protocol::C_USE_ITEM pkt)
{
	auto itPlayer = _players.find(playerId);
	if (itPlayer == _players.end())
		return;

	PlayerRef player = itPlayer->second;
	if (!player) return;

	// 거래 중 차단 (이중 방어)
	if (player->ActiveTradeId_ActorOnly() != 0)
		return;

	HandleUseItem(session, player, pkt);
}

// [장비 장착/해제]
void GameRoom::HandleEquipItemById(PlayerSessionRef session, uint64 playerId, Protocol::C_EQUIP_ITEM pkt)
{
	auto itPlayer = _players.find(playerId);
	if (itPlayer == _players.end())
		return;

	PlayerRef player = itPlayer->second;
	if (!player) return;

	// 거래 중에는 장비 변경 불가 (스탯 변동 및 아이템 상태 보호)
	if (player->ActiveTradeId_ActorOnly() != 0)
	{
		// TODO: 실패 메시지 전송
		return;
	}

	// 1. 대상 아이템 찾기
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
		return; // 해킹 의심: 없는 아이템

	const EquipSlot targetSlot = GetEquipSlotFromTemplate(targetItem->templateid());

	// 2. 장착 요청 (Equip)
	if (pkt.equip())
	{
		// 장착 가능한 부위인지 확인
		if (targetSlot == EquipSlot::None)
			return;

		// 이미 장착 중이면 패스
		if (targetItem->isequipped())
			return;

		// [교체 로직] 같은 부위에 이미 다른 아이템을 끼고 있다면? -> 자동 해제(Unequip)
		for (auto& item : items)
		{
			if (!item.isequipped())
				continue;

			// 자기 자신은 제외
			if (item.itemuid() == targetItem->itemuid())
				continue;

			// 같은 슬롯 타입인지 확인 (예: 무기 자리에 다른 무기가 있는지)
			if (GetEquipSlotFromTemplate(item.templateid()) != targetSlot)
				continue;

			// 기존 장비 해제 처리
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

			// 클라에게 "기존 장비 벗겨짐" 알림
			if (session)
			{
				Protocol::S_EQUIP_ITEM resOther;
				resOther.set_itemuid(item.itemuid());
				resOther.set_equipped(false);
				resOther.set_slotindex(item.slot());
				session->Send(ClientPacketHandler::MakeSendBuffer(resOther));
			}
		}

		// 새 장비 장착
		targetItem->set_isequipped(true);
	}
	else
	{
		// 3. 해제 요청 (Unequip)
		if (!targetItem->isequipped())
			return; // 이미 해제된 상태

		targetItem->set_isequipped(false);
	}

	// 4. DB 저장 (상태 변경)
	Persistence::PersistenceService::I().UpdateInventoryItem(
		playerId,
		targetItem->itemuid(),
		targetItem->templateid(),
		targetItem->slot(),
		targetItem->count(),
		targetItem->isequipped(),
		true
	);

	// 5. 스탯 재계산 (장비 옵션 반영)
	player->RefreshStats();

	// 6. 결과 전송
	if (session)
	{
		Protocol::S_EQUIP_ITEM res;
		res.set_itemuid(targetItem->itemuid());
		res.set_equipped(targetItem->isequipped());
		res.set_slotindex(targetItem->slot());
		session->Send(ClientPacketHandler::MakeSendBuffer(res));
	}

	// 7. 변경된 스탯 정보 전송
	if (session)
	{
		Protocol::S_CHANGE_STAT st;
		st.mutable_statinfo()->CopyFrom(*player->GetStatInfo());
		session->Send(ClientPacketHandler::MakeSendBuffer(st));
	}
}

// ============================================================
// [인벤토리 드래그 & 드롭]
// 아이템 이동, 병합, 스왑을 처리하는 복합 로직
//  - 빈 곳으로 이동 -> 단순 슬롯 변경
//  - 있는 곳으로 이동
//      * 같은 종류 + 겹치기 가능 -> 개수 합치기(Merge)
//      * 다른 종류 -> 위치 맞교환(Swap)
// ============================================================
void GameRoom::HandleInvDragDrop(PlayerSessionRef session, PlayerRef player, Protocol::C_INV_DRAG_DROP pkt)
{
	if (!player) return;
	if (!session) return;

	const uint64 playerId = player->GetPlayerId();
	if (_players.find(playerId) == _players.end()) return;

	// 거래 중 인벤토리 조작 금지 (절대 원칙)
	if (player->ActiveTradeId_ActorOnly() != 0)
		return;

	const int32 maxSlots = 24;
	const int32 fromSlot = pkt.fromslot();
	const int32 toSlot = pkt.toslot();
	const uint64 itemUid = pkt.itemuid();

	// 유효성 검사 (범위, 자기 자신으로 이동 등)
	if (itemUid == 0) return;
	if (fromSlot < 0 || fromSlot >= maxSlots) return;
	if (toSlot < 0 || toSlot >= maxSlots) return;
	if (fromSlot == toSlot) return;

	auto& items = player->GetItems();

	Protocol::ItemInfo* src = FindItemByUid(items, itemUid);
	if (!src) return;

	// 클라가 보낸 슬롯 정보와 서버 데이터가 일치하는지 확인 (검증)
	if (src->slot() != fromSlot) return;
	// 장착 중인 아이템은 이동 불가
	if (src->isequipped()) return;

	// 목적지에 아이템이 있는지 확인
	Protocol::ItemInfo* dst = FindItemBySlot(items, toSlot);

	// 1. 빈 슬롯으로 이동 (Move)
	if (dst == nullptr)
	{
		src->set_slot(toSlot);

		// DB 업데이트
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

	// 2. 이미 아이템이 있는 곳으로 이동 (Merge or Swap)

	// 장착 중인 아이템과 스왑 불가
	if (dst->isequipped())
		return;
	// 같은 아이템끼리 비비기 방지
	if (dst->itemuid() == src->itemuid())
		return;

	// 2-A. 합치기 (Merge): 템플릿 ID 같고, 스택 가능하면
	if (src->templateid() == dst->templateid() && IsStackableTemplate(src->templateid()))
	{
		const int64 merged = static_cast<int64>(dst->count()) + static_cast<int64>(src->count());

		// 오버플로우 방지 (int32 범위 체크)
		if (merged > (std::numeric_limits<int32>::max)())
			return;

		// 목적지에 개수 몰아주기
		dst->set_count(static_cast<int32>(merged));

		// 소스 아이템 삭제
		const uint64 removedUid = src->itemuid();
		auto itErase = std::find_if(items.begin(), items.end(),
			[&](const Protocol::ItemInfo& it) { return it.itemuid() == removedUid; });
		if (itErase != items.end())
			items.erase(itErase);

		// DB: 소스 삭제 + 목적지 업데이트 (트랜잭션으로 묶으면 더 좋음)
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

		// 클라 통보: 소스 삭제, 목적지 변경
		Protocol::S_REMOVE_ITEM rm;
		rm.set_itemuid(removedUid);
		session->Send(ClientPacketHandler::MakeSendBuffer(rm));

		Protocol::S_CHANGE_ITEM ch;
		ch.mutable_item()->CopyFrom(*dst);
		session->Send(ClientPacketHandler::MakeSendBuffer(ch));
		return;
	}

	// 2-B. 맞교환 (Swap): 서로 다른 아이템 위치 바꾸기
	{
		const int32 srcSlot = src->slot();
		const int32 dstSlot = dst->slot();

		// 슬롯 번호 교체
		src->set_slot(dstSlot);
		dst->set_slot(srcSlot);

		// 둘 다 DB 업데이트
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

		// 클라에 각각 변경 정보 전송
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

	// 거래 중 조작 차단
	if (player->ActiveTradeId_ActorOnly() != 0)
		return;

	HandleInvDragDrop(session, player, pkt);
}

// [몬스터 사망 처리] 경험치 지급 및 드랍
void GameRoom::HandleMonsterDead(std::shared_ptr<Creature> attacker, MonsterRef monster)
{
	if (monster == nullptr) return;
	// 다른 방의 몬스터가 넘어왔을 리 없지만 체크
	if (monster->GetRoom().get() != this) return;

	const uint64 monsterId = monster->GetObjectId();
	if (_monsters.find(monsterId) == _monsters.end())
	{
		// 이미 처리되었거나(중복 사망) 없는 몬스터
		return;
	}

	// 1. 막타 친 유저(Killer) 확인
	PlayerRef killer = nullptr;
	if (attacker && attacker->GetObjectType() == Protocol::OBJECT_TYPE_PLAYER)
		killer = std::static_pointer_cast<Player>(attacker);

	const int32 monsterTemplateId = monster->GetMonsterInfo()->templateid();
	const MonsterTemplate* monsterTpl = DataManager::Instance()->GetMonsterTemplate(monsterTemplateId);

	// 2. 경험치 지급
	if (killer && monsterTpl && monsterTpl->exp > 0)
	{
		const int64 exp = monsterTpl->exp;
		AddExpAndLevelUp(killer, exp);

		// 경험치 획득 정보 저장
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

		// UI 갱신용 패킷
		Protocol::S_CHANGE_STAT st;
		st.mutable_statinfo()->CopyFrom(*killer->GetStatInfo());
		SendToPlayer(killer->GetPlayerId(), ClientPacketHandler::MakeSendBuffer(st));
	}

	// 3. 아이템 드랍 (Auto-Loot 기본, 바닥 드랍은 추후 확장)
	if (killer && monsterTpl && monsterTpl->dropGroupId > 0)
	{
		const DropGroup* group = DataManager::Instance()->GetDropGroup(monsterTpl->dropGroupId);
		Vector<DropRollResult> drops;
		RollDropGroup(group, drops);

		for (const auto& d : drops)
		{
			const int32 added = TryAutoLootItem(killer, d.itemId, d.count);
			if (added < d.count)
			{
				// TODO: 바닥 드랍 시스템 추가 시 여기서 월드 아이템 생성
			}
		}
	}

	// 4. 몬스터 소멸 (Despawn)
	// 월드에서 지우고 리스폰 타이머 돌림
	OnMonsterDespawned_ActorOnly(monster);
	LeaveMonster(monsterId);
}
