using System;
using System.Net;
using System.Net.Sockets;
using System.Threading;
using UnityEngine;
using Google.Protobuf;
// using Packet; // PacketHeader 네임스페이스가 없다면 제거

public class ServerSession
{
    Socket _socket;
    object _lock = new object();

    // 수신 버퍼
    byte[] _recvBuffer = new byte[65535];

    // [Security] Seq 관리
    uint _sendSeq = 0;
    uint _recvSeq = 0;

    public bool CheckRecvSeq(uint seq)
    {
        if (seq <= _recvSeq) return false;
        _recvSeq = seq;
        return true;
    }

    // [Modification] IPEndPoint를 직접 받도록 변경
    public void Connect(IPEndPoint endPoint)
    {
        _socket = new Socket(AddressFamily.InterNetwork, SocketType.Stream, ProtocolType.Tcp);

        try
        {
            _socket.Connect(endPoint);
            Debug.Log($"[ServerSession] Connected to {endPoint}");

            // 수신 대기 시작
            _socket.BeginReceive(_recvBuffer, 0, _recvBuffer.Length, SocketFlags.None, OnRecv, null);
        }
        catch (Exception e)
        {
            Debug.LogError($"[ServerSession] Connect Failed: {e}");
        }
    }

    // [GIGACHAD] 암호화 (XOR)
    void XorCrypt(byte[] buffer, int offset, int len)
    {
        byte xorKey = 0x5A;
        for (int i = 0; i < len; i++)
        {
            buffer[offset + i] ^= xorKey;
        }
    }

    public void Send(IMessage packet, ushort packetId)
    {
        // 1. [Body] Protobuf 직렬화
        int dataSize = packet.CalculateSize();
        byte[] bodyBytes = packet.ToByteArray();

        // 2. [Body] 암호화 (Serialize -> Encrypt)
        XorCrypt(bodyBytes, 0, bodyBytes.Length);

        // 3. [Total] 최종 전송 버퍼 생성 (Header 12 + Body)
        byte[] sendBuffer = new byte[dataSize + 12];

        // 4. [Header] 기본 정보 작성 (Size, ID)
        Array.Copy(BitConverter.GetBytes((ushort)(dataSize + 12)), 0, sendBuffer, 0, 2);
        Array.Copy(BitConverter.GetBytes(packetId), 0, sendBuffer, 2, 2);

        // 5. [Header] Seq 할당
        _sendSeq++;
        Array.Copy(BitConverter.GetBytes(_sendSeq), 0, sendBuffer, 8, 4);

        // 6. [Body] 암호화된 바디 복사
        Array.Copy(bodyBytes, 0, sendBuffer, 12, dataSize);

        // 7. [Header] CRC 계산
        uint crc = Crc32.Compute(sendBuffer, 12, dataSize);
        Array.Copy(BitConverter.GetBytes(crc), 0, sendBuffer, 4, 4);

        // 8. 전송
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
            int recvLen = _socket.EndReceive(ar);
            if (recvLen == 0)
            {
                Disconnect();
                return;
            }

            // [패킷 처리]
            // 주의: 여기서 PacketManager를 호출하면 IO 스레드에서 실행됨.
            // PacketManager 내부에서 Handler를 호출할 때 NetworkManager.PushPacket을 사용해야 함.
            PacketManager.Instance.OnRecvPacket(this, new ArraySegment<byte>(_recvBuffer, 0, recvLen));

            // 다시 수신 대기
            _socket.BeginReceive(_recvBuffer, 0, _recvBuffer.Length, SocketFlags.None, OnRecv, null);
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
            catch (Exception e)
            {
                Debug.Log($"[ServerSession] Disconnect Error: {e}");
            }

            _socket = null;
            Debug.Log("[ServerSession] Disconnected");
        }
    }
}