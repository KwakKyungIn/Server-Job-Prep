using System;
using System.Collections.Generic;
using Google.Protobuf.Collections;
using Protocol;
using UnityEngine;

/// <summary>
/// Client-side quickslot cache.
/// - Authoritative state comes from server: S_QUICKSLOT_LIST / S_SET_QUICKSLOT
/// - UI reads from this cache.
/// </summary>
public class QuickSlotManager
{
    public const int MaxSlots = 12;

    static readonly QuickSlotManager _instance = new QuickSlotManager();
    public static QuickSlotManager Instance => _instance;

    public event Action OnUpdated;

    bool _inited = false;
    readonly QuickSlotInfo[] _slots = new QuickSlotInfo[MaxSlots];

    public bool IsReady => _inited;

    public void Init()
    {
        if (_inited) return;
        _inited = true;

        // init empty
        for (int i = 0; i < MaxSlots; i++)
            _slots[i] = new QuickSlotInfo { SlotIndex = i, RefType = QuickSlotRefType.QsNone, RefId = 0 };

        PacketHandler.OnQuickSlotList += HandleQuickSlotList;
        PacketHandler.OnQuickSlotChanged += HandleQuickSlotChanged;

        // When inventory changes, quickslot item icons may need refresh.
        InventoryManager.Instance.OnInventoryUpdated += NotifyUpdated;

        Debug.Log("[QuickSlotManager] Initialized & Listening...");
    }

    public void Shutdown()
    {
        if (!_inited) return;
        _inited = false;

        PacketHandler.OnQuickSlotList -= HandleQuickSlotList;
        PacketHandler.OnQuickSlotChanged -= HandleQuickSlotChanged;
        if (InventoryManager.Instance != null)
            InventoryManager.Instance.OnInventoryUpdated -= NotifyUpdated;
    }

    public void ResetToEmpty()
    {
        for (int i = 0; i < MaxSlots; i++)
        {
            _slots[i].SlotIndex = i;
            _slots[i].RefType = QuickSlotRefType.QsNone;
            _slots[i].RefId = 0;
        }
        NotifyUpdated();
    }

    public QuickSlotInfo GetSlot(int idx)
    {
        if (idx < 0 || idx >= MaxSlots) return null;
        return _slots[idx];
    }

    public IReadOnlyList<QuickSlotInfo> GetAll()
    {
        return _slots;
    }

    public void RequestSetSlot(int slotIndex, QuickSlotRefType refType, ulong refId)
    {
        if (slotIndex < 0 || slotIndex >= MaxSlots) return;

        if (NetworkManager.Instance != null && NetworkManager.Instance.IsMapChanging)
        {
            Debug.Log("[QuickSlot] Set blocked during map change.");
            return;
        }

        // normalize clear
        if (refType == QuickSlotRefType.QsNone || refId == 0)
        {
            refType = QuickSlotRefType.QsNone;
            refId = 0;
        }

        // optimistic local update (server will echo back S_SET_QUICKSLOT)
        ApplyLocal(slotIndex, refType, refId);

        C_SET_QUICKSLOT pkt = new C_SET_QUICKSLOT
        {
            SlotIndex = slotIndex,
            RefType = refType,
            RefId = refId
        };

        NetworkManager.Instance.Send(pkt, (ushort)PacketManager.MsgId.C_SET_QUICKSLOT);
    }

    public void TryUse(int slotIndex, Transform myTransform = null)
    {
        if (slotIndex < 0 || slotIndex >= MaxSlots) return;
        var s = _slots[slotIndex];
        if (s == null || s.RefType == QuickSlotRefType.QsNone || s.RefId == 0) return;

        if (NetworkManager.Instance != null && NetworkManager.Instance.IsMapChanging)
            return;

        if (s.RefType == QuickSlotRefType.QsItem)
        {
            // TODO: local validation (item existence/count) + UI feedback on failure
            C_USE_ITEM pkt = new C_USE_ITEM { ItemUid = s.RefId };
            NetworkManager.Instance.Send(pkt, (ushort)PacketManager.MsgId.C_USE_ITEM);
            return;
        }

        if (s.RefType == QuickSlotRefType.QsSkill)
        {
            int skillId = (int)s.RefId;

            // Client-side cooldown gate: don't send C_SKILL while cooldown active.
            if (SkillCooldownManager.Instance != null && SkillCooldownManager.Instance.IsOnCooldown(skillId))
            {
                Debug.Log($"[QuickSlot] Skill blocked by cooldown. skillId={skillId} slot={slotIndex}");
                return;
            }

            C_SKILL pkt = new C_SKILL();
            pkt.SkillId = skillId;

            // optional fields (present in your proto)
            if (myTransform != null)
                pkt.CastYaw = myTransform.eulerAngles.y;
            pkt.ClientTimeMs = (uint)(Time.realtimeSinceStartupAsDouble * 1000.0);

            NetworkManager.Instance.Send(pkt, (ushort)PacketManager.MsgId.C_SKILL);
            return;
        }
    }

    // ===================== Internals =====================

    void HandleQuickSlotList(RepeatedField<QuickSlotInfo> slots)
    {
        ResetToEmpty();
        if (slots == null) return;

        foreach (var s in slots)
        {
            if (s == null) continue;
            if (s.SlotIndex < 0 || s.SlotIndex >= MaxSlots) continue;
            ApplyLocal(s.SlotIndex, s.RefType, s.RefId, notify: false);
        }

        NotifyUpdated();
    }

    void HandleQuickSlotChanged(QuickSlotInfo slot)
    {
        if (slot == null) return;
        if (slot.SlotIndex < 0 || slot.SlotIndex >= MaxSlots) return;

        ApplyLocal(slot.SlotIndex, slot.RefType, slot.RefId);
    }

    void ApplyLocal(int idx, QuickSlotRefType t, ulong id, bool notify = true)
    {
        if (_slots[idx] == null) _slots[idx] = new QuickSlotInfo();

        _slots[idx].SlotIndex = idx;
        _slots[idx].RefType = (id == 0) ? QuickSlotRefType.QsNone : t;
        _slots[idx].RefId = id;

        if (notify)
            NotifyUpdated();
    }

    void NotifyUpdated()
    {
        OnUpdated?.Invoke();
    }
}
