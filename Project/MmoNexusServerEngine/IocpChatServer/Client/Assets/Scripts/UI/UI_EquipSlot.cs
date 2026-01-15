using UnityEngine;
using UnityEngine.EventSystems;
using UnityEngine.UI;
using Protocol;

/// <summary>
/// Simple 3-slot equipment UI (Weapon/Body/Head).
/// - Right click: Unequip
/// - Drop (drag from inventory): Equip (slot type must match)
/// </summary>
public class UI_EquipSlot : MonoBehaviour, IPointerClickHandler, IDropHandler
{
    [Header("UI")]
    public Image icon;
    public Image selectedFrame;
    public Image emptyFrame; // optional

    [Header("Slot Type")]
    public EquipSlotType slotType = EquipSlotType.None;

    ItemInfo _item;

    public void Init(EquipSlotType type)
    {
        slotType = type;
        SetItem(null);
    }

    public void SetItem(ItemInfo item)
    {
        _item = item;

        if (emptyFrame) emptyFrame.gameObject.SetActive(_item == null);

        if (icon == null)
            return;

        if (_item == null)
        {
            icon.gameObject.SetActive(false);
            return;
        }

        icon.gameObject.SetActive(true);
        icon.sprite = ItemIconDB.Get(_item.TemplateId);
        icon.color = Color.white;
    }

    public void SetSelected(bool v)
    {
        if (selectedFrame) selectedFrame.gameObject.SetActive(v);
    }

    public void OnPointerClick(PointerEventData eventData)
    {
        if (_item == null)
            return;

        // Right click -> Unequip
        if (eventData.button == PointerEventData.InputButton.Right)
        {
            RequestEquip(_item, false);
        }
    }

    public void OnDrop(PointerEventData eventData)
    {
        if (!UI_DragPayload.HasPayload)
            return;

        var payload = UI_DragPayload.Current;
        if (payload.Type != UI_DragPayload.PayloadType.Item)
            return;

        // Slot type validation (templateId range based)
        if (EquipUtil.GetSlotType(payload.TemplateId) != slotType)
            return;

        // Find cached item by slot to get full info (best effort)
        ItemInfo it = InventoryManager.Instance.GetItem(payload.InventorySlot);
        if (it == null || it.ItemUid != payload.RefId)
        {
            // fallback: search by UID
            foreach (var kv in InventoryManager.Instance.GetAllItems())
            {
                if (kv.Value != null && kv.Value.ItemUid == payload.RefId)
                {
                    it = kv.Value;
                    break;
                }
            }
        }

        if (it == null)
            return;

        RequestEquip(it, true);
    }

    void RequestEquip(ItemInfo item, bool equip)
    {
        if (item == null) return;
        if (NetworkManager.Instance != null && NetworkManager.Instance.IsMapChanging) return;

        C_EQUIP_ITEM pkt = new C_EQUIP_ITEM
        {
            ItemUid = item.ItemUid,
            SlotIndex = item.Slot, // inventory slot
            Equip = equip
        };

        NetworkManager.Instance.Send(pkt, (ushort)PacketManager.MsgId.C_EQUIP_ITEM);
    }
}
