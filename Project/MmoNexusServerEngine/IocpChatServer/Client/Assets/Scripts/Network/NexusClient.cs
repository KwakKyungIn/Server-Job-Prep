using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using Google.Protobuf;
using Protocol;
using UnityEngine.SceneManagement;

public class NexusClient : MonoBehaviour
{
    bool _isGameEntered = false;
    bool _isLoggedIn = false;          // 로그인 성공 여부 (채널 선택 단계 분리)

    string _inputName = "KwakPpiPpi";
    string _inputPw = "1234";
    string _inputChat = "";
    string _chatLog = "";
    Vector2 _scrollPos;

    string _loginToken = "";

    // 로그인 서버에서 받은 서버(채널) 리스트
    List<ServerInfo> _serverList = new List<ServerInfo>();

    // 선택된 채널 ID (1, 2, 3...)
    int _selectedChannelId = 0;


    static NexusClient _instance;
    public static NexusClient Instance => _instance;

    void Awake()
    {
        if (_instance == null)
        {
            _instance = this;
            DontDestroyOnLoad(gameObject);
        }
        else
        {
            Destroy(gameObject);
        }
    }

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

            // 서버 리스트 저장 (채널 선택용)
            _serverList.Clear();
            foreach (var s in pkt.ServerList)
                _serverList.Add(s);

            if (_serverList.Count == 0)
            {
                // 방어 코드: 혹시 서버에서 안 보내줬을 때 기본 1개라도 만들어 둔다.
                ServerInfo dummy = new ServerInfo();
                dummy.Name = "Channel 1";
                dummy.Ip = "127.0.0.1";
                dummy.Port = 7777;
                dummy.Congestion = 0;
                _serverList.Add(dummy);
            }

            _isLoggedIn = true;   // 이제 채널 선택 UI로 넘어간다.
            _chatLog += ">> System: Select a channel.\n";
            SceneManager.LoadScene("02_Channel");
        }
        else
        {
            Debug.LogError("[ConnectionDebug] Login Failed.");
            _chatLog += ">> System: Login Failed! Check ID/PW.\n";
        }
    }

    // [Step 3] 환승 코루틴 (선택된 채널로 이동)
    IEnumerator CoConnectToGameServer(ServerInfo serverInfo, int channelId)
    {
        Debug.Log("[ConnectionDebug] 4. Disconnecting from LoginServer...");
        NetworkManager.Instance.Disconnect();

        yield return new WaitForSeconds(0.2f);

        Debug.Log($"[ConnectionDebug] 5. Connecting to GameServer {serverInfo.Name} ({serverInfo.Ip}:{serverInfo.Port})...");
        NetworkManager.Instance.Connect(serverInfo.Ip, serverInfo.Port);

        yield return new WaitForSeconds(0.5f); // 접속 대기

        SceneManager.LoadScene("03_Game_0001");
        yield return null;


        Debug.Log("[ConnectionDebug] 6. Sending C_ENTER_GAME with Token + ChannelId + MapId...");
        _selectedChannelId = channelId;
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

    public void RequestLogin(string id, string pw)
    {
        _inputName = id;
        _inputPw = pw;
        SendLoginPacket();
    }

    public List<ServerInfo> GetServerList()
    {
        return _serverList;
    }

    public void RequestEnterGameByIndex(int index)
    {
        if (index < 0 || index >= _serverList.Count) return;
        StartCoroutine(CoConnectToGameServer(_serverList[index], index + 1));
    }

    public void RequestSendChat(string msg)
    {
        _inputChat = msg;
        SendChatPacket();
    }


    /*
    private void OnGUI()
    {
        // 1단계: 아직 로그인 안 됨 -> 로그인 UI
        if (_isLoggedIn == false && _isGameEntered == false)
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
        // 2단계: 로그인은 됐는데 게임 입장 전 -> 채널 선택 UI
        else if (_isLoggedIn == true && _isGameEntered == false)
        {
            GUI.Box(new Rect(Screen.width / 2 - 150, Screen.height / 2 - 120, 300, 240), "Select Channel");

            if (_serverList.Count == 0)
            {
                GUI.Label(new Rect(Screen.width / 2 - 120, Screen.height / 2 - 60, 240, 20),
                    "No server list from LoginServer.");
            }
            else
            {
                for (int i = 0; i < _serverList.Count; i++)
                {
                    ServerInfo s = _serverList[i];
                    string label = $"{i + 1}. {s.Name}  ({s.Ip}:{s.Port})  Load:{s.Congestion}";

                    if (GUI.Button(new Rect(Screen.width / 2 - 140, Screen.height / 2 - 80 + i * 40, 280, 30), label))
                    {
                        int channelId = i + 1;     // index+1 을 ChannelId로 사용
                        Debug.Log($"[ChannelSelect] Selected {s.Name} / ChannelId={channelId}");
                        _chatLog += $">> System: Selected Channel {s.Name}\n";

                        StartCoroutine(CoConnectToGameServer(s, channelId));
                    }
                }
            }
        }
        // 3단계: 게임 입장 완료 -> 기존 채팅 UI
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
    */
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
        if (_selectedChannelId == 0) return;

        C_ENTER_GAME packet = new C_ENTER_GAME();
        packet.Token = _loginToken;
        packet.ChannelId = _selectedChannelId;

        // 지금은 맵 하나만 사용하니까 하드코딩
        packet.MapId = 1;

        NetworkManager.Instance.Send(packet, (ushort)PacketManager.MsgId.C_ENTER_GAME);
    }

    void SendChatPacket()
    {
        C_CHAT_REQ packet = new C_CHAT_REQ();
        packet.Message = _inputChat;
        NetworkManager.Instance.Send(packet, (ushort)PacketManager.MsgId.C_CHAT_REQ);
    }
}
