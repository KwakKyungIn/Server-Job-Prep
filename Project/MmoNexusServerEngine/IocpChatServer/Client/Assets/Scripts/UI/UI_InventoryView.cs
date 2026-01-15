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

    [Header("Options")]
    public bool hideEquippedInInventory = true; // 장착 중 아이템은 인벤에서 숨김

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

            if (hideEquippedInInventory && item != null && item.IsEquipped)
                item = null;

            _slots[i].SetItem(item);
            _slots[i].SetSelected(i == _selectedSlot);
        }

        if (_selectedSlot >= 0)
        {
            var selItem = InventoryManager.Instance.GetItem(_selectedSlot);
            if (hideEquippedInInventory && selItem != null && selItem.IsEquipped)
                selItem = null;

            if (selItem == null)
            {
            _selectedSlot = -1;
            for (int i = 0; i < slotCount; i++)
                _slots[i].SetSelected(false);
                    }
        }

        if (contextMenu && contextMenu.IsOpen)
        {
            var cmItem = InventoryManager.Instance.GetItem(contextMenu.SlotIndex);
            if (hideEquippedInInventory && cmItem != null && cmItem.IsEquipped)
                cmItem = null;

            if (cmItem == null)
                contextMenu.Hide();
        }

        if (detailPopupRoot && detailPopupRoot.activeSelf)
        {
            ItemInfo dtItem = null;
            if (_detailSlot >= 0)
                dtItem = InventoryManager.Instance.GetItem(_detailSlot);
            if (hideEquippedInInventory && dtItem != null && dtItem.IsEquipped)
                dtItem = null;

            if (_detailSlot < 0 || dtItem == null)
                CloseDetail();
        }
    }

    public void OnSlotLeftClick(int slotIndex)
    {
        var it = InventoryManager.Instance.GetItem(slotIndex);
        if (hideEquippedInInventory && it != null && it.IsEquipped)
            return;

        _selectedSlot = slotIndex;

        for (int i = 0; i < _slots.Count; i++)
            _slots[i].SetSelected(i == _selectedSlot);

        if (contextMenu && contextMenu.IsOpen)
            contextMenu.Hide();
    }

    public void OnSlotRightClick(int slotIndex, Vector2 screenPos)
    {
        var item = InventoryManager.Instance.GetItem(slotIndex);
        if (hideEquippedInInventory && item != null && item.IsEquipped)
            item = null;

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
            onEquip: () => RequestEquip(slotIndex, true),
            onUnequip: () => RequestEquip(slotIndex, false),
            onUse: () => RequestUseItem(slotIndex),
            onDetails: () => OpenDetail(slotIndex)
        );
    }

    // ===================== Actions =====================

    void RequestEquip(int slotIndex, bool equip)
    {
        var item = InventoryManager.Instance.GetItem(slotIndex);
        if (hideEquippedInInventory && item != null && item.IsEquipped)
            item = null;

        if (item == null) return;

        if (NetworkManager.Instance != null && NetworkManager.Instance.IsMapChanging)
        {
            Debug.Log("[UI] Equip/Unequip blocked during map change.");
            return;
        }

        C_EQUIP_ITEM pkt = new C_EQUIP_ITEM
        {
            ItemUid = item.ItemUid,
            SlotIndex = item.Slot,
            Equip = equip
        };

        NetworkManager.Instance.Send(pkt, (ushort)PacketManager.MsgId.C_EQUIP_ITEM);
    }

    void RequestUseItem(int slotIndex)
    {
        var item = InventoryManager.Instance.GetItem(slotIndex);
        if (hideEquippedInInventory && item != null && item.IsEquipped)
            item = null;

        if (item == null) return;

        if (NetworkManager.Instance != null && NetworkManager.Instance.IsMapChanging)
        {
            Debug.Log("[UI] Use blocked during map change.");
            return;
        }

        // ✅ Protocol.proto 기준: C_USE_ITEM은 itemUid만 보냄
        C_USE_ITEM pkt = new C_USE_ITEM
        {
            ItemUid = item.ItemUid
        };

        NetworkManager.Instance.Send(pkt, (ushort)PacketManager.MsgId.C_USE_ITEM);

        // (선택) 로컬 로그
        Debug.Log($"[UI] UseItem sent. template={item.TemplateId} uid={item.ItemUid}");
    }

    void OpenDetail(int slotIndex)
    {
        var item = InventoryManager.Instance.GetItem(slotIndex);
        if (hideEquippedInInventory && item != null && item.IsEquipped)
            item = null;

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
