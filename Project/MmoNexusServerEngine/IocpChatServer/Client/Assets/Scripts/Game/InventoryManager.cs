using System;
using System.Collections.Generic;
using System.Linq;
using Google.Protobuf.Collections;
using Protocol;
using UnityEngine;

public class InventoryManager
{
    private static InventoryManager _instance = new InventoryManager();
    public static InventoryManager Instance { get { return _instance; } }

    private Dictionary<int, ItemInfo> _items = new Dictionary<int, ItemInfo>();

    public Action OnInventoryUpdated;

    public void Init()
    {
        PacketHandler.OnItemList += HandleItemList;
        PacketHandler.OnUpdateItem += HandleUpdateItem;
        PacketHandler.OnRemoveItem += HandleRemoveItem;

        // ✅ [ADD] Equip/Unequip 결과도 인벤에 반영해야 UI가 바뀜
        PacketHandler.OnEquipItem += HandleEquipItem;

        Debug.Log("[InventoryManager] Initialized & Listening...");
    }

    // (선택) 네가 따로 Dispose/Reset 만들어두면 여기서 해제해라
    public void Shutdown()
    {
        PacketHandler.OnItemList -= HandleItemList;
        PacketHandler.OnUpdateItem -= HandleUpdateItem;
        PacketHandler.OnRemoveItem -= HandleRemoveItem;

        // ✅ [ADD]
        PacketHandler.OnEquipItem -= HandleEquipItem;
    }

    private void HandleItemList(RepeatedField<ItemInfo> items)
    {
        _items.Clear();

        foreach (ItemInfo item in items)
            _items[item.Slot] = item;

        Debug.Log($"[InventoryManager] Full Load Complete. Count: {_items.Count}");
        OnInventoryUpdated?.Invoke();
    }

    private void HandleUpdateItem(ItemInfo item)
    {
        _items[item.Slot] = item;

        Debug.Log($"[InventoryManager] Updated Slot {item.Slot} ({item.TemplateId}) equipped={item.IsEquipped}");
        OnInventoryUpdated?.Invoke();
    }

    private void HandleRemoveItem(ulong itemUid)
    {
        var itemInSlot = _items.FirstOrDefault(x => x.Value != null && x.Value.ItemUid == itemUid);

        if (itemInSlot.Value != null)
        {
            _items.Remove(itemInSlot.Key);
            Debug.Log($"[InventoryManager] Removed Item UID: {itemUid} at Slot {itemInSlot.Key}");
            OnInventoryUpdated?.Invoke();
        }
    }

    // ✅ [NEW] 서버가 보내는 S_EQUIP_ITEM을 인벤 캐시에 반영
    private void HandleEquipItem(S_EQUIP_ITEM pkt)
    {
        // 서버가 slotIndex도 주니까 우선 그 슬롯을 신뢰
        if (_items.TryGetValue(pkt.SlotIndex, out ItemInfo item) && item != null && item.ItemUid == pkt.ItemUid)
        {
            item.IsEquipped = pkt.Equipped;     // ✅ 핵심
            _items[pkt.SlotIndex] = item;       // (ItemInfo가 class라면 없어도 되지만 안전하게 갱신)

            Debug.Log($"[InventoryManager] Equip Updated (by slot) slot={pkt.SlotIndex} uid={pkt.ItemUid} equipped={pkt.Equipped}");
            OnInventoryUpdated?.Invoke();
            return;
        }

        // 슬롯이 다르거나(클라/서버 불일치), slot에 다른 아이템이 있으면 UID로 한번 더 찾는다
        var found = _items.FirstOrDefault(x => x.Value != null && x.Value.ItemUid == pkt.ItemUid);
        if (found.Value != null)
        {
            found.Value.IsEquipped = pkt.Equipped;
            _items[found.Key] = found.Value;

            Debug.Log($"[InventoryManager] Equip Updated (by uid) slot={found.Key} uid={pkt.ItemUid} equipped={pkt.Equipped}");
            OnInventoryUpdated?.Invoke();
            return;
        }

        // 못 찾으면 로그만 (인벤/패킷 순서 문제일 수 있음)
        Debug.LogWarning($"[InventoryManager] EquipItem received but item not found. uid={pkt.ItemUid} slotIndex={pkt.SlotIndex}");
    }

    // ================= Helper =================

    public ItemInfo GetItem(int slot)
    {
        if (_items.TryGetValue(slot, out ItemInfo item))
            return item;
        return null;
    }

    public Dictionary<int, ItemInfo> GetAllItems()
    {
        return _items;
    }

    public int GetEmptySlot()
    {
        for (int i = 0; i < 20; i++)
        {
            if (!_items.ContainsKey(i))
                return i;
        }
        return -1;
    }
}
