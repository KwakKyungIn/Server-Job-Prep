using TMPro;
using UnityEngine;
using UnityEngine.EventSystems;
using UnityEngine.UI;
using Protocol;

/// <summary>
/// A single quickslot UI cell.
/// - Drop Item from inventory -> sends C_SET_QUICKSLOT
/// - Right click -> clear
/// - Left click -> use (optional)
/// </summary>
public class UI_QuickSlotSlot : MonoBehaviour, IDropHandler, IPointerClickHandler
{
    [Header("UI")]
    public Image icon;
    public TMP_Text countText;
    public TMP_Text keyText;

    int _index;
    QuickSlotInfo _info;

    public void Init(int idx)
    {
        _index = idx;
    }

    public void SetKeyLabel(string label)
    {
        if (keyText) keyText.text = label;
    }

    public void Bind(QuickSlotInfo info)
    {
        _info = info;
        Refresh();
    }

    public void Refresh()
    {
        if (_info == null || _info.RefType == QuickSlotRefType.QsNone || _info.RefId == 0)
        {
            if (icon) icon.gameObject.SetActive(false);
            if (countText) countText.text = "";
            return;
        }

        if (_info.RefType == QuickSlotRefType.QsItem)
        {
            var item = FindItemByUid(_info.RefId);
            if (item == null)
            {
                // TODO: item missing (consumed/moved) -> decide whether to auto-clear
                if (icon) icon.gameObject.SetActive(false);
                if (countText) countText.text = "";
                return;
            }

            if (icon)
            {
                icon.gameObject.SetActive(true);
                icon.sprite = ItemIconDB.Get(item.TemplateId);
                icon.color = Color.white;
            }

            if (countText)
                countText.text = item.Count > 1 ? item.Count.ToString() : "";

            return;
        }

        // QS_SKILL
        // TODO: SkillIconDB / skill tooltip
        if (icon) icon.gameObject.SetActive(false);
        if (countText) countText.text = "";
    }

    public void OnDrop(PointerEventData eventData)
    {
        if (!UI_DragPayload.HasPayload)
            return;

        var payload = UI_DragPayload.Current;
        if (payload.Type == UI_DragPayload.PayloadType.Item)
        {
            QuickSlotManager.Instance.RequestSetSlot(_index, QuickSlotRefType.QsItem, payload.RefId);
        }

        // TODO: payload.Type == Skill -> QsSkill
    }

    public void OnPointerClick(PointerEventData eventData)
    {
        if (eventData.button == PointerEventData.InputButton.Right)
        {
            QuickSlotManager.Instance.RequestSetSlot(_index, QuickSlotRefType.QsNone, 0);
            return;
        }

        if (eventData.button == PointerEventData.InputButton.Left)
        {
            // optional: click-to-use
            QuickSlotManager.Instance.TryUse(_index);
        }
    }

    ItemInfo FindItemByUid(ulong uid)
    {
        var all = InventoryManager.Instance.GetAllItems();
        if (all == null) return null;

        foreach (var kv in all)
        {
            var it = kv.Value;
            if (it != null && it.ItemUid == uid)
                return it;
        }
        return null;
    }
}
