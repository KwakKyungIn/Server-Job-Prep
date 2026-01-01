using System.Collections.Generic;
using UnityEngine;
using UnityEngine.UI;
using TMPro;
using Protocol;

public class UI_InventoryView : MonoBehaviour
{
    [Header("Grid")]
    public UI_ItemSlot slotPrefab;
    public Transform slotRoot;
    public int slotCount = 24;

    [Header("Context Menu")]
    public UI_InventoryContextMenu contextMenu;

    [Header("Detail Popup (Details 눌렀을 때만 켜짐)")]
    public GameObject detailPopupRoot;
    public Image detailIcon;
    public TMP_Text detailName;
    public TMP_Text detailDesc;
    public Button detailCloseButton;

    readonly List<UI_ItemSlot> _slots = new();
    int _selectedSlot = -1;
    int _detailSlot = -1;

    void Start()
    {
        // 슬롯 생성
        for (int i = 0; i < slotCount; i++)
        {
            var slot = Instantiate(slotPrefab, slotRoot);
            slot.Init(i, this);
            slot.SetItem(null);
            slot.SetSelected(false);
            _slots.Add(slot);
        }

        if (contextMenu) contextMenu.Hide();
        CloseDetail();

        if (detailCloseButton)
        {
            detailCloseButton.onClick.RemoveAllListeners();
            detailCloseButton.onClick.AddListener(CloseDetail);
        }

        InventoryManager.Instance.OnInventoryUpdated += Refresh;
        Refresh();
    }

    void OnDestroy()
    {
        if (InventoryManager.Instance != null)
            InventoryManager.Instance.OnInventoryUpdated -= Refresh;
    }

    public void Refresh()
    {
        var items = InventoryManager.Instance.GetAllItems();

        for (int i = 0; i < slotCount; i++)
        {
            items.TryGetValue(i, out ItemInfo item);
            _slots[i].SetItem(item);
            _slots[i].SetSelected(i == _selectedSlot);
        }

        // 선택된 슬롯이 비었으면 선택 해제
        if (_selectedSlot >= 0 && InventoryManager.Instance.GetItem(_selectedSlot) == null)
        {
            _selectedSlot = -1;
            for (int i = 0; i < slotCount; i++)
                _slots[i].SetSelected(false);
        }

        // 메뉴가 떠있는데 해당 슬롯이 비어버리면 닫기
        if (contextMenu && contextMenu.IsOpen)
        {
            if (InventoryManager.Instance.GetItem(contextMenu.SlotIndex) == null)
                contextMenu.Hide();
        }

        // 디테일 팝업이 떠있는데 아이템이 사라지면 닫기
        if (detailPopupRoot && detailPopupRoot.activeSelf)
        {
            if (_detailSlot < 0 || InventoryManager.Instance.GetItem(_detailSlot) == null)
                CloseDetail();
        }
    }

    // ========== Slot Events ==========

    public void OnSlotLeftClick(int slotIndex)
    {
        _selectedSlot = slotIndex;

        for (int i = 0; i < _slots.Count; i++)
            _slots[i].SetSelected(i == _selectedSlot);

        // UX: 좌클릭하면 메뉴는 닫아줌
        if (contextMenu && contextMenu.IsOpen)
            contextMenu.Hide();
    }

    public void OnSlotRightClick(int slotIndex, Vector2 screenPos)
    {
        var item = InventoryManager.Instance.GetItem(slotIndex);
        if (item == null)
        {
            if (contextMenu) contextMenu.Hide();
            return;
        }

        if (!contextMenu)
        {
            Debug.LogWarning("[UI_InventoryView] contextMenu is null");
            return;
        }

        contextMenu.Show(
            slotIndex,
            item,
            screenPos,
            onEquip: () => RequestEquipToggle(slotIndex),
            onDetails: () => OpenDetail(slotIndex)
        );
    }

    // ========== Actions ==========

    void RequestEquipToggle(int slotIndex)
    {
        var item = InventoryManager.Instance.GetItem(slotIndex);
        if (item == null) return;

        // 필요하면 전환 중 차단
        if (NetworkManager.Instance != null && NetworkManager.Instance.IsMapChanging)
        {
            Debug.Log("[UI] Equip blocked during map change.");
            if (contextMenu) contextMenu.Hide();
            return;
        }

        C_EQUIP_ITEM pkt = new C_EQUIP_ITEM
        {
            ItemUid = item.ItemUid,
            SlotIndex = item.Slot,
            Equip = !item.IsEquipped
        };

        NetworkManager.Instance.Send(pkt, (ushort)PacketManager.MsgId.C_EQUIP_ITEM);

        if (contextMenu) contextMenu.Hide();
    }

    void OpenDetail(int slotIndex)
    {
        var item = InventoryManager.Instance.GetItem(slotIndex);
        if (item == null) return;

        _detailSlot = slotIndex;

        if (detailPopupRoot) detailPopupRoot.SetActive(true);

        if (detailIcon)
        {
            detailIcon.gameObject.SetActive(true);
            detailIcon.color = (item.TemplateId == 101) ? Color.red :
                               (item.TemplateId == 102) ? Color.blue : Color.white;
        }

        if (detailName) detailName.text = $"Template {item.TemplateId}";
        if (detailDesc) detailDesc.text = $"UID: {item.ItemUid}\nSlot: {item.Slot}\nCount: {item.Count}\nEquipped: {item.IsEquipped}";

        if (contextMenu) contextMenu.Hide();
    }

    void CloseDetail()
    {
        _detailSlot = -1;

        if (detailPopupRoot) detailPopupRoot.SetActive(false);
        if (detailIcon) detailIcon.gameObject.SetActive(false);
        if (detailName) detailName.text = "";
        if (detailDesc) detailDesc.text = "";
    }
}
