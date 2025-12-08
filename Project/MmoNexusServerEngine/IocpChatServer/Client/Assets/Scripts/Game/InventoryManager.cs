using System;
using System.Collections.Generic;
using System.Linq;
using Google.Protobuf.Collections;
using Protocol;
using UnityEngine;

public class InventoryManager
{
    // [Singleton] 어디서든 접근 가능하게
    private static InventoryManager _instance = new InventoryManager();
    public static InventoryManager Instance { get { return _instance; } }

    // [Data] 슬롯 번호(Int)를 Key로 사용하여 빠른 검색 지원
    // List보다 Dictionary가 특정 슬롯 비우기/채우기에 훨씬 효율적이다.
    private Dictionary<int, ItemInfo> _items = new Dictionary<int, ItemInfo>();

    // [Event] 인벤토리가 변했을 때 UI에게 "그려라"라고 알리는 신호
    public Action OnInventoryUpdated;

    public void Init()
    {
        // PacketHandler가 받은 소식을 내가 가로채서 처리한다.
        PacketHandler.OnItemList += HandleItemList;
        PacketHandler.OnUpdateItem += HandleUpdateItem;
        PacketHandler.OnRemoveItem += HandleRemoveItem;

        Debug.Log("[InventoryManager] Initialized & Listening...");
    }

    // 1. 전체 리스트 로딩 (로그인 직후)
    private void HandleItemList(RepeatedField<ItemInfo> items)
    {
        _items.Clear();

        foreach (ItemInfo item in items)
        {
            _items.Add(item.Slot, item);
        }

        Debug.Log($"[InventoryManager] Full Load Complete. Count: {_items.Count}");

        // UI 갱신 알림
        OnInventoryUpdated?.Invoke();
    }

    // 2. 아이템 1개 변경/추가 (획득, 이동, 사용 등)
    private void HandleUpdateItem(ItemInfo item)
    {
        // 이미 해당 슬롯에 무언가 있다면 덮어쓰기, 없으면 추가
        if (_items.ContainsKey(item.Slot))
        {
            _items[item.Slot] = item;
        }
        else
        {
            _items.Add(item.Slot, item);
        }

        Debug.Log($"[InventoryManager] Updated Slot {item.Slot} ({item.TemplateId})");
        OnInventoryUpdated?.Invoke();
    }

    // 3. 아이템 삭제 (버리기, 다 씀)
    private void HandleRemoveItem(ulong itemUid)
    {
        // UID로 슬롯을 찾아야 함 (서버가 UID만 줬으니까)
        // Linq를 써서 UID가 같은 놈의 Key(Slot)를 찾는다.
        var itemInSlot = _items.FirstOrDefault(x => x.Value.ItemUid == itemUid);

        // KeyValuePair는 struct라 null 체크 대신 Key 유효성 체크
        if (itemInSlot.Value != null)
        {
            _items.Remove(itemInSlot.Key);
            Debug.Log($"[InventoryManager] Removed Item UID: {itemUid} at Slot {itemInSlot.Key}");
            OnInventoryUpdated?.Invoke();
        }
    }

    // ============================================================
    // [Helper Methods] UI나 로직에서 가져다 쓸 함수들
    // ============================================================

    // 특정 슬롯에 아이템이 있냐?
    public ItemInfo GetItem(int slot)
    {
        if (_items.TryGetValue(slot, out ItemInfo item))
            return item;
        return null;
    }

    // 전체 아이템 내놔 (UI가 루프 돌릴 때 사용)
    public Dictionary<int, ItemInfo> GetAllItems()
    {
        return _items;
    }

    // 빈 슬롯 찾기 (혹시 클라에서 예측해서 넣을 때 필요)
    public int GetEmptySlot()
    {
        for (int i = 0; i < 20; i++) // 가방 크기 20칸 가정
        {
            if (!_items.ContainsKey(i))
                return i;
        }
        return -1;
    }
}