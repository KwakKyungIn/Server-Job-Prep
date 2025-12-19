using UnityEngine;
using UnityEngine.UI;
using Protocol;
using TMPro;
public class UI_PartyInvitePopup : MonoBehaviour
{
    public GameObject root;
    public TMP_Text msgText;
    public Button btnAccept;
    public Button btnReject;

    ulong _partyId;

    void Start()
    {
        if (root != null) root.SetActive(false);

        PartyClient.Instance.OnInviteArrived += Show;

        btnAccept.onClick.AddListener(() => { PartyApi.AcceptInvite(_partyId, true); Hide(); });
        btnReject.onClick.AddListener(() => { PartyApi.AcceptInvite(_partyId, false); Hide(); });
    }

    void OnDestroy()
    {
        if (PartyClient.Instance != null)
            PartyClient.Instance.OnInviteArrived -= Show;
    }

    void Show(S_PARTY_INVITE_NTF pkt)
    {
        _partyId = pkt.PartyId;

        if (msgText != null)
            msgText.text = $"{pkt.InviterName}({pkt.InviterId}) invited you!\nPartyId={pkt.PartyId}";

        if (root != null) root.SetActive(true);
    }

    void Hide()
    {
        if (root != null) root.SetActive(false);
        _partyId = 0;
    }
}
