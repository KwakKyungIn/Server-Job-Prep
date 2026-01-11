using UnityEngine;
using UnityEngine.UI;
using TMPro;

public class UI_EscMenu : MonoBehaviour
{
    [Header("Root")]
    public GameObject root;

    [Header("Buttons")]
    public Button btnExit;
    public Button btnChannelChange;            // ✅ NEW: "채널 변경" 버튼

    [Header("Panels")]
    public UI_ChannelChangePanel channelPanel; // ✅ NEW: 채널 1/2/3 패널 스크립트

    [Header("Optional Text")]
    public TMP_Text statusText;

    void Awake()
    {
        if (root != null)
            root.SetActive(false);

        // ✅ 중복 리스너 방지
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

        // 시작 시 채널 패널은 닫아두기
        if (channelPanel != null)
            channelPanel.Hide();
    }

    void Update()
    {
        // 맵 체인지 중엔 메뉴/패널 전부 닫기 (꼬임 방지)
        if (NetworkManager.Instance != null && NetworkManager.Instance.IsMapChanging)
        {
            if (root != null && root.activeSelf)
                root.SetActive(false);

            if (channelPanel != null)
                channelPanel.Hide();

            return;
        }

        if (Input.GetKeyDown(KeyCode.Escape))
            Toggle();
    }

    void Toggle()
    {
        if (root == null) return;

        bool next = !root.activeSelf;
        root.SetActive(next);

        // 메뉴 열릴 때 채널 패널은 기본 닫힘
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

        channelPanel.Show(statusText); // statusText를 넘겨서 같은 텍스트로 표시하고 싶으면 사용
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
