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

    void Awake()
    {
        // ✅ 구독은 Start 말고 Awake에서 (씬 로드시 바로)
        PartyClient.Instance.OnInviteArrived += Show;

        // ✅ 버튼 리스너 중복 방지
        btnAccept.onClick.RemoveAllListeners();
        btnReject.onClick.RemoveAllListeners();

        btnAccept.onClick.AddListener(() => { PartyApi.AcceptInvite(_partyId, true); Hide(); });
        btnReject.onClick.AddListener(() => { PartyApi.AcceptInvite(_partyId, false); Hide(); });

        // ✅ 시각 요소만 꺼두기 (스크립트 오브젝트 자체는 켜진 상태 유지)
        if (root != null) root.SetActive(false);
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
