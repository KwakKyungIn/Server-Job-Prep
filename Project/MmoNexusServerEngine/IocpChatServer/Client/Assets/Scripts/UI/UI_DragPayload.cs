using Protocol;

/// <summary>
/// Simple global drag payload used by UI drag & drop.
/// For quickslot v1 we only need item drag (QS_ITEM).
/// </summary>
public static class UI_DragPayload
{
    public enum PayloadType
    {
        None,
        Item,
        Skill,
    }

    public struct Payload
    {
        public PayloadType Type;
        public ulong RefId;          // itemUid or skillId
        public int TemplateId;       // for preview
        public int InventorySlot;    // optional
    }

    public static bool HasPayload => Current.Type != PayloadType.None;
    public static Payload Current { get; private set; }

    public static void SetItem(ItemInfo item)
    {
        if (item == null)
        {
            Clear();
            return;
        }

        Current = new Payload
        {
            Type = PayloadType.Item,
            RefId = item.ItemUid,
            TemplateId = item.TemplateId,
            InventorySlot = item.Slot,
        };
    }

    public static void Clear()
    {
        Current = new Payload { Type = PayloadType.None, RefId = 0, TemplateId = 0, InventorySlot = -1 };
    }
}
