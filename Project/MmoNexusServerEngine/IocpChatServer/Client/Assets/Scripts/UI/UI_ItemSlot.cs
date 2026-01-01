using TMPro;
using UnityEngine;
using UnityEngine.UI;
using UnityEngine.EventSystems;
using Protocol;

public class UI_ItemSlot : MonoBehaviour, IPointerClickHandler
{
    public Image icon;
    public TMP_Text countText;
    public Image equipMark;
    public Image selectedFrame;

    int _slotIndex;
    UI_InventoryView _owner;
    ItemInfo _item;

    public void Init(int slotIndex, UI_InventoryView owner)
    {
        _slotIndex = slotIndex;
        _owner = owner;
    }

    public void OnPointerClick(PointerEventData eventData)
    {
        if (_owner == null) return;

        if (eventData.button == PointerEventData.InputButton.Left)
            _owner.OnSlotLeftClick(_slotIndex);
        else if (eventData.button == PointerEventData.InputButton.Right)
            _owner.OnSlotRightClick(_slotIndex, eventData.position);
    }

    public void SetSelected(bool v)
    {
        if (selectedFrame) selectedFrame.gameObject.SetActive(v);
    }

    public void SetItem(ItemInfo item)
    {
        _item = item;

        if (item == null)
        {
            if (icon) icon.gameObject.SetActive(false);
            if (countText) countText.text = "";
            if (equipMark) equipMark.gameObject.SetActive(false);
            return;
        }

        if (icon) icon.gameObject.SetActive(true);
        if (countText) countText.text = item.Count > 1 ? item.Count.ToString() : "";
        if (equipMark) equipMark.gameObject.SetActive(item.IsEquipped);

        // 임시 아이콘(색)
        if (icon)
        {
            if (item.TemplateId == 101) icon.color = Color.red;
            else if (item.TemplateId == 102) icon.color = Color.blue;
            else icon.color = Color.white;
        }
    }
}
