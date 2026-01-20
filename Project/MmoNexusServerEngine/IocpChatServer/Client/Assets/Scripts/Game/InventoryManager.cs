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
        // [MOVE/SWAP SUPPORT]
        // If the same uid already exists in a different slot, remove the old slot entry first.
        // (S_CHANGE_ITEM may represent a slot move; server won't send a separate remove.)
        var existing = _items.FirstOrDefault(x => x.Value != null && x.Value.ItemUid == item.ItemUid);
        if (existing.Value != null && existing.Key != item.Slot)
            _items.Remove(existing.Key);

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

    public ItemInfo GetItemByUid(ulong itemUid)
    {
        foreach (var kv in _items)
        {
            var it = kv.Value;
            if (it != null && it.ItemUid == itemUid)
                return it;
        }
        return null;
    }

    public int GetEmptySlot()
    {
        // NOTE: server inventory slots are currently 24.
        for (int i = 0; i < 24; i++)
        {
            if (!_items.ContainsKey(i))
                return i;
        }
        return -1;
    }

    // =========================================================
    // [Client -> Server] Inventory Drag&Drop (Move/Swap/Merge)
    // =========================================================
    public void RequestInvDragDrop(int fromSlot, int toSlot, ulong itemUid)
    {
        if (NetworkManager.Instance == null)
            return;
        if (NetworkManager.Instance.IsMapChanging)
            return;

        if (fromSlot < 0 || toSlot < 0 || fromSlot == toSlot)
            return;

        // Client is NOT authoritative; server validates (uid/slot ownership, equipped, etc.)
        var pkt = new C_INV_DRAG_DROP
        {
            FromSlot = fromSlot,
            ToSlot = toSlot,
            ItemUid = itemUid,
        };

        NetworkManager.Instance.Send(pkt, (ushort)PacketManager.MsgId.C_INV_DRAG_DROP);
    }

    // ================= Equipment Helpers =================

    public ItemInfo GetEquippedItem(EquipSlotType slotType)
    {
        // NOTE: Equipped items may be hidden in the inventory UI, but remain in this cache.
        return _items.Values.FirstOrDefault(x => x != null && x.IsEquipped && EquipUtil.GetSlotType(x.TemplateId) == slotType);
    }

    public static bool IsEquipable(ItemInfo item)
    {
        if (item == null) return false;
        return EquipUtil.GetSlotType(item.TemplateId) != EquipSlotType.None;
    }

}

public enum EquipSlotType
{
    None = 0,
    Weapon = 1,
    Body = 2,
    Head = 3,
}

public static class EquipUtil
{
    public static EquipSlotType GetSlotType(int templateId)
    {
        if (templateId >= 1000 && templateId < 2000) return EquipSlotType.Weapon;
        if (templateId >= 2000 && templateId < 3000) return EquipSlotType.Body;
        if (templateId >= 4000 && templateId < 5000) return EquipSlotType.Head;
        return EquipSlotType.None;
    }
}
