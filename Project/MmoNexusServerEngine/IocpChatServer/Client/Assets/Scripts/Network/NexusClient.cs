using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using Google.Protobuf;
using Protocol;

public class NexusClient : MonoBehaviour
{
    bool _isGameEntered = false;

    string _inputName = "KwakPpiPpi";
    string _inputPw = "1234";
    string _inputChat = "";
    string _chatLog = "";
    Vector2 _scrollPos;

    string _loginToken = "";

    void Start()
    {
        PacketHandler.OnLogin += HandleLogin;
        PacketHandler.OnEnterGame += HandleEnterGame;
        PacketHandler.OnChatRes += HandleChatRes;
    }

    void OnDestroy()
    {
        PacketHandler.OnLogin -= HandleLogin;
        PacketHandler.OnEnterGame -= HandleEnterGame;
        PacketHandler.OnChatRes -= HandleChatRes;
    }

    // [Step 2] 로그인 결과 수신
    void HandleLogin(S_LOGIN pkt)
    {
        if (pkt.Success)
        {
            Debug.Log($"[ConnectionDebug] 3. Login Success! Token Received: {pkt.Token}");
            _chatLog += $">> System: Login Success!\n";
            _loginToken = pkt.Token;

            // 게임 서버로 환승 시작
            StartCoroutine(CoConnectToGameServer());
        }
        else
        {
            Debug.LogError("[ConnectionDebug] Login Failed.");
            _chatLog += ">> System: Login Failed! Check ID/PW.\n";
        }
    }

    // [Step 3] 환승 코루틴
    IEnumerator CoConnectToGameServer()
    {
        Debug.Log("[ConnectionDebug] 4. Disconnecting from LoginServer...");
        NetworkManager.Instance.Disconnect();

        yield return new WaitForSeconds(0.2f);

        Debug.Log("[ConnectionDebug] 5. Connecting to GameServer (7777)...");
        NetworkManager.Instance.Connect("127.0.0.1", 7777);

        yield return new WaitForSeconds(0.5f); // 접속 대기 시간을 조금 넉넉히

        Debug.Log("[ConnectionDebug] 6. Sending C_ENTER_GAME with Token...");
        SendEnterGamePacket();
    }

    // [Step 4] 게임 입장 완료
    void HandleEnterGame(S_ENTER_GAME pkt)
    {
        if (pkt.Success)
        {
            Debug.Log("[ConnectionDebug] 7. Game Enter Success! Welcome!");
            _isGameEntered = true;
            _chatLog += ">> System: Entered Game! You can move now.\n";
        }
        else
        {
            Debug.LogError("[ConnectionDebug] 7. Game Enter Failed! (Invalid Token?)");
            _chatLog += ">> System: Enter Game Failed\n";
        }
    }

    void HandleChatRes(bool success) { }

    private void OnGUI()
    {
        if (_isGameEntered == false)
        {
            GUI.Box(new Rect(Screen.width / 2 - 100, Screen.height / 2 - 80, 200, 160), "Nexus Login");

            GUI.Label(new Rect(Screen.width / 2 - 90, Screen.height / 2 - 50, 50, 20), "ID:");
            _inputName = GUI.TextField(new Rect(Screen.width / 2 - 40, Screen.height / 2 - 50, 130, 20), _inputName);

            GUI.Label(new Rect(Screen.width / 2 - 90, Screen.height / 2 - 20, 50, 20), "PW:");
            _inputPw = GUI.PasswordField(new Rect(Screen.width / 2 - 40, Screen.height / 2 - 20, 130, 20), _inputPw, '*');

            if (GUI.Button(new Rect(Screen.width / 2 - 50, Screen.height / 2 + 20, 100, 30), "Login"))
            {
                Debug.Log($"[ConnectionDebug] 2. Sending Login Request...");
                SendLoginPacket();
            }
        }
        else
        {
            GUILayout.BeginArea(new Rect(50, 50, Screen.width - 100, Screen.height - 100));
            _scrollPos = GUILayout.BeginScrollView(_scrollPos, GUILayout.Width(Screen.width - 100), GUILayout.Height(Screen.height - 150));
            GUILayout.Label(_chatLog);
            GUILayout.EndScrollView();

            GUILayout.BeginHorizontal();
            _inputChat = GUILayout.TextField(_inputChat, GUILayout.Width(Screen.width - 200));

            if (GUILayout.Button("Send", GUILayout.Width(80)) || (Event.current.isKey && Event.current.keyCode == KeyCode.Return))
            {
                if (string.IsNullOrEmpty(_inputChat) == false)
                {
                    SendChatPacket();
                    _inputChat = "";
                }
            }
            GUILayout.EndHorizontal();
            GUILayout.EndArea();
        }
    }

    void SendLoginPacket()
    {
        C_LOGIN packet = new C_LOGIN();
        packet.UserId = _inputName;
        packet.Password = _inputPw;
        NetworkManager.Instance.Send(packet, (ushort)PacketManager.MsgId.C_LOGIN);
    }

    void SendEnterGamePacket()
    {
        if (string.IsNullOrEmpty(_loginToken)) return;

        C_ENTER_GAME packet = new C_ENTER_GAME();
        packet.Token = _loginToken;
        NetworkManager.Instance.Send(packet, (ushort)PacketManager.MsgId.C_ENTER_GAME);
    }

    void SendChatPacket()
    {
        C_CHAT_REQ packet = new C_CHAT_REQ();
        packet.Message = _inputChat;
        NetworkManager.Instance.Send(packet, (ushort)PacketManager.MsgId.C_CHAT_REQ);
    }
}