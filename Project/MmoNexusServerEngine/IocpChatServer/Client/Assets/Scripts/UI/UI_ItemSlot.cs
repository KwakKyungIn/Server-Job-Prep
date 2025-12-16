using TMPro;
using UnityEngine;
using UnityEngine.UI;
using Protocol;

public class UI_ItemSlot : MonoBehaviour
{
    public Image icon;
    public TMP_Text countText;
    public Image equipMark;
    public Image selectedFrame;
    public Button button;

    int _slotIndex;
    UI_InventoryView _owner;
    ItemInfo _item;

    public void Init(int slotIndex, UI_InventoryView owner)
    {
        _slotIndex = slotIndex;
        _owner = owner;

        button.onClick.RemoveAllListeners();
        button.onClick.AddListener(() => _owner.OnClickSlot(_slotIndex));
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
            icon.gameObject.SetActive(false);
            countText.text = "";
            if (equipMark) equipMark.gameObject.SetActive(false);
            return;
        }

        icon.gameObject.SetActive(true);
        countText.text = item.Count > 1 ? item.Count.ToString() : "";
        if (equipMark) equipMark.gameObject.SetActive(item.IsEquipped);

        // 임시 아이콘(색)
        if (item.TemplateId == 101) icon.color = Color.red;
        else if (item.TemplateId == 102) icon.color = Color.blue;
        else icon.color = Color.white;
    }
}
