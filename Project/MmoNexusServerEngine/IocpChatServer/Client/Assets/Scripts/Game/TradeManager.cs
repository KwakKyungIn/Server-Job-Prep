using System;
using System.Collections.Generic;
using Protocol;
using UnityEngine;

public class TradeManager
{
    static TradeManager _instance;
    public static TradeManager Instance => _instance ??= new TradeManager();

    public bool InTrade => TradeId != 0;
    public bool Locked { get; private set; }

    public ulong TradeId { get; private set; }
    public ulong PeerId { get; private set; }
    public string PeerName { get; private set; } = "";

    public bool MyReady { get; private set; }
    public bool PeerReady { get; private set; }

    // Sequential display order (server sends as map iteration order; treat as unordered)
    public readonly List<TradeOfferItem> MyOffer = new List<TradeOfferItem>();
    public readonly List<TradeOfferItem> PeerOffer = new List<TradeOfferItem>();

    // UI hooks
    public event Action<S_TRADE_INVITE> OnInviteArrived;
    public event Action OnTradeStarted;
    public event Action OnTradeUpdated;
    public event Action OnTradeEnded;
    public event Action<S_TRADE_RESULT> OnTradeResult;

    enum PendingRole { None, A, B }
    PendingRole _pendingRole = PendingRole.None;
    bool _amA = true;

    bool _inited = false;

    public void Init()
    {
        if (_inited) return;
        _inited = true;

        PacketHandler.OnTradeInvite += HandleInvite;
        PacketHandler.OnTradeStart += HandleStart;
        PacketHandler.OnTradeOfferUpdate += HandleOfferUpdate;
        PacketHandler.OnTradeReadyState += HandleReadyState;
        PacketHandler.OnTradeLocked += HandleLocked;
        PacketHandler.OnTradeCancelled += HandleCancelled;
        PacketHandler.OnTradeResult += HandleResult;

        // If we start a map change, trade UI should go away immediately on client-side.
        PacketHandler.OnMapChangeBegin += _ => ResetLocal();
    }

    // ================== Public API ==================

    public void RequestTrade(ulong targetPlayerId)
    {
        if (targetPlayerId == 0) return;
        if (InTrade) return;

        _pendingRole = PendingRole.A;
        TradeApi.RequestTrade(targetPlayerId);
    }

    public void RespondInvite(bool accept)
    {
        // When invited, we are B
        _pendingRole = PendingRole.B;
        TradeApi.RespondInvite(accept);
    }

    public bool TryOfferItem(ulong itemUid, int count = -1)
    {
        if (!InTrade || Locked) return false;
        if (itemUid == 0) return false;

        var it = InventoryManager.Instance.GetItemByUid(itemUid);
        if (it == null) return false;
        if (it.IsEquipped) return false;

        int sendCount = count;
        if (sendCount <= 0)
            sendCount = it.Count;

        if (sendCount <= 0 || sendCount > it.Count)
            return false;

        TradeApi.OfferSet(TradeId, itemUid, sendCount);
        return true;
    }

    public void RemoveOffer(ulong itemUid)
    {
        if (!InTrade || Locked) return;
        if (itemUid == 0) return;
        TradeApi.OfferSet(TradeId, itemUid, 0); // server: count<=0 => remove
    }

    public void SetReady(bool ready)
    {
        if (!InTrade) return;
        if (Locked) return; // locked state -> confirm stage
        TradeApi.SetReady(TradeId, ready);
    }

    public void Confirm()
    {
        if (!InTrade) return;
        if (!Locked) return;
        TradeApi.Confirm(TradeId);
    }

    public void Cancel(TradeCancelReason reason = TradeCancelReason.TradeCancelBySelf)
    {
        if (!InTrade) return;
        TradeApi.Cancel(TradeId, reason);
    }

    public void ResetLocal()
    {
        if (!InTrade)
            return;

        // Do not send cancel here (server will cancel on map change anyway). Just clean UI quickly.
        ClearState();
        OnTradeEnded?.Invoke();
    }

    // ================== Packet Handlers ==================

    void HandleInvite(S_TRADE_INVITE pkt)
    {
        if (InTrade)
        {
            // Already trading: ignore invite UI.
            return;
        }

        _pendingRole = PendingRole.B;
        OnInviteArrived?.Invoke(pkt);
    }

    void HandleStart(S_TRADE_START pkt)
    {
        TradeId = pkt.TradeId;
        PeerId = pkt.PeerId;
        PeerName = pkt.PeerName;

        _amA = _pendingRole != PendingRole.B; // default A if not explicitly invited
        _pendingRole = PendingRole.None;

        Locked = false;
        MyReady = false;
        PeerReady = false;
        MyOffer.Clear();
        PeerOffer.Clear();

        OnTradeStarted?.Invoke();
        OnTradeUpdated?.Invoke();
    }

    void HandleOfferUpdate(S_TRADE_OFFER_UPDATE pkt)
    {
        if (!InTrade || pkt.TradeId != TradeId) return;

        bool isMine = pkt.WhoPlayerId == ObjectManager.MyPlayerId;
        var dst = isMine ? MyOffer : PeerOffer;

        dst.Clear();
        foreach (var e in pkt.Items)
            dst.Add(e);

        // Offer change resets ready state on server -> UI should update quickly.
        OnTradeUpdated?.Invoke();
    }

    void HandleReadyState(S_TRADE_READY_STATE pkt)
    {
        if (!InTrade || pkt.TradeId != TradeId) return;

        bool aReady = pkt.AReady;
        bool bReady = pkt.BReady;

        MyReady = _amA ? aReady : bReady;
        PeerReady = _amA ? bReady : aReady;

        OnTradeUpdated?.Invoke();
    }

    void HandleLocked(S_TRADE_LOCKED pkt)
    {
        if (!InTrade || pkt.TradeId != TradeId) return;

        Locked = true;
        OnTradeUpdated?.Invoke();
    }

    void HandleCancelled(S_TRADE_CANCELLED pkt)
    {
        if (!InTrade || pkt.TradeId != TradeId) return;

        ClearState();
        OnTradeEnded?.Invoke();
    }

    void HandleResult(S_TRADE_RESULT pkt)
    {
        // Result can come after cancelled too. Always forward first.
        OnTradeResult?.Invoke(pkt);

        if (InTrade && pkt.TradeId == TradeId)
        {
            if (pkt.Success)
            {
                // Inventory will be updated via S_CHANGE_ITEM/S_REMOVE_ITEM streams.
            }
            ClearState();
            OnTradeEnded?.Invoke();
        }
    }

    void ClearState()
    {
        TradeId = 0;
        PeerId = 0;
        PeerName = "";
        Locked = false;
        MyReady = false;
        PeerReady = false;
        MyOffer.Clear();
        PeerOffer.Clear();
        _pendingRole = PendingRole.None;
    }
}
