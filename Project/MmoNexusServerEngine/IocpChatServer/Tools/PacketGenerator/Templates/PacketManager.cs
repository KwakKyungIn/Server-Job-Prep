using Google.Protobuf;
using Protocol; // [수정] 네임스페이스 변경
using System;
using System.Collections.Generic;

public class PacketManager
{
	#region Singleton
	static PacketManager _instance = new PacketManager();
	public static PacketManager Instance { get { return _instance; } }
	#endregion

	PacketManager()
	{
		Register();
	}

	// [수정] ServerSession을 전역으로 사용
	Dictionary<ushort, Action<ServerSession, ArraySegment<byte>, ushort>> _onRecv = new Dictionary<ushort, Action<ServerSession, ArraySegment<byte>, ushort>>();
	Dictionary<ushort, Action<ServerSession, IMessage>> _handler = new Dictionary<ushort, Action<ServerSession, IMessage>>();

	// [GIGACHAD FIX] MsgId를 외부 Enum.cs에 의존하지 않고 내부에서 정의함.
	// 이렇게 하면 파이썬 파서가 넘겨준 이름(S_LOGIN_RES 등)과 100% 일치함.
	public enum MsgId : ushort
	{
	{%- for pkt in parser.total_pkt %}
		{{pkt.name}} = {{pkt.id}},
	{%- endfor %}
	}
		
	public void Register()
	{
{%- for pkt in parser.recv_pkt %}
		_onRecv.Add((ushort)MsgId.{{pkt.name}}, MakePacket<{{pkt.name}}>);
		_handler.Add((ushort)MsgId.{{pkt.name}}, PacketHandler.{{pkt.name}}Handler);
{%- endfor %}
	}

	public void OnRecvPacket(ServerSession session, ArraySegment<byte> buffer)
	{
		ushort count = 0;

		ushort size = BitConverter.ToUInt16(buffer.Array, buffer.Offset);
		count += 2;
		ushort id = BitConverter.ToUInt16(buffer.Array, buffer.Offset + count);
		count += 2;
		uint crc = BitConverter.ToUInt32(buffer.Array, buffer.Offset + count);
		count += 4;
		uint seq = BitConverter.ToUInt32(buffer.Array, buffer.Offset + count);
		count += 4;

		int headerSize = 12;
		int dataSize = size - headerSize;

		// [CRC Check]
		uint calcCrc = Crc32.Compute(buffer.Array, buffer.Offset + headerSize, dataSize);
		if (crc != calcCrc)
		{
			UnityEngine.Debug.LogError($"[PacketManager] CRC Mismatch! Recv:{crc} Calc:{calcCrc}");
			return;
		}

		// [Seq Check]
		if (session.CheckRecvSeq(seq) == false)
		{
			UnityEngine.Debug.LogError($"[PacketManager] Invalid Seq! Recv:{seq}");
			return;
		}

		// [Decrypt]
		XorCrypt(buffer.Array, buffer.Offset + headerSize, dataSize);

		Action<ServerSession, ArraySegment<byte>, ushort> action = null;
		if (_onRecv.TryGetValue(id, out action))
			action.Invoke(session, buffer, id);
	}

	void MakePacket<T>(ServerSession session, ArraySegment<byte> buffer, ushort id) where T : IMessage, new()
	{
		T pkt = new T();
		// Header(12) Skip
		pkt.MergeFrom(buffer.Array, buffer.Offset + 12, buffer.Count - 12);

		Action<ServerSession, IMessage> action = null;
		if (_handler.TryGetValue(id, out action))
			action.Invoke(session, pkt);
	}

	public Action<ServerSession, IMessage> GetPacketHandler(ushort id)
	{
		Action<ServerSession, IMessage> action = null;
		if (_handler.TryGetValue(id, out action))
			return action;
		return null;
	}

	void XorCrypt(byte[] buffer, int offset, int len)
	{
		byte xorKey = 0x5A;
		for (int i = 0; i < len; i++)
		{
			buffer[offset + i] ^= xorKey;
		}
	}
}