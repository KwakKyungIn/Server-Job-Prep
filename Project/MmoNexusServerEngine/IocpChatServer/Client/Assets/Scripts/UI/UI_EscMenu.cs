using UnityEngine;
using UnityEngine.UI;
using TMPro;
using Protocol;

public class UI_EscMenu : MonoBehaviour
{
    [Header("Root")]
    public GameObject root;

    [Header("Buttons")]
    public Button btnResume;
    public Button btnExit;
    public Button btnCh1;
    public Button btnCh2;
    public Button btnCh3;

    [Header("Optional Text")]
    public TMP_Text statusText;

    void Awake()
    {
        if (root != null)
            root.SetActive(false);

        // 버튼 바인딩 (인스펙터 안 꽂아도 최소 동작)
        if (btnResume != null) btnResume.onClick.AddListener(OnResume);
        if (btnExit != null) btnExit.onClick.AddListener(OnExit);

        if (btnCh1 != null) btnCh1.onClick.AddListener(() => RequestChannelChange(1));
        if (btnCh2 != null) btnCh2.onClick.AddListener(() => RequestChannelChange(2));
        if (btnCh3 != null) btnCh3.onClick.AddListener(() => RequestChannelChange(3));
    }

    void Update()
    {
        // 맵 체인지 중엔 메뉴 자체를 닫아버림 (꼬임 방지)
        if (NetworkManager.Instance != null && NetworkManager.Instance.IsMapChanging)
        {
            if (root != null && root.activeSelf)
                root.SetActive(false);
            return;
        }

        if (Input.GetKeyDown(KeyCode.Escape))
            Toggle();
    }

    void Toggle()
    {
        if (root == null) return;
        root.SetActive(!root.activeSelf);
    }

    void OnResume()
    {
        if (root != null)
            root.SetActive(false);
    }

    void OnExit()
    {
        // 테스트 정책: 소켓 끊고 앱 종료
        if (NetworkManager.Instance != null)
            NetworkManager.Instance.Disconnect();

#if UNITY_EDITOR
        UnityEditor.EditorApplication.isPlaying = false;
#else
        Application.Quit();
#endif
    }

    void RequestChannelChange(int targetChannelId)
    {
        if (NetworkManager.Instance == null) return;
        if (NetworkManager.Instance.IsMapChanging) return;

        var req = new C_CHANNEL_CHANGE_REQ { TargetChannelId = targetChannelId };
        NetworkManager.Instance.Send(req, (ushort)PacketManager.MsgId.C_CHANNEL_CHANGE_REQ);

        statusText?.SetText($"Switching to Channel {targetChannelId}...");

        // 사용자가 연타 못 하게 닫아버림
        if (root != null) root.SetActive(false);

        // (선택) 서버 BEGIN 오기 전까지도 “즉시” 로딩 감각 주고 싶으면:
        var overlay = FindObjectOfType<UI_LoadingOverlay>(true);
        overlay?.Show($"Switching to Channel {targetChannelId}...");
    }
}
