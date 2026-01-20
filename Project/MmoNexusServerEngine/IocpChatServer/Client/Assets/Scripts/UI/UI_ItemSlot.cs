using TMPro;
using UnityEngine;
using UnityEngine.UI;
using UnityEngine.EventSystems;
using Protocol;

public class UI_ItemSlot : MonoBehaviour, IPointerClickHandler, IBeginDragHandler, IDragHandler, IEndDragHandler, IDropHandler
{
    public Image icon;
    public TMP_Text countText;
    public Image equipMark;
    public Image selectedFrame;

    int _slotIndex;
    UI_InventoryView _owner;
    ItemInfo _item;

    Canvas _canvas;
    bool _dragging;

    void Awake()
    {
        _canvas = GetComponentInParent<Canvas>();
    }

    public void Init(int slotIndex, UI_InventoryView owner)
    {
        _slotIndex = slotIndex;
        _owner = owner;
    }

    public void OnPointerClick(PointerEventData eventData)
    {
        // Drag 종료 후 PointerClick가 함께 호출되는 케이스 방지
        if (_dragging) return;
        if (_owner == null) return;

        if (eventData.button == PointerEventData.InputButton.Left)
            _owner.OnSlotLeftClick(_slotIndex);
        else if (eventData.button == PointerEventData.InputButton.Right)
            _owner.OnSlotRightClick(_slotIndex, eventData.position);
    }

    // ============================================================
    // [Drag & Drop -> QuickSlot]
    // ============================================================
    public void OnBeginDrag(PointerEventData eventData)
    {
        if (_item == null) return;

        _dragging = true;
        UI_DragPayload.SetItem(_item);

        // show drag ghost
        var sp = ItemIconDB.Get(_item.TemplateId);
        UI_DragGhost.Begin(_canvas, sp, eventData.position);
    }

    public void OnDrag(PointerEventData eventData)
    {
        if (!_dragging) return;
        UI_DragGhost.UpdatePos(eventData.position);
    }

    public void OnEndDrag(PointerEventData eventData)
    {
        if (!_dragging) return;
        _dragging = false;

        UI_DragGhost.End();
        UI_DragPayload.Clear();
    }

    // ============================================================
    // [Drag & Drop -> Inventory] (Move / Swap / Merge)
    // ============================================================
    public void OnDrop(PointerEventData eventData)
    {
        if (!UI_DragPayload.HasPayload)
            return;

        var payload = UI_DragPayload.Current;
        if (payload.Type != UI_DragPayload.PayloadType.Item)
            return;

        int fromSlot = payload.InventorySlot;
        int toSlot = _slotIndex;
        if (fromSlot < 0 || toSlot < 0 || fromSlot == toSlot)
            return;

        // If the destination slot actually holds an equipped item (hidden by UI), block the drop.
        var realDst = InventoryManager.Instance.GetItem(toSlot);
        if (realDst != null && realDst.IsEquipped)
            return;

        InventoryManager.Instance.RequestInvDragDrop(fromSlot, toSlot, payload.RefId);
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
