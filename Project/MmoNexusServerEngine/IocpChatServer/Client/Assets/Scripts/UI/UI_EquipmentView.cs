using UnityEngine;
using Protocol;

/// <summary>
/// 3-slot equipment panel.
/// Wires to InventoryManager cache (IsEquipped + templateId range).
/// </summary>
public class UI_EquipmentView : MonoBehaviour
{
    public UI_EquipSlot weaponSlot;
    public UI_EquipSlot bodySlot;
    public UI_EquipSlot headSlot;

    void Start()
    {
        if (weaponSlot) weaponSlot.Init(EquipSlotType.Weapon);
        if (bodySlot) bodySlot.Init(EquipSlotType.Body);
        if (headSlot) headSlot.Init(EquipSlotType.Head);

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
        if (weaponSlot) weaponSlot.SetItem(InventoryManager.Instance.GetEquippedItem(EquipSlotType.Weapon));
        if (bodySlot) bodySlot.SetItem(InventoryManager.Instance.GetEquippedItem(EquipSlotType.Body));
        if (headSlot) headSlot.SetItem(InventoryManager.Instance.GetEquippedItem(EquipSlotType.Head));
    }
}
