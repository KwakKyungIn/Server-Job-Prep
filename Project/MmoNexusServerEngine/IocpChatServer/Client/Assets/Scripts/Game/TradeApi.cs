using Protocol;

public static class TradeApi
{
    public static void RequestTrade(ulong targetPlayerId)
    {
        var pkt = new C_TRADE_REQ { TargetPlayerId = targetPlayerId };
        NetworkManager.Instance.Send(pkt, (ushort)PacketManager.MsgId.C_TRADE_REQ);
    }

    public static void RespondInvite(bool accept)
    {
        var pkt = new C_TRADE_INVITE_RESP { Accept = accept };
        NetworkManager.Instance.Send(pkt, (ushort)PacketManager.MsgId.C_TRADE_INVITE_RESP);
    }

    public static void OfferSet(ulong tradeId, ulong itemUid, int count)
    {
        var pkt = new C_TRADE_OFFER_SET
        {
            TradeId = tradeId,
            ItemUid = itemUid,
            Count = count,
        };
        NetworkManager.Instance.Send(pkt, (ushort)PacketManager.MsgId.C_TRADE_OFFER_SET);
    }

    public static void SetReady(ulong tradeId, bool ready)
    {
        var pkt = new C_TRADE_READY { TradeId = tradeId, Ready = ready };
        NetworkManager.Instance.Send(pkt, (ushort)PacketManager.MsgId.C_TRADE_READY);
    }

    public static void Confirm(ulong tradeId)
    {
        var pkt = new C_TRADE_CONFIRM { TradeId = tradeId };
        NetworkManager.Instance.Send(pkt, (ushort)PacketManager.MsgId.C_TRADE_CONFIRM);
    }

    public static void Cancel(ulong tradeId, TradeCancelReason reason = TradeCancelReason.TradeCancelBySelf)
    {
        var pkt = new C_TRADE_CANCEL { TradeId = tradeId, Reason = reason };
        NetworkManager.Instance.Send(pkt, (ushort)PacketManager.MsgId.C_TRADE_CANCEL);
    }
}
