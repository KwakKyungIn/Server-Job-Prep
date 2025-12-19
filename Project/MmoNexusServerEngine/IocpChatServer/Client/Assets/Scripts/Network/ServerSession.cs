// ServerSession.cs

using System;
using System.Net;
using System.Net.Sockets;
using UnityEngine;
using Google.Protobuf;

public class ServerSession
{
    Socket _socket;
    object _lock = new object();

    byte[] _recvBuffer = new byte[65535];

    // ✅ 누적 수신 길이(Partial 대응)
    int _recvCount = 0;

    // Seq
    uint _sendSeq = 0;
    uint _recvSeq = 0;

    public bool CheckRecvSeq(uint seq)
    {
        if (seq <= _recvSeq) return false;
        _recvSeq = seq;
        return true;
    }

    public void Connect(IPEndPoint endPoint)
    {
        _socket = new Socket(AddressFamily.InterNetwork, SocketType.Stream, ProtocolType.Tcp);

        try
        {
            _socket.Connect(endPoint);
            Debug.Log($"[ServerSession] Connected to {endPoint}");

            _recvCount = 0;
            _socket.BeginReceive(_recvBuffer, 0, _recvBuffer.Length, SocketFlags.None, OnRecv, null);
        }
        catch (Exception e)
        {
            Debug.LogError($"[ServerSession] Connect Failed: {e}");
        }
    }

    void XorCrypt(byte[] buffer, int offset, int len)
    {
        byte xorKey = 0x5A;
        for (int i = 0; i < len; i++)
            buffer[offset + i] ^= xorKey;
    }

    public void Send(IMessage packet, ushort packetId)
    {
        int dataSize = packet.CalculateSize();
        byte[] bodyBytes = packet.ToByteArray();

        XorCrypt(bodyBytes, 0, bodyBytes.Length);

        byte[] sendBuffer = new byte[dataSize + 12];

        Array.Copy(BitConverter.GetBytes((ushort)(dataSize + 12)), 0, sendBuffer, 0, 2);
        Array.Copy(BitConverter.GetBytes(packetId), 0, sendBuffer, 2, 2);

        _sendSeq++;
        Array.Copy(BitConverter.GetBytes(_sendSeq), 0, sendBuffer, 8, 4);

        Array.Copy(bodyBytes, 0, sendBuffer, 12, dataSize);

        uint crc = Crc32.Compute(sendBuffer, 12, dataSize);
        Array.Copy(BitConverter.GetBytes(crc), 0, sendBuffer, 4, 4);

        try
        {
            lock (_lock)
            {
                if (_socket != null && _socket.Connected)
                    _socket.Send(sendBuffer);
            }
        }
        catch (Exception e)
        {
            Debug.LogError($"[ServerSession] Send Failed: {e}");
        }
    }

    void OnRecv(IAsyncResult ar)
    {
        try
        {
            int len = _socket.EndReceive(ar);
            if (len == 0)
            {
                Disconnect();
                return;
            }

            _recvCount += len;

            int processed = 0;
            while (true)
            {
                int remaining = _recvCount - processed;

                if (remaining < 12) break;

                ushort packetSize = BitConverter.ToUInt16(_recvBuffer, processed);

                if (packetSize < 12 || packetSize > _recvBuffer.Length)
                {
                    Debug.LogError($"[ServerSession] Invalid packet size={packetSize}. Force disconnect.");
                    Disconnect();
                    return;
                }

                if (remaining < packetSize) break;

                PacketManager.Instance.OnRecvPacket(this,
                    new ArraySegment<byte>(_recvBuffer, processed, packetSize));

                processed += packetSize;
            }

            if (processed > 0)
            {
                // ✅ 남은 데이터 앞으로 당겨서 다음 recv에 이어붙임
                Buffer.BlockCopy(_recvBuffer, processed, _recvBuffer, 0, _recvCount - processed);
                _recvCount -= processed;
            }

            _socket.BeginReceive(_recvBuffer, _recvCount, _recvBuffer.Length - _recvCount, SocketFlags.None, OnRecv, null);
        }
        catch (Exception e)
        {
            Debug.LogError($"[ServerSession] OnRecv Error: {e}");
            Disconnect();
        }
    }

    public void Disconnect()
    {
        if (_socket != null)
        {
            try
            {
                _socket.Shutdown(SocketShutdown.Both);
                _socket.Close();
            }
            catch { }

            _socket = null;
            _recvCount = 0;
            Debug.Log("[ServerSession] Disconnected");
        }
    }
}
