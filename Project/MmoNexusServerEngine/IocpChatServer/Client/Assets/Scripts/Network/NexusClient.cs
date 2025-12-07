using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using Google.Protobuf;
using Protocol;

public class NexusClient : MonoBehaviour
{
    // UI 상태 제어 (로그인 화면 -> 게임/채팅 화면)
    bool _isGameEntered = false;

    // UI 변수들
    string _inputName = "KwakPpiPpi_Unity";
    string _inputChat = "";
    string _chatLog = "";
    Vector2 _scrollPos;

    void Start()
    {
        // [Event Subscription] 패킷 핸들러 이벤트 구독
        PacketHandler.OnLoginResult += HandleLoginResult;
        PacketHandler.OnEnterGame += HandleEnterGame; // [New] 입장 완료 이벤트
        PacketHandler.OnChatMsg += HandleChatMsg;
    }

    void OnDestroy()
    {
        // 구독 해제 (습관화해라)
        PacketHandler.OnLoginResult -= HandleLoginResult;
        PacketHandler.OnEnterGame -= HandleEnterGame;
        PacketHandler.OnChatMsg -= HandleChatMsg;
    }

    // [Callback] 로그인 결과 처리
    void HandleLoginResult(bool success)
    {
        if (success)
        {
            _chatLog += ">> System: Login Success! Requesting Enter Game...\n";
            // [Core Logic] 로그인 성공했으면 바로 게임 입장 요청
            SendEnterGamePacket();
        }
        else
        {
            _chatLog += ">> System: Login Failed! Check Server.\n";
        }
    }

    // [Callback] 게임 입장 결과 (캐릭터 스폰 시점)
    void HandleEnterGame(S_ENTER_GAME_RES pkt)
    {
        if (pkt.Success)
        {
            _isGameEntered = true; // UI 전환
            _chatLog += ">> System: Entered Game! You can move now (WASD).\n";
        }
    }

    // [Callback] 채팅 수신 처리
    void HandleChatMsg(string msg)
    {
        _chatLog += msg + "\n";
        _scrollPos.y = Mathf.Infinity;
    }

    // [GUI] 테스트용 UI
    private void OnGUI()
    {
        // 1. 게임 입장 전 (로그인 화면)
        if (_isGameEntered == false)
        {
            GUI.Box(new Rect(Screen.width / 2 - 100, Screen.height / 2 - 50, 200, 100), "Nexus Login");

            _inputName = GUI.TextField(new Rect(Screen.width / 2 - 90, Screen.height / 2 - 20, 180, 20), _inputName);

            if (GUI.Button(new Rect(Screen.width / 2 - 50, Screen.height / 2 + 10, 100, 30), "Login & Start"))
            {
                SendLoginPacket();
            }
        }
        // 2. 게임 입장 후 (채팅 + 로그)
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
                    _inputChat = "";
                }
            }
            GUILayout.EndHorizontal();
            GUILayout.EndArea();
        }
    }

    // [Packet Send] 1. 로그인 요청
    void SendLoginPacket()
    {
        C_LOGIN_REQ packet = new C_LOGIN_REQ();
        packet.Name = _inputName;

        // [FIX] PacketId 인자 추가
        NetworkManager.Instance.Send(packet, (ushort)PacketManager.MsgId.C_LOGIN_REQ);
        Debug.Log($"[Client] Try Login: {_inputName}");
    }

    // [Packet Send] 2. 게임 입장 요청
    void SendEnterGamePacket()
    {
        C_ENTER_GAME_REQ packet = new C_ENTER_GAME_REQ();
        packet.PlayerIndex = 0; // 첫 번째 슬롯 캐릭터 선택 (일단 고정)

        NetworkManager.Instance.Send(packet, (ushort)PacketManager.MsgId.C_ENTER_GAME_REQ);
    }

    // [Packet Send] 채팅
    void SendChatPacket()
    {
        C_CHAT_REQ packet = new C_CHAT_REQ();
        packet.Message = _inputChat;

        NetworkManager.Instance.Send(packet, (ushort)PacketManager.MsgId.C_CHAT_REQ);
    }
}