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
		C_LOGIN_REQ = 1000,
		S_LOGIN_RES = 1001,
		C_ENTER_GAME_REQ = 1002,
		S_ENTER_GAME_RES = 1003,
		C_MOVE = 1004,
		S_MOVE = 1005,
		S_SPAWN = 1006,
		S_DESPAWN = 1007,
		C_CHAT_REQ = 1008,
		S_CHAT_RES = 1009,
		S_CHAT_NTF = 1010,
		S_HEART_BEAT_RES = 1011,
		C_HEART_BEAT_REQ = 1012,
	}

	public void Register()
	{
		_onRecv.Add((ushort)MsgId.S_LOGIN_RES, MakePacket<S_LOGIN_RES>);
		_handler.Add((ushort)MsgId.S_LOGIN_RES, PacketHandler.S_LOGIN_RESHandler);
		_onRecv.Add((ushort)MsgId.S_ENTER_GAME_RES, MakePacket<S_ENTER_GAME_RES>);
		_handler.Add((ushort)MsgId.S_ENTER_GAME_RES, PacketHandler.S_ENTER_GAME_RESHandler);
		_onRecv.Add((ushort)MsgId.S_MOVE, MakePacket<S_MOVE>);
		_handler.Add((ushort)MsgId.S_MOVE, PacketHandler.S_MOVEHandler);
		_onRecv.Add((ushort)MsgId.S_SPAWN, MakePacket<S_SPAWN>);
		_handler.Add((ushort)MsgId.S_SPAWN, PacketHandler.S_SPAWNHandler);
		_onRecv.Add((ushort)MsgId.S_DESPAWN, MakePacket<S_DESPAWN>);
		_handler.Add((ushort)MsgId.S_DESPAWN, PacketHandler.S_DESPAWNHandler);
		_onRecv.Add((ushort)MsgId.S_CHAT_RES, MakePacket<S_CHAT_RES>);
		_handler.Add((ushort)MsgId.S_CHAT_RES, PacketHandler.S_CHAT_RESHandler);
		_onRecv.Add((ushort)MsgId.S_CHAT_NTF, MakePacket<S_CHAT_NTF>);
		_handler.Add((ushort)MsgId.S_CHAT_NTF, PacketHandler.S_CHAT_NTFHandler);
		_onRecv.Add((ushort)MsgId.S_HEART_BEAT_RES, MakePacket<S_HEART_BEAT_RES>);
		_handler.Add((ushort)MsgId.S_HEART_BEAT_RES, PacketHandler.S_HEART_BEAT_RESHandler);
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
		
		// [GIGACHAD FIX] 중요! buffer.Count가 아니라 헤더에 적힌 진짜 사이즈를 믿어야 한다.
		// TCP 패킷이 뭉쳐서 왔을 때(Sticky), 뒤에 붙은 데이터까지 읽지 않도록 차단.
		ushort size = BitConverter.ToUInt16(buffer.Array, buffer.Offset);
		int headerSize = 12;
		int payloadSize = size - headerSize;

		// 정확히 payloadSize만큼만 파싱 -> Invalid Tag 에러 해결
		pkt.MergeFrom(buffer.Array, buffer.Offset + 12, payloadSize);

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