using UnityEngine;
using UnityEngine.UI;
using TMPro;

public class UI_EscMenu : MonoBehaviour
{
    [Header("Root")]
    public GameObject root;

    [Header("Buttons")]
    public Button btnExit;
    public Button btnChannelChange;

    [Header("Panels")]
    public UI_ChannelChangePanel channelPanel;

    [Header("External UI")]
    public UI_PanelToggle panelToggle; // ✅ NEW: 인벤/파티 토글 스크립트 연결

    [Header("Optional Text")]
    public TMP_Text statusText;

    void Awake()
    {
        if (root != null)
            root.SetActive(false);

        if (btnExit != null)
        {
            btnExit.onClick.RemoveAllListeners();
            btnExit.onClick.AddListener(OnExit);
        }

        if (btnChannelChange != null)
        {
            btnChannelChange.onClick.RemoveAllListeners();
            btnChannelChange.onClick.AddListener(OpenChannelPanel);
        }

        if (channelPanel != null)
            channelPanel.Hide();
    }

    void Update()
    {
        // 맵 체인지 중엔 메뉴/패널 전부 닫기
        if (NetworkManager.Instance != null && NetworkManager.Instance.IsMapChanging)
        {
            if (root != null && root.activeSelf)
                root.SetActive(false);

            if (channelPanel != null)
                channelPanel.Hide();

            return;
        }

        if (Input.GetKeyDown(KeyCode.Escape))
        {
            // ✅ 1) 채널 패널이 열려있으면 먼저 닫기 (한 단계 뒤로)
            if (channelPanel != null && channelPanel.IsOpen)
            {
                channelPanel.Hide();
                return;
            }

            // ✅ 2) 인벤/파티가 열려있으면 하나만 닫기
            if (panelToggle != null && panelToggle.CloseOneByEsc())
                return;

            // ✅ 3) 그 외에는 ESC 메뉴 토글
            Toggle();
        }
    }

    void Toggle()
    {
        if (root == null) return;

        bool next = !root.activeSelf;
        root.SetActive(next);

        if (next && channelPanel != null)
            channelPanel.Hide();
    }

    void OpenChannelPanel()
    {
        if (NetworkManager.Instance == null) return;
        if (NetworkManager.Instance.IsMapChanging) return;

        if (channelPanel == null)
        {
            Debug.LogWarning("[UI_EscMenu] channelPanel is null (assign in inspector).");
            return;
        }

        channelPanel.Show(statusText);
    }

    void OnExit()
    {
        if (NetworkManager.Instance != null)
            NetworkManager.Instance.Disconnect();

#if UNITY_EDITOR
        UnityEditor.EditorApplication.isPlaying = false;
#else
        Application.Quit();
#endif
    }
}
