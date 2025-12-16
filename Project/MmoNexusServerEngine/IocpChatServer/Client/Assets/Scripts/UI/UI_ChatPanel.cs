using UnityEngine;
using UnityEngine.UI;
using TMPro;
using Protocol;

public class UI_ChatPanel : MonoBehaviour
{
    public TMP_Text chatLog;          // ChatLog
    public TMP_InputField chatInput;  // ChatInput
    public Button sendButton;         // SendButton

    [Header("Log Limit")]
    public int maxLines = 50;

    void Awake()
    {
        if (sendButton) sendButton.onClick.AddListener(Send);
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
    }

    void Update()
    {
        if (chatInput != null && chatInput.isFocused && Input.GetKeyDown(KeyCode.Return))
            Send();
    }

    void Send()
    {
        if (chatInput == null) return;

        string msg = chatInput.text;
        if (string.IsNullOrWhiteSpace(msg)) return;

        C_CHAT_REQ pkt = new C_CHAT_REQ();
        pkt.Message = msg;

        NetworkManager.Instance.Send(pkt, (ushort)PacketManager.MsgId.C_CHAT_REQ);

        chatInput.text = "";
        chatInput.ActivateInputField();
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

        // 라인 수 제한(로그 폭발 방지)
        var lines = chatLog.text.Split('\n');
        if (lines.Length > maxLines + 1)
        {
            int start = lines.Length - (maxLines + 1);
            chatLog.text = string.Join("\n", lines, start, maxLines + 1);
        }
    }
}
