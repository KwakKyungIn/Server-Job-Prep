using UnityEngine;
using UnityEngine.UI;
using TMPro;
using Protocol;
using UnityEngine.EventSystems;

public class UI_ChatPanel : MonoBehaviour
{
    [Header("UI Refs")]
    public TMP_Text chatLog;                 // ChatLog
    public TMP_InputField chatInput;         // ChatInput
    public Button sendButton;                // SendButton

    [Header("Root (ChatBox)")]
    public Transform chatRoot;               // ChatBox 루트(자식 판별용)
    public CanvasGroup group;                // ChatBox에 붙은 CanvasGroup

    [Header("Alpha")]
    [Range(0f, 1f)] public float inactiveAlpha = 0.25f;
    [Range(0f, 1f)] public float activeAlpha = 1.0f;

    [Header("Scroll")]
    public UnityEngine.UI.ScrollRect scrollRect;
    bool _forceToBottom = true;


    [Header("Log Limit")]
    public int maxLines = 50;

    bool _active = false;

    public static bool IsTyping { get; private set; } = false;

    void Awake()
    {
        if (sendButton)
        {
            sendButton.onClick.RemoveAllListeners();
            sendButton.onClick.AddListener(() =>
            {
                // 활성 상태에서만 전송
                if (_active) Send();
                else ActivateChat(); // 혹시 버튼 눌러도 켜지게
            });
        }

        // 시작은 비활성(반투명)
        DeactivateChat(clearInput: false);
    }

    void OnEnable()
    {
        PacketHandler.OnChatMsg += OnChatMsg;
        PacketHandler.OnChatRes += OnChatRes;
    }

    void OnDisable()
    {
        PacketHandler.OnChatMsg -= OnChatMsg;
        PacketHandler.OnChatRes -= OnChatRes;

        IsTyping = false;
    }

    void Update()
    {
        // Enter: 비활성이면 활성화 / 활성 상태면 Send
        if (IsEnterDown())
        {
            if (!_active)
            {
                ActivateChat();
                return;
            }

            // 활성 상태에서 Enter면 전송 (빈 문자열이면 전송 안 함)
            Send();
            return;
        }

        // ESC: 활성 상태면 비활성화 + 입력 지움
        if (_active && Input.GetKeyDown(KeyCode.Escape))
        {
            DeactivateChat(clearInput: true);
            return;
        }

        // 활성 상태인데 포커스가 채팅 UI 밖으로 나가면 비활성화(클릭 다른 곳)
        if (_active && HasFocusLeftChat())
        {
            DeactivateChat(clearInput: false);
            return;
        }
    }

    bool IsEnterDown()
    {
        return Input.GetKeyDown(KeyCode.Return) || Input.GetKeyDown(KeyCode.KeypadEnter);
    }

    void ActivateChat()
    {
        _active = true;
        IsTyping = true;

        if (group != null)
        {
            group.alpha = activeAlpha;
            group.interactable = true;
            group.blocksRaycasts = true;
        }

        if (chatInput != null)
        {
            // 기존 텍스트 유지한 채로 포커스만 잡기
            chatInput.ActivateInputField();
            chatInput.Select();
        }
    }

    void DeactivateChat(bool clearInput)
    {
        _active = false;
        IsTyping = false;

        if (clearInput && chatInput != null)
            chatInput.text = "";

        // 선택 해제(포커스 제거)
        if (EventSystem.current != null)
            EventSystem.current.SetSelectedGameObject(null);

        if (group != null)
        {
            group.alpha = inactiveAlpha;
            group.interactable = true;      // ✅ 반투명 상태에서도 스크롤/선택 가능하게 하고 싶으면 true
            group.blocksRaycasts = false;    // ✅ 클릭 막고 싶으면 false로 바꿔
        }
    }

    bool HasFocusLeftChat()
    {
        // 1) 입력 필드가 아직 포커스면 유지
        if (chatInput != null && chatInput.isFocused)
            return false;

        // 2) 현재 선택된 오브젝트가 ChatBox 내부면 유지
        if (EventSystem.current == null) return false;
        var selected = EventSystem.current.currentSelectedGameObject;
        if (selected == null) return true; // 아무데도 선택 안 됨 = 대개 바깥 클릭

        if (chatRoot == null) return false; // 루트 미지정이면 판단 불가 -> 유지
        return !selected.transform.IsChildOf(chatRoot);
    }

    void Send()
    {
        if (chatInput == null) return;

        string msg = chatInput.text;
        if (string.IsNullOrWhiteSpace(msg))
        {
            // 빈 메시지는 그냥 포커스 유지
            chatInput.ActivateInputField();
            return;
        }

        var pkt = new C_CHAT_REQ { Message = msg };
        NetworkManager.Instance.Send(pkt, (ushort)PacketManager.MsgId.C_CHAT_REQ);

        chatInput.text = "";
        chatInput.ActivateInputField();
        chatInput.Select();
    }

    void OnChatRes(bool ok)
    {
        if (!ok) AppendLine("[System] Chat send failed");
    }

    void OnChatMsg(string msg)
    {
        AppendLine(msg);
    }

    void AppendLine(string line)
    {
        if (chatLog == null) return;

        chatLog.text += line + "\n";

        var lines = chatLog.text.Split('\n');
        if (lines.Length > maxLines + 1)
        {
            int start = lines.Length - (maxLines + 1);
            chatLog.text = string.Join("\n", lines, start, maxLines + 1);
        }

        // ✅ 레이아웃 갱신 후 맨 아래로
        if (scrollRect != null)
            StartCoroutine(CoScrollToBottom());
    }

    System.Collections.IEnumerator CoScrollToBottom()
    {
        // UI 레이아웃 반영 1~2프레임 기다렸다가 내리는 게 안전
        yield return null;
        yield return null;

        scrollRect.verticalNormalizedPosition = 0f; // 0 = bottom
    }

}
