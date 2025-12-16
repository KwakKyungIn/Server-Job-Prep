using System.Collections.Generic;
using UnityEngine;
using Protocol;
using TMPro;
using UnityEngine.UI;

public class UI_InventoryView : MonoBehaviour
{
    [Header("Grid")]
    public UI_ItemSlot slotPrefab;
    public Transform slotRoot;
    public int slotCount = 24;

    [Header("Detail")]
    public Image detailIcon;
    public TMP_Text detailName;
    public TMP_Text detailDesc;
    public Button equipButton;
    public TMP_Text equipButtonLabel;

    readonly List<UI_ItemSlot> _slots = new();
    int _selectedSlot = -1;

    void Start()
    {
        // 슬롯 생성
        for (int i = 0; i < slotCount; i++)
        {
            var slot = Instantiate(slotPrefab, slotRoot);
            slot.Init(i, this);
            slot.SetItem(null);
            _slots.Add(slot);
        }

        InventoryManager.Instance.OnInventoryUpdated += Refresh;
        Refresh();
        ClearDetail();
    }

    void OnDestroy()
    {
        InventoryManager.Instance.OnInventoryUpdated -= Refresh;
    }

    public void Refresh()
    {
        var items = InventoryManager.Instance.GetAllItems();

        for (int i = 0; i < slotCount; i++)
        {
            items.TryGetValue(i, out ItemInfo item);
            _slots[i].SetItem(item);

            // 선택 프레임 갱신
            _slots[i].SetSelected(i == _selectedSlot);
        }

        // 선택된 슬롯이 비었으면 상세 패널 비움
        if (_selectedSlot >= 0 && InventoryManager.Instance.GetItem(_selectedSlot) == null)
        {
            _selectedSlot = -1;
            ClearDetail();
            for (int i = 0; i < slotCount; i++)
                _slots[i].SetSelected(false);
        }
    }

    public void OnClickSlot(int slotIndex)
    {
        _selectedSlot = slotIndex;

        // 선택 표시
        for (int i = 0; i < _slots.Count; i++)
            _slots[i].SetSelected(i == _selectedSlot);

        var item = InventoryManager.Instance.GetItem(slotIndex);
        if (item == null)
        {
            ClearDetail();
            return;
        }

        ShowDetail(item);
    }

    void ShowDetail(ItemInfo item)
    {
        // 아이콘은 지금은 색으로만 표현 (나중에 스프라이트 로드)
        detailIcon.gameObject.SetActive(true);
        detailIcon.color = (item.TemplateId == 101) ? Color.red :
                           (item.TemplateId == 102) ? Color.blue : Color.white;

        detailName.text = $"Template {item.TemplateId}";
        detailDesc.text = $"UID: {item.ItemUid}\nSlot: {item.Slot}\nCount: {item.Count}";

        equipButton.interactable = true;
        equipButton.onClick.RemoveAllListeners();
        equipButton.onClick.AddListener(() => RequestEquipToggle(item));

        equipButtonLabel.text = item.IsEquipped ? "Unequip" : "Equip";
    }

    void ClearDetail()
    {
        if (detailIcon) detailIcon.gameObject.SetActive(false);
        if (detailName) detailName.text = "";
        if (detailDesc) detailDesc.text = "";
        if (equipButtonLabel) equipButtonLabel.text = "Equip";
        if (equipButton) equipButton.interactable = false;
        if (equipButton) equipButton.onClick.RemoveAllListeners();
    }

    void RequestEquipToggle(ItemInfo item)
    {
        C_EQUIP_ITEM pkt = new C_EQUIP_ITEM();
        pkt.ItemUid = item.ItemUid;
        pkt.SlotIndex = item.Slot;
        pkt.Equip = !item.IsEquipped;

        NetworkManager.Instance.Send(pkt, (ushort)PacketManager.MsgId.C_EQUIP_ITEM);
    }
}
