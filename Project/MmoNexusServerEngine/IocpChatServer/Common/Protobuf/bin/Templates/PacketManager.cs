using Google.Protobuf;
using Protocol;
using System;
using System.Collections.Generic;
using UnityEngine;

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

	Dictionary<ushort, Action<ServerSession, ArraySegment<byte>, ushort>> _onRecv = new Dictionary<ushort, Action<ServerSession, ArraySegment<byte>, ushort>>();
	Dictionary<ushort, Action<ServerSession, IMessage>> _handler = new Dictionary<ushort, Action<ServerSession, IMessage>>();

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
		{
			// [GIGACHAD FIX] IO Thread -> Main Thread Queue Toss!
			// 패킷 조립(Deserialize)은 여기서 끝내고, 로직 실행(Handler)은 메인 스레드 큐로 넘긴다.
			NetworkManager.Instance.PushPacket(() => action.Invoke(session, pkt));
		}
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