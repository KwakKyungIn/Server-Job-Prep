using TMPro;
using UnityEngine;
using UnityEngine.UI;
using Protocol;

public class UI_TradePanel : MonoBehaviour
{
    [Header("Root")]
    public GameObject root;

    [Header("Texts")]
    public TMP_Text titleText;
    public TMP_Text stateText;

    [Header("Slots")]
    public UI_TradeOfferSlot[] mySlots;
    public UI_TradeOfferSlot[] peerSlots;

    [Header("Buttons")]
    public Button btnReady;
    public TMP_Text btnReadyText;
    public Button btnConfirm;
    public Button btnCancel;

    void Awake()
    {
        TradeManager.Instance.Init();

        TradeManager.Instance.OnTradeStarted += OnTradeStarted;
        TradeManager.Instance.OnTradeUpdated += Refresh;
        TradeManager.Instance.OnTradeEnded += OnTradeEnded;
        TradeManager.Instance.OnTradeResult += OnTradeResult;

        if (btnReady != null)
        {
            btnReady.onClick.RemoveAllListeners();
            btnReady.onClick.AddListener(() =>
            {
                if (!TradeManager.Instance.InTrade) return;
                if (TradeManager.Instance.Locked) return;
                TradeManager.Instance.SetReady(!TradeManager.Instance.MyReady);
            });
        }

        if (btnConfirm != null)
        {
            btnConfirm.onClick.RemoveAllListeners();
            btnConfirm.onClick.AddListener(() =>
            {
                if (!TradeManager.Instance.InTrade) return;
                TradeManager.Instance.Confirm();
            });
        }

        if (btnCancel != null)
        {
            btnCancel.onClick.RemoveAllListeners();
            btnCancel.onClick.AddListener(() =>
            {
                if (!TradeManager.Instance.InTrade) return;
                TradeManager.Instance.Cancel(TradeCancelReason.TradeCancelBySelf);
            });
        }

        if (root != null) root.SetActive(false);
    }

    void OnDestroy()
    {
        var tm = TradeManager.Instance;
        if (tm != null)
        {
            tm.OnTradeStarted -= OnTradeStarted;
            tm.OnTradeUpdated -= Refresh;
            tm.OnTradeEnded -= OnTradeEnded;
            tm.OnTradeResult -= OnTradeResult;
        }
    }

    void OnTradeStarted()
    {
        if (root != null) root.SetActive(true);
        Refresh();
    }

    void OnTradeEnded()
    {
        if (root != null) root.SetActive(false);
        ClearSlots();
    }

    void OnTradeResult(S_TRADE_RESULT pkt)
    {
        if (pkt == null) return;
        if (stateText != null)
        {
            if (pkt.Success)
                stateText.text = "Trade Success!";
            else
                stateText.text = $"Trade Failed: {pkt.FailCode} ({pkt.Msg})";
        }
    }

    void Refresh()
    {
        var tm = TradeManager.Instance;
        if (!tm.InTrade)
        {
            if (root != null) root.SetActive(false);
            return;
        }

        if (titleText != null)
            titleText.text = $"Trading with {tm.PeerName}({tm.PeerId})";

        if (btnReadyText != null)
            btnReadyText.text = tm.MyReady ? "Ready: ON" : "Ready: OFF";

        if (btnConfirm != null)
            btnConfirm.interactable = tm.Locked; // Locked -> confirm stage

        if (btnReady != null)
            btnReady.interactable = !tm.Locked;

        if (stateText != null)
        {
            if (tm.Locked)
                stateText.text = "LOCKED - Press CONFIRM";
            else
                stateText.text = $"Ready: me={tm.MyReady} peer={tm.PeerReady}";
        }

        // Fill slots sequentially
        FillSlots(mySlots, tm.MyOffer, allowDrop: true);
        FillSlots(peerSlots, tm.PeerOffer, allowDrop: false);
    }

    void FillSlots(UI_TradeOfferSlot[] slots, System.Collections.Generic.List<TradeOfferItem> items, bool allowDrop)
    {
        if (slots == null) return;
        for (int i = 0; i < slots.Length; i++)
        {
            if (slots[i] == null) continue;
            slots[i].allowDrop = allowDrop;

            if (items != null && i < items.Count)
            {
                var e = items[i];
                slots[i].SetData(e.ItemUid, e.TemplateId, e.Count);
            }
            else
            {
                slots[i].Clear();
            }
        }
    }

    void ClearSlots()
    {
        if (mySlots != null)
            foreach (var s in mySlots)
                s?.Clear();
        if (peerSlots != null)
            foreach (var s in peerSlots)
                s?.Clear();
    }
}
