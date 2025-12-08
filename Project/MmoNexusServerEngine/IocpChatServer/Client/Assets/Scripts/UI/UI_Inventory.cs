using System.Collections.Generic;
using UnityEngine;
using Protocol;

public class UI_Inventory : MonoBehaviour
{
    public GameObject _slotPrefab;
    public Transform _slotRoot; // Grid Layout Group이 있는 Panel

    private List<UI_ItemSlot> _slots = new List<UI_ItemSlot>();
    private const int SLOT_COUNT = 24; // 인벤토리 크기 고정

    void Start()
    {
        // 1. 슬롯 미리 생성 (Object Pooling 비슷하게)
        for (int i = 0; i < SLOT_COUNT; i++)
        {
            GameObject go = Instantiate(_slotPrefab, _slotRoot);
            UI_ItemSlot slotUI = go.GetComponent<UI_ItemSlot>();
            _slots.Add(slotUI);

            // 빈 슬롯으로 초기화
            slotUI.SetItem(null);
        }

        // 2. 이벤트 구독
        InventoryManager.Instance.OnInventoryUpdated += RefreshUI;

        // 3. 최초 1회 갱신 (이미 데이터가 있을 수 있으므로)
        RefreshUI();
    }

    void OnDestroy()
    {
        if (InventoryManager.Instance != null)
            InventoryManager.Instance.OnInventoryUpdated -= RefreshUI;
    }

    void RefreshUI()
    {
        // 매니저에서 데이터 가져오기 (Dictionary)
        var items = InventoryManager.Instance.GetAllItems();

        for (int i = 0; i < SLOT_COUNT; i++)
        {
            // 해당 슬롯(i)에 아이템이 있는가?
            if (items.TryGetValue(i, out ItemInfo item))
            {
                _slots[i].SetItem(item);
            }
            else
            {
                _slots[i].SetItem(null);
            }
        }

        Debug.Log("[UI] Inventory Refreshed.");
    }
}