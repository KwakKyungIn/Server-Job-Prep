using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class NetworkManager : MonoBehaviour
{
    // 싱글톤 (어디서든 NetworkManager.Instance로 접근 가능)
    public static NetworkManager Instance { get; private set; }

    ServerSession _session = new ServerSession();

    void Awake()
    {
        if (Instance == null)
        {
            Instance = this;
            DontDestroyOnLoad(gameObject); // 씬 넘어가도 파괴 안 됨
        }
        else
        {
            Destroy(gameObject);
        }
    }

    void Start()
    {
        // 게임 시작 시 서버 접속 시도
        // IP는 로컬호스트(127.0.0.1), 포트는 GameServer(7777)
        Debug.Log("Try Connect to Server...");
        _session.Connect("127.0.0.1", 7777);
    }

    void Update()
    {
        // 나중에 여기서 메인 스레드 패킷 처리(Dispatch)를 할 예정
        // _session.HandlePacketQueue(); 
    }

    public void Send(Google.Protobuf.IMessage packet, ushort packetId)
    {
        _session.Send(packet, packetId);
    }

    private void OnApplicationQuit()
    {
        _session.Disconnect();
    }
}