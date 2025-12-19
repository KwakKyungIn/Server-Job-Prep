using Protocol;

public static class PartyApi
{
    public static void Create()
    {
        var pkt = new C_PARTY_CREATE_REQ { TargetPlayerId = 0 };
        NetworkManager.Instance.Send(pkt, (ushort)PacketManager.MsgId.C_PARTY_CREATE_REQ);
    }

    public static void Invite(ulong targetPlayerId)
    {
        var pkt = new C_PARTY_INVITE_REQ { TargetPlayerId = targetPlayerId };
        NetworkManager.Instance.Send(pkt, (ushort)PacketManager.MsgId.C_PARTY_INVITE_REQ);
    }

    public static void AcceptInvite(ulong partyId, bool accept)
    {
        var pkt = new C_PARTY_INVITE_ACCEPT_REQ { PartyId = partyId, Accept = accept };
        NetworkManager.Instance.Send(pkt, (ushort)PacketManager.MsgId.C_PARTY_INVITE_ACCEPT_REQ);
    }

    public static void Leave()
    {
        var pkt = new C_PARTY_LEAVE_REQ();
        NetworkManager.Instance.Send(pkt, (ushort)PacketManager.MsgId.C_PARTY_LEAVE_REQ);
    }

    public static void Kick(ulong targetPlayerId)
    {
        var pkt = new C_PARTY_KICK_REQ { TargetPlayerId = targetPlayerId };
        NetworkManager.Instance.Send(pkt, (ushort)PacketManager.MsgId.C_PARTY_KICK_REQ);
    }

    public static void Disband()
    {
        var pkt = new C_PARTY_DISBAND_REQ();
        NetworkManager.Instance.Send(pkt, (ushort)PacketManager.MsgId.C_PARTY_DISBAND_REQ);
    }

    public static void RequestStatus()
    {
        var pkt = new C_PARTY_STATUS_REQ();
        NetworkManager.Instance.Send(pkt, (ushort)PacketManager.MsgId.C_PARTY_STATUS_REQ);
    }

    public static void SendPartyChat(string msg)
    {
        var pkt = new C_PARTY_CHAT_REQ { Message = msg };
        NetworkManager.Instance.Send(pkt, (ushort)PacketManager.MsgId.C_PARTY_CHAT_REQ);
    }
}
