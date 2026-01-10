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

        if (icon)
        {
            icon.gameObject.SetActive(true);

            // ✅ 진짜 아이콘 적용
            icon.sprite = ItemIconDB.Get(item.TemplateId);

            // sprite가 없으면 아예 숨기고 싶으면 이거
            // icon.gameObject.SetActive(icon.sprite != null);

            // 색은 기본 흰색으로 (원본 스프라이트 컬러 유지)
            icon.color = Color.white;
        }

        if (countText) countText.text = item.Count > 1 ? item.Count.ToString() : "";
        if (equipMark) equipMark.gameObject.SetActive(item.IsEquipped);
    }

}
