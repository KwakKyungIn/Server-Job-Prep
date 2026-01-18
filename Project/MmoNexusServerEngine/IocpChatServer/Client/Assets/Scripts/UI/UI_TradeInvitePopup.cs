using UnityEngine;
using UnityEngine.UI;
using Protocol;
using TMPro;

public class UI_TradeInvitePopup : MonoBehaviour
{
    public GameObject root;
    public TMP_Text msgText;
    public Button btnAccept;
    public Button btnReject;

    S_TRADE_INVITE _last;

    void Awake()
    {
        TradeManager.Instance.Init(); // 안전장치 (중복 Init 방지)

        TradeManager.Instance.OnInviteArrived += Show;

        btnAccept.onClick.RemoveAllListeners();
        btnReject.onClick.RemoveAllListeners();

        btnAccept.onClick.AddListener(() =>
        {
            TradeManager.Instance.RespondInvite(true);
            Hide();
        });

        btnReject.onClick.AddListener(() =>
        {
            TradeManager.Instance.RespondInvite(false);
            Hide();
        });

        if (root != null) root.SetActive(false);
    }

    void OnDestroy()
    {
        if (TradeManager.Instance != null)
            TradeManager.Instance.OnInviteArrived -= Show;
    }

    void Show(S_TRADE_INVITE pkt)
    {
        _last = pkt;
        if (msgText != null)
            msgText.text = $"{pkt.FromName}({pkt.FromPlayerId}) wants to trade.";

        if (root != null) root.SetActive(true);
    }

    void Hide()
    {
        if (root != null) root.SetActive(false);
        _last = null;
    }
}
