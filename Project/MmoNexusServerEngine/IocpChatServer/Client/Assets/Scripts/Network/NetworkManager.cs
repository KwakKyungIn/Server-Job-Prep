using System;
using System.Collections;
using System.Collections.Generic;
using System.Net;
using UnityEngine;
using Google.Protobuf;

public class NetworkManager : MonoBehaviour
{
    // [Singleton]
    static NetworkManager _instance;
    public static NetworkManager Instance { get { return _instance; } }

    ServerSession _session = new ServerSession();

    // [GIGACHAD FIX] 메인 스레드 처리를 위한 작업 큐
    Queue<Action> _packetQueue = new Queue<Action>();
    object _lock = new object();

    void Awake()
    {
        if (_instance == null)
        {
            _instance = this;
            DontDestroyOnLoad(gameObject); // 씬 이동해도 유지
        }
        else
        {
            Destroy(gameObject);
        }
    }

    void Start()
    {
        // [GIGACHAD FIX] Back to Basic.
        // 복잡한 DNS 조회 없이, 네가 원하던 대로 127.0.0.1로 직결한다.
        string ip = "127.0.0.1";
        int port = 7777;

        IPAddress ipAddr = IPAddress.Parse(ip);
        IPEndPoint endPoint = new IPEndPoint(ipAddr, port);

        Debug.Log($"[NetworkManager] Try Connect to Server... ({ip}:{port})");

        // ServerSession이 IPEndPoint를 받도록 수정했으므로 맞춰서 넘겨준다.
        ConnectToServer(endPoint);
    }

    void ConnectToServer(IPEndPoint endPoint)
    {
        _session.Connect(endPoint);
    }

    void Update()
    {
        // [Main Thread Dispatch]
        // 큐에 쌓인 패킷 로직을 메인 스레드에서 처리 (UnityException 방지)
        List<Action> list = null;
        lock (_lock)
        {
            if (_packetQueue.Count > 0)
            {
                list = new List<Action>(_packetQueue);
                _packetQueue.Clear();
            }
        }

        if (list != null)
        {
            foreach (Action action in list)
            {
                try
                {
                    action.Invoke();
                }
                catch (Exception e)
                {
                    Debug.LogError($"Packet Action Error: {e}");
                }
            }
        }
    }

    // 패킷 전송
    public void Send(IMessage packet, ushort packetId)
    {
        _session.Send(packet, packetId);
    }

    // [External Interface] 외부(Session/Handler)에서 메인 스레드 작업을 요청할 때 사용
    public void PushPacket(Action action)
    {
        lock (_lock)
        {
            _packetQueue.Enqueue(action);
        }
    }

    private void OnApplicationQuit()
    {
        _session.Disconnect();
    }
}