using UnityEngine;
using UnityEngine.UI;
using TMPro;
using Protocol;

public class UI_ChannelChangePanel : MonoBehaviour
{
    [Header("Root")]
    public GameObject root;

    [Header("Buttons")]
    public Button btnClose;
    public Button btnCh1;
    public Button btnCh2;
    public Button btnCh3;

    [Header("Optional Text")]
    public TMP_Text statusText;

    TMP_Text _externalStatusText; // EscMenu의 statusText를 넘겨받으면 같이 갱신용

    public bool IsOpen => root != null && root.activeSelf;

    void Awake()
    {
        if (root != null)
            root.SetActive(false);

        // ✅ 중복 리스너 방지
        if (btnClose != null)
        {
            btnClose.onClick.RemoveAllListeners();
            btnClose.onClick.AddListener(Hide);
        }

        BindChannelButton(btnCh1, 1);
        BindChannelButton(btnCh2, 2);
        BindChannelButton(btnCh3, 3);
    }

    void BindChannelButton(Button btn, int channelId)
    {
        if (btn == null) return;
        btn.onClick.RemoveAllListeners();
        btn.onClick.AddListener(() => RequestChannelChange(channelId));
    }

    public void Show(TMP_Text externalStatusText = null)
    {
        _externalStatusText = externalStatusText;

        if (NetworkManager.Instance != null && NetworkManager.Instance.IsMapChanging)
            return;

        if (root != null)
            root.SetActive(true);
    }

    public void Hide()
    {
        if (root != null)
            root.SetActive(false);
    }

    void RequestChannelChange(int targetChannelId)
    {
        if (NetworkManager.Instance == null) return;
        if (NetworkManager.Instance.IsMapChanging) return;

        var req = new C_CHANNEL_CHANGE_REQ { TargetChannelId = targetChannelId };
        NetworkManager.Instance.Send(req, (ushort)PacketManager.MsgId.C_CHANNEL_CHANGE_REQ);

        // 텍스트 갱신(내 패널 텍스트 + EscMenu 텍스트 둘 다)
        statusText?.SetText($"Switching to Channel {targetChannelId}...");
        _externalStatusText?.SetText($"Switching to Channel {targetChannelId}...");

        // 연타 방지: 패널 닫기
        Hide();

        // 즉시 로딩 감각
        var overlay = FindObjectOfType<UI_LoadingOverlay>(true);
        overlay?.Show($"Switching to Channel {targetChannelId}...");
    }
}
