using System.Collections.Generic;
using UnityEngine;
using UnityEngine.UI;
using Protocol;
using TMPro;
public class UI_PartyPanel : MonoBehaviour
{
    public GameObject panelRoot;

    [Header("Header")]
    public TMP_Text headerText;

    [Header("Invite")]
    public TMP_InputField inviteTargetIdInput;
    public Button btnInvite;

    [Header("Buttons")]
    public Button btnCreate;
    public Button btnLeave;
    public Button btnDisband;
    public Button btnRefresh;

    [Header("Member List")]
    public Transform memberListRoot;
    public GameObject memberRowPrefab;

    readonly List<GameObject> _rows = new List<GameObject>();

    void Start()
    {
        //if (panelRoot != null) panelRoot.SetActive(false);

        btnCreate.onClick.AddListener(PartyApi.Create);
        btnLeave.onClick.AddListener(PartyApi.Leave);
        btnDisband.onClick.AddListener(PartyApi.Disband);
        btnRefresh.onClick.AddListener(PartyApi.RequestStatus);
        btnInvite.onClick.AddListener(OnInviteClicked);

        PartyClient.Instance.OnPartyChanged += Refresh;
        PartyClient.Instance.OnPartyResult += _ => Refresh();

        Refresh();
    }

    void OnDestroy()
    {
        if (PartyClient.Instance == null) return;
        PartyClient.Instance.OnPartyChanged -= Refresh;
    }

    void Update()
    {
        // ✅ K키: 파티창 토글 (주의: MyPlayerController에서 K 테스트키 제거해야 함)
        //if (Input.GetKeyDown(KeyCode.K))
          //  Toggle();
    }

    void Toggle()
    {
        if (panelRoot == null) return;

        bool next = !panelRoot.activeSelf;
        panelRoot.SetActive(next);

        if (next)
        {
            if (PartyClient.Instance.HasParty)
                PartyApi.RequestStatus();
            Refresh();
        }
    }

    void OnInviteClicked()
    {
        if (inviteTargetIdInput == null) return;
        if (!ulong.TryParse(inviteTargetIdInput.text, out ulong targetId))
        {
            Debug.LogWarning("[PartyUI] Invalid target id");
            return;
        }

        PartyApi.Invite(targetId);
    }

    void Refresh()
    {
        var pc = PartyClient.Instance;
        if (pc == null) return;

        if (headerText != null)
        {
            headerText.text = pc.HasParty
                ? (pc.IsLeader ? "Party: My Party" : $"Party: {pc.LeaderId}'s Party")
                : "Party:";
        }


        bool hasParty = pc.HasParty;
        bool isLeader = pc.IsLeader;

        btnCreate.gameObject.SetActive(!hasParty);
        btnLeave.gameObject.SetActive(hasParty);
        btnDisband.gameObject.SetActive(hasParty && isLeader);

        ClearRows();

        if (!hasParty) return;

        // Status 있으면 Status 기준
        if (pc.Members.Count > 0)
        {
            foreach (var kv in pc.Members)
                SpawnRow(kv.Value);
        }
        else
        {
            // Status 없으면 ID만 표시
            foreach (var id in pc.MemberIds)
                SpawnRowFallback(id);
        }
    }

    void SpawnRowFallback(ulong playerId)
    {
        if (memberRowPrefab == null || memberListRoot == null) return;

        var go = Instantiate(memberRowPrefab, memberListRoot);
        _rows.Add(go);

        var row = go.GetComponent<UI_PartyMemberRow>();
        if (row != null)
            row.BindFallback(playerId);
    }

    void SpawnRow(PartyMemberStatus m)
    {
        if (memberRowPrefab == null || memberListRoot == null) return;

        var go = Instantiate(memberRowPrefab, memberListRoot);
        _rows.Add(go);

        var row = go.GetComponent<UI_PartyMemberRow>();
        if (row != null)
            row.Bind(m);
    }

    void ClearRows()
    {
        foreach (var go in _rows) Destroy(go);
        _rows.Clear();
    }

    public void OnOpen()
    {
        // 패널이 켜진 시점에 상태 요청
        if (PartyClient.Instance != null && PartyClient.Instance.HasParty)
            PartyApi.RequestStatus();

        Refresh();
    }

}
