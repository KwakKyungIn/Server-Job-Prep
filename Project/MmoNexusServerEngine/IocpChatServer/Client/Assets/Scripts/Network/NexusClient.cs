using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using Google.Protobuf;
using Protocol;

public class NexusClient : MonoBehaviour
{
    // C++의 g_isLoggedIn 역할
    bool _isLoggedIn = false;

    // UI 변수들
    string _inputName = "KwakPpiPpi_Unity";
    string _inputChat = "";
    string _chatLog = "";
    Vector2 _scrollPos;

    void Start()
    {
        // PacketHandler의 이벤트에 내 함수를 등록 (구독)
        PacketHandler.OnLoginResult = HandleLoginResult;
        PacketHandler.OnChatMsg = HandleChatMsg;
    }

    void OnDestroy()
    {
        // 파괴될 때 구독 해제 (메모리 누수 방지)
        PacketHandler.OnLoginResult = null;
        PacketHandler.OnChatMsg = null;
    }

    // [Callback] 로그인 결과 처리
    void HandleLoginResult(bool success)
    {
        _isLoggedIn = success;
        if (success)
            _chatLog += ">> System: Login Success!\n";
        else
            _chatLog += ">> System: Login Failed!\n";
    }

    // [Callback] 채팅 수신 처리
    void HandleChatMsg(string msg)
    {
        _chatLog += msg + "\n";
        _scrollPos.y = Mathf.Infinity; // 스크롤 맨 아래로
    }

    // [GUI] 옛날 방식 코드 UI (빠른 테스트용)
    private void OnGUI()
    {
        // 1. 로그인 전 화면
        if (_isLoggedIn == false)
        {
            GUI.Box(new Rect(Screen.width / 2 - 100, Screen.height / 2 - 50, 200, 100), "Login");

            _inputName = GUI.TextField(new Rect(Screen.width / 2 - 90, Screen.height / 2 - 20, 180, 20), _inputName);

            if (GUI.Button(new Rect(Screen.width / 2 - 50, Screen.height / 2 + 10, 100, 30), "Connect & Login"))
            {
                SendLoginPacket();
            }
        }
        // 2. 로그인 후 채팅 화면
        else
        {
            // 채팅 로그 영역
            GUILayout.BeginArea(new Rect(50, 50, Screen.width - 100, Screen.height - 100));
            _scrollPos = GUILayout.BeginScrollView(_scrollPos, GUILayout.Width(Screen.width - 100), GUILayout.Height(Screen.height - 150));
            GUILayout.Label(_chatLog);
            GUILayout.EndScrollView();

            // 입력 영역
            GUILayout.BeginHorizontal();
            _inputChat = GUILayout.TextField(_inputChat, GUILayout.Width(Screen.width - 200));

            if (GUILayout.Button("Send", GUILayout.Width(80)) || (Event.current.isKey && Event.current.keyCode == KeyCode.Return))
            {
                if (string.IsNullOrEmpty(_inputChat) == false)
                {
                    SendChatPacket();
                    _inputChat = ""; // 입력창 비우기
                }
            }
            GUILayout.EndHorizontal();
            GUILayout.EndArea();
        }
    }

    // [Packet Send] 로그인 패킷 발사
    void SendLoginPacket()
    {
        C_LOGIN_REQ packet = new C_LOGIN_REQ();
        packet.Name = _inputName;

        // PacketManager.MsgId를 이용해 ID 자동 매핑
        NetworkManager.Instance.Send(packet, (ushort)PacketManager.MsgId.C_LOGIN_REQ);
        Debug.Log($"[Client] Try Login: {_inputName}");
    }

    // [Packet Send] 채팅 패킷 발사
    void SendChatPacket()
    {
        C_CHAT_REQ packet = new C_CHAT_REQ();
        packet.Message = _inputChat;

        NetworkManager.Instance.Send(packet, (ushort)PacketManager.MsgId.C_CHAT_REQ);
    }
}