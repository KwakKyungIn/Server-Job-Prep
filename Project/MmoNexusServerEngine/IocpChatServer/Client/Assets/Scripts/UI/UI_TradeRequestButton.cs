using TMPro;
using UnityEngine;
using UnityEngine.UI;

// Simple helper for testing: wire a UI button to send a trade request.
public class UI_TradeRequestButton : MonoBehaviour
{
    public Button button;
    public TMP_InputField targetPlayerIdInput; // "12345" like

    void Awake()
    {
        if (button == null)
            button = GetComponent<Button>();

        if (button != null)
        {
            button.onClick.RemoveAllListeners();
            button.onClick.AddListener(() =>
            {
                if (targetPlayerIdInput == null) return;
                if (ulong.TryParse(targetPlayerIdInput.text, out ulong pid))
                    TradeManager.Instance.RequestTrade(pid);
            });
        }
    }
}
