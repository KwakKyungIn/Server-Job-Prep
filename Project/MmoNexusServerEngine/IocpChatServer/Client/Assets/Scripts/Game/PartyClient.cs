using System;
using System.Collections.Generic;
using UnityEngine;
using Protocol;

public class PartyClient : MonoBehaviour
{
    public static PartyClient Instance { get; private set; }

    public bool HasParty { get; private set; }
    public ulong PartyId { get; private set; }
    public ulong LeaderId { get; private set; }
    public uint Version { get; private set; }

    public readonly List<ulong> MemberIds = new List<ulong>();
    public readonly Dictionary<ulong, PartyMemberStatus> Members = new Dictionary<ulong, PartyMemberStatus>();

    public event Action OnPartyChanged;
    public event Action<S_PARTY_INVITE_NTF> OnInviteArrived;
    public event Action<S_PARTY_RESULT> OnPartyResult;

    public ulong MyId => ObjectManager.MyPlayerId;
    public bool IsLeader => HasParty && MyId != 0 && LeaderId == MyId;

    void Awake()
    {
        if (Instance == null) { Instance = this; DontDestroyOnLoad(gameObject); }
        else Destroy(gameObject);
    }

    void OnEnable()
    {
        PacketHandler.OnPartyInfoNtf += HandlePartyInfo;
        PacketHandler.OnPartyStatusNtf += HandlePartyStatus;
        PacketHandler.OnPartyInviteNtf += HandlePartyInvite;
        PacketHandler.OnPartyResult += HandlePartyResult;
    }

    void OnDisable()
    {
        PacketHandler.OnPartyInfoNtf -= HandlePartyInfo;
        PacketHandler.OnPartyStatusNtf -= HandlePartyStatus;
        PacketHandler.OnPartyInviteNtf -= HandlePartyInvite;
        PacketHandler.OnPartyResult -= HandlePartyResult;
    }

    void HandlePartyInfo(S_PARTY_INFO_NTF pkt)
    {
        // 버전 역전 방지(선택)
        if (pkt.Version < Version && pkt.HasParty == HasParty && pkt.PartyId == PartyId)
            return;

        HasParty = pkt.HasParty;
        PartyId = pkt.PartyId;
        LeaderId = pkt.LeaderId;
        Version = pkt.Version;

        MemberIds.Clear();
        Members.Clear();

        if (HasParty)
            foreach (var id in pkt.MemberIds)
                MemberIds.Add(id);

        OnPartyChanged?.Invoke();
    }

    void HandlePartyStatus(S_PARTY_STATUS_NTF pkt)
    {
        Debug.Log($"[HandlePartyStatus] PartyId={pkt.PartyId}, HasParty={HasParty}");

        if (pkt.PartyId == 0)
        {
            ClearParty();
            return;
        }

        // ✅ STATUS만 와도 파티 복원
        if (!HasParty || pkt.PartyId != PartyId)
        {
            Debug.Log($"[HandlePartyStatus] Restoring party: {pkt.PartyId}");
            HasParty = true;
            PartyId = pkt.PartyId;
        }

        if (pkt.Version < Version) return;
        Version = pkt.Version;

        Members.Clear();
        foreach (var m in pkt.Members)
            Members[m.PlayerId] = m;

        MemberIds.Clear();
        foreach (var kv in Members)
            MemberIds.Add(kv.Key);

        OnPartyChanged?.Invoke();
    }

    void HandlePartyInvite(S_PARTY_INVITE_NTF pkt) => OnInviteArrived?.Invoke(pkt);
    void HandlePartyResult(S_PARTY_RESULT pkt) => OnPartyResult?.Invoke(pkt);

    public void ClearParty()
    {
        HasParty = false;
        PartyId = 0;
        LeaderId = 0;
        Version = 0;
        MemberIds.Clear();
        Members.Clear();
        OnPartyChanged?.Invoke();
    }
}
