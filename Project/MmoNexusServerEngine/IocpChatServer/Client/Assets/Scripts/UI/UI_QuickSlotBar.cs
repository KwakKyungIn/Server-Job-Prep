using System.Collections.Generic;
using UnityEngine;

/// <summary>
/// Quickslot bar UI binder.
/// Attach to a panel that has 12 child slot objects with UI_QuickSlotSlot.
/// </summary>
public class UI_QuickSlotBar : MonoBehaviour
{
    [Header("Slots (auto fill from children if empty)")]
    public List<UI_QuickSlotSlot> slots = new List<UI_QuickSlotSlot>();

    static readonly string[] DefaultKeyLabels = new string[]
    {
        "1","2","3","4","5","6","7","8","9","0","-","="
    };

    void Awake()
    {
        if (slots == null || slots.Count == 0)
        {
            slots = new List<UI_QuickSlotSlot>(GetComponentsInChildren<UI_QuickSlotSlot>(includeInactive: true));
        }

        // Ensure order by hierarchy (common UI setup)
        slots.Sort((a, b) => a.transform.GetSiblingIndex().CompareTo(b.transform.GetSiblingIndex()));

        for (int i = 0; i < slots.Count; i++)
        {
            slots[i].Init(i);
            if (i < DefaultKeyLabels.Length)
                slots[i].SetKeyLabel(DefaultKeyLabels[i]);
        }
    }

    void OnEnable()
    {
        QuickSlotManager.Instance.OnUpdated += Refresh;
        Refresh();
    }

    void OnDisable()
    {
        QuickSlotManager.Instance.OnUpdated -= Refresh;
    }

    void Refresh()
    {
        // In case the panel is not configured yet
        if (slots == null || slots.Count == 0)
            return;

        int max = Mathf.Min(slots.Count, QuickSlotManager.MaxSlots);
        for (int i = 0; i < max; i++)
        {
            var info = QuickSlotManager.Instance.GetSlot(i);
            slots[i].Bind(info);
        }

        // TODO: If you have more than 12 slot objects by mistake, hide extras.
    }
}
