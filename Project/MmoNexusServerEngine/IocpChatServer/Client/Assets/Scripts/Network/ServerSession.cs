using System;
using System.Net;
using System.Net.Sockets;
using System.Threading;
using UnityEngine;
using Google.Protobuf;
using Packet; // PacketHeader 네임스페이스

public class ServerSession
{
    Socket _socket;
    object _lock = new object();

    // 수신 버퍼
    byte[] _recvBuffer = new byte[65535];
    int _recvBytesStored = 0; // 현재 버퍼에 차있는 데이터 양

    // [Security] Seq 관리
    uint _sendSeq = 0;
    uint _recvSeq = 0; // 서버가 보낸 마지막 Seq 기억

    public bool CheckRecvSeq(uint seq)
    {
        if (seq <= _recvSeq) return false;
        _recvSeq = seq;
        return true;
    }

    public void Connect(string ip, int port)
    {
        _socket = new Socket(AddressFamily.InterNetwork, SocketType.Stream, ProtocolType.Tcp);
        IPEndPoint endPoint = new IPEndPoint(IPAddress.Parse(ip), port);

        try
        {
            _socket.Connect(endPoint);
            Debug.Log($"[ServerSession] Connected to {ip}:{port}");

            // 수신 대기 시작
            _socket.BeginReceive(_recvBuffer, 0, _recvBuffer.Length, SocketFlags.None, OnRecv, null);
        }
        catch (Exception e)
        {
            Debug.LogError($"[ServerSession] Connect Failed: {e}");
        }
    }

    // [GIGACHAD] 암호화 (XOR) - C++ 서버 키(0x5A)와 동일해야 함
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
        // 헤더를 붙이기 전에 내용물을 먼저 암호화한다.
        XorCrypt(bodyBytes, 0, bodyBytes.Length);

        // 3. [Total] 최종 전송 버퍼 생성 (Header 12 + Body)
        byte[] sendBuffer = new byte[dataSize + 12];

        // 4. [Header] 기본 정보 작성 (Size, ID)
        Array.Copy(BitConverter.GetBytes((ushort)(dataSize + 12)), 0, sendBuffer, 0, 2);
        Array.Copy(BitConverter.GetBytes(packetId), 0, sendBuffer, 2, 2);

        // 5. [Header] Seq 할당 (1부터 시작해야 서버가 첫 패킷 인정함)
        _sendSeq++;
        Array.Copy(BitConverter.GetBytes(_sendSeq), 0, sendBuffer, 8, 4);

        // 6. [Body] 암호화된 바디 복사
        Array.Copy(bodyBytes, 0, sendBuffer, 12, dataSize);

        // 7. [Header] CRC 계산 (Body 부분만 무결성 체크)
        // 12번 인덱스부터 끝(Body)까지 계산해서 헤더에 박는다.
        uint crc = Crc32.Compute(sendBuffer, 12, dataSize);
        Array.Copy(BitConverter.GetBytes(crc), 0, sendBuffer, 4, 4);

        // 8. 전송
        try
        {
            lock (_lock)
            {
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
                // 연결 끊김
                Disconnect();
                return;
            }

            // [패킷 처리] PacketManager에게 넘김
            // TCP 패킷 조립(TCP Sticky/Slicing) 문제는 추후 RecvBuffer 구현 시 해결.
            // 지금은 1:1 테스트 환경이라 온전한 패킷이 온다고 가정.
            PacketManager.Instance.OnRecvPacket(this, new ArraySegment<byte>(_recvBuffer, 0, recvLen));

            Debug.Log($"[ServerSession] Recv {recvLen} bytes");

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