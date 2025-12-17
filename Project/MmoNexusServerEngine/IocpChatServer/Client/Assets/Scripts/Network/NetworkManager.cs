using System;
using System.Collections;
using System.Collections.Generic;
using System.Net;
using UnityEngine;
using Google.Protobuf;

public class NetworkManager : MonoBehaviour
{
    static NetworkManager _instance;
    public static NetworkManager Instance { get { return _instance; } }

    ServerSession _session = new ServerSession();
    Queue<Action> _packetQueue = new Queue<Action>();
    object _lock = new object();

    public bool IsMapChanging { get; private set; } = false;
    public ulong MapChangeToken { get; private set; } = 0;


    public void BeginMapChange(ulong token)
    {
        IsMapChanging = true;
        MapChangeToken = token;
        Debug.Log($"[NetworkManager] BeginMapChange token={token}");
    }

    public void EndMapChange(ulong token)
    {
        // 토큰 다르면 무시 (꼬임 방지)
        if (MapChangeToken != token)
        {
            Debug.LogWarning($"[NetworkManager] EndMapChange ignored (token mismatch) cur={MapChangeToken} recv={token}");
            return;
        }

        IsMapChanging = false;
        MapChangeToken = 0;
        Debug.Log($"[NetworkManager] EndMapChange token={token}");
    }


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
        string ip = "127.0.0.1";
        int port = 7775; // LoginServer Port

        InventoryManager.Instance.Init();

        Debug.Log($"[ConnectionDebug] 1. Start() - Initial Connect to LoginServer ({ip}:{port})");
        Connect(ip, port);
    }

    void Update()
    {
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

    public void Connect(string ip, int port)
    {
        IPAddress ipAddr = IPAddress.Parse(ip);
        IPEndPoint endPoint = new IPEndPoint(ipAddr, port);
        ConnectToServer(endPoint);
    }

    void ConnectToServer(IPEndPoint endPoint)
    {
        if (_session != null)
            _session.Disconnect();

        _session = new ServerSession();

        Debug.Log($"[ConnectionDebug] Connecting to Server... {endPoint.Address}:{endPoint.Port}");
        try
        {
            _session.Connect(endPoint);
            Debug.Log($"[ConnectionDebug] Socket Connect Called.");
        }
        catch (Exception e)
        {
            Debug.LogError($"[ConnectionDebug] Connect Failed: {e}");
        }
    }

    public void Disconnect()
    {
        if (_session != null)
        {
            _session.Disconnect();
            Debug.Log("[ConnectionDebug] Disconnected by User/Logic.");
        }
    }

    public void Send(IMessage packet, ushort packetId)
    {
        if (_session != null)
        {
            // Debug.Log($"[ConnectionDebug] Sending Packet ID: {packetId}");
            _session.Send(packet, packetId);
        }
        else
        {
            Debug.LogError("[ConnectionDebug] Send Failed! Session is null or disconnected.");
        }
    }

    public void PushPacket(Action action)
    {
        lock (_lock)
        {
            _packetQueue.Enqueue(action);
        }
    }

    private void OnApplicationQuit()
    {
        Disconnect();
    }
}