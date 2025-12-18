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
		C_LOGIN = 1000,
		S_LOGIN = 1001,
		C_ENTER_GAME = 1002,
		S_ENTER_GAME = 1003,
		C_MOVE = 1004,
		S_MOVE = 1005,
		S_SPAWN = 1006,
		S_DESPAWN = 1007,
		C_SKILL = 1008,
		S_SKILL = 1009,
		S_CHANGE_HP = 1010,
		S_ITEM_LIST = 1011,
		C_USE_ITEM = 1012,
		S_CHANGE_ITEM = 1013,
		S_REMOVE_ITEM = 1014,
		C_EQUIP_ITEM = 1015,
		S_EQUIP_ITEM = 1016,
		S_CHANGE_STAT = 1017,
		C_MAP_CHANGE_REQ = 1018,
		S_MAP_CHANGE_BEGIN = 1019,
		C_MAP_CHANGE_ACK = 1020,
		S_MAP_CHANGE_END = 1021,
		C_CHAT_REQ = 1022,
		S_CHAT_RES = 1023,
		S_CHAT_NTF = 1024,
		S_HEART_BEAT_RES = 1025,
		C_HEART_BEAT_REQ = 1026,
		C_PARTY_CHAT_REQ = 1027,
		S_PARTY_CHAT_NTF = 1028,
		S_PARTY_INFO_NTF = 1029,
	}

	public void Register()
	{
		_onRecv.Add((ushort)MsgId.S_LOGIN, MakePacket<S_LOGIN>);
		_handler.Add((ushort)MsgId.S_LOGIN, PacketHandler.S_LOGINHandler);
		_onRecv.Add((ushort)MsgId.S_ENTER_GAME, MakePacket<S_ENTER_GAME>);
		_handler.Add((ushort)MsgId.S_ENTER_GAME, PacketHandler.S_ENTER_GAMEHandler);
		_onRecv.Add((ushort)MsgId.S_MOVE, MakePacket<S_MOVE>);
		_handler.Add((ushort)MsgId.S_MOVE, PacketHandler.S_MOVEHandler);
		_onRecv.Add((ushort)MsgId.S_SPAWN, MakePacket<S_SPAWN>);
		_handler.Add((ushort)MsgId.S_SPAWN, PacketHandler.S_SPAWNHandler);
		_onRecv.Add((ushort)MsgId.S_DESPAWN, MakePacket<S_DESPAWN>);
		_handler.Add((ushort)MsgId.S_DESPAWN, PacketHandler.S_DESPAWNHandler);
		_onRecv.Add((ushort)MsgId.S_SKILL, MakePacket<S_SKILL>);
		_handler.Add((ushort)MsgId.S_SKILL, PacketHandler.S_SKILLHandler);
		_onRecv.Add((ushort)MsgId.S_CHANGE_HP, MakePacket<S_CHANGE_HP>);
		_handler.Add((ushort)MsgId.S_CHANGE_HP, PacketHandler.S_CHANGE_HPHandler);
		_onRecv.Add((ushort)MsgId.S_ITEM_LIST, MakePacket<S_ITEM_LIST>);
		_handler.Add((ushort)MsgId.S_ITEM_LIST, PacketHandler.S_ITEM_LISTHandler);
		_onRecv.Add((ushort)MsgId.S_CHANGE_ITEM, MakePacket<S_CHANGE_ITEM>);
		_handler.Add((ushort)MsgId.S_CHANGE_ITEM, PacketHandler.S_CHANGE_ITEMHandler);
		_onRecv.Add((ushort)MsgId.S_REMOVE_ITEM, MakePacket<S_REMOVE_ITEM>);
		_handler.Add((ushort)MsgId.S_REMOVE_ITEM, PacketHandler.S_REMOVE_ITEMHandler);
		_onRecv.Add((ushort)MsgId.S_EQUIP_ITEM, MakePacket<S_EQUIP_ITEM>);
		_handler.Add((ushort)MsgId.S_EQUIP_ITEM, PacketHandler.S_EQUIP_ITEMHandler);
		_onRecv.Add((ushort)MsgId.S_CHANGE_STAT, MakePacket<S_CHANGE_STAT>);
		_handler.Add((ushort)MsgId.S_CHANGE_STAT, PacketHandler.S_CHANGE_STATHandler);
		_onRecv.Add((ushort)MsgId.S_MAP_CHANGE_BEGIN, MakePacket<S_MAP_CHANGE_BEGIN>);
		_handler.Add((ushort)MsgId.S_MAP_CHANGE_BEGIN, PacketHandler.S_MAP_CHANGE_BEGINHandler);
		_onRecv.Add((ushort)MsgId.S_MAP_CHANGE_END, MakePacket<S_MAP_CHANGE_END>);
		_handler.Add((ushort)MsgId.S_MAP_CHANGE_END, PacketHandler.S_MAP_CHANGE_ENDHandler);
		_onRecv.Add((ushort)MsgId.S_CHAT_RES, MakePacket<S_CHAT_RES>);
		_handler.Add((ushort)MsgId.S_CHAT_RES, PacketHandler.S_CHAT_RESHandler);
		_onRecv.Add((ushort)MsgId.S_CHAT_NTF, MakePacket<S_CHAT_NTF>);
		_handler.Add((ushort)MsgId.S_CHAT_NTF, PacketHandler.S_CHAT_NTFHandler);
		_onRecv.Add((ushort)MsgId.S_HEART_BEAT_RES, MakePacket<S_HEART_BEAT_RES>);
		_handler.Add((ushort)MsgId.S_HEART_BEAT_RES, PacketHandler.S_HEART_BEAT_RESHandler);
		_onRecv.Add((ushort)MsgId.S_PARTY_CHAT_NTF, MakePacket<S_PARTY_CHAT_NTF>);
		_handler.Add((ushort)MsgId.S_PARTY_CHAT_NTF, PacketHandler.S_PARTY_CHAT_NTFHandler);
		_onRecv.Add((ushort)MsgId.S_PARTY_INFO_NTF, MakePacket<S_PARTY_INFO_NTF>);
		_handler.Add((ushort)MsgId.S_PARTY_INFO_NTF, PacketHandler.S_PARTY_INFO_NTFHandler);
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