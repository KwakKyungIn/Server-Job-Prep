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
		S_GOLD_UPDATE = 1018,
		C_MAP_CHANGE_REQ = 1019,
		S_MAP_CHANGE_BEGIN = 1020,
		C_MAP_CHANGE_ACK = 1021,
		S_MAP_CHANGE_END = 1022,
		C_CHANNEL_CHANGE_REQ = 1023,
		C_CHAT_REQ = 1024,
		S_CHAT_RES = 1025,
		S_CHAT_NTF = 1026,
		S_HEART_BEAT_RES = 1027,
		C_HEART_BEAT_REQ = 1028,
		C_PARTY_CHAT_REQ = 1029,
		S_PARTY_CHAT_NTF = 1030,
		S_PARTY_INFO_NTF = 1031,
		C_PARTY_CREATE_REQ = 1032,
		C_PARTY_INVITE_REQ = 1033,
		C_PARTY_INVITE_ACCEPT_REQ = 1034,
		C_PARTY_LEAVE_REQ = 1035,
		C_PARTY_KICK_REQ = 1036,
		C_PARTY_DISBAND_REQ = 1037,
		S_PARTY_RESULT = 1038,
		S_PARTY_INVITE_NTF = 1039,
		C_PARTY_STATUS_REQ = 1040,
		S_PARTY_STATUS_NTF = 1041,
		C_DUNGEON_ENTER_REQ = 1042,
		S_DUNGEON_ENTER_RES = 1043,
		C_DUNGEON_EXIT_REQ = 1044,
		S_DUNGEON_EXIT_RES = 1045,
		S_QUICKSLOT_LIST = 1046,
		C_SET_QUICKSLOT = 1047,
		S_SET_QUICKSLOT = 1048,
		C_TRADE_REQ = 1049,
		S_TRADE_INVITE = 1050,
		C_TRADE_INVITE_RESP = 1051,
		S_TRADE_START = 1052,
		C_TRADE_OFFER_SET = 1053,
		C_TRADE_GOLD_SET = 1054,
		S_TRADE_OFFER_UPDATE = 1055,
		C_TRADE_READY = 1056,
		S_TRADE_READY_STATE = 1057,
		S_TRADE_LOCKED = 1058,
		C_TRADE_CONFIRM = 1059,
		C_TRADE_CANCEL = 1060,
		S_TRADE_CANCELLED = 1061,
		S_TRADE_RESULT = 1062,
		C_INV_DRAG_DROP = 1063,
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
		_onRecv.Add((ushort)MsgId.S_GOLD_UPDATE, MakePacket<S_GOLD_UPDATE>);
		_handler.Add((ushort)MsgId.S_GOLD_UPDATE, PacketHandler.S_GOLD_UPDATEHandler);
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
		_onRecv.Add((ushort)MsgId.S_PARTY_RESULT, MakePacket<S_PARTY_RESULT>);
		_handler.Add((ushort)MsgId.S_PARTY_RESULT, PacketHandler.S_PARTY_RESULTHandler);
		_onRecv.Add((ushort)MsgId.S_PARTY_INVITE_NTF, MakePacket<S_PARTY_INVITE_NTF>);
		_handler.Add((ushort)MsgId.S_PARTY_INVITE_NTF, PacketHandler.S_PARTY_INVITE_NTFHandler);
		_onRecv.Add((ushort)MsgId.S_PARTY_STATUS_NTF, MakePacket<S_PARTY_STATUS_NTF>);
		_handler.Add((ushort)MsgId.S_PARTY_STATUS_NTF, PacketHandler.S_PARTY_STATUS_NTFHandler);
		_onRecv.Add((ushort)MsgId.S_DUNGEON_ENTER_RES, MakePacket<S_DUNGEON_ENTER_RES>);
		_handler.Add((ushort)MsgId.S_DUNGEON_ENTER_RES, PacketHandler.S_DUNGEON_ENTER_RESHandler);
		_onRecv.Add((ushort)MsgId.S_DUNGEON_EXIT_RES, MakePacket<S_DUNGEON_EXIT_RES>);
		_handler.Add((ushort)MsgId.S_DUNGEON_EXIT_RES, PacketHandler.S_DUNGEON_EXIT_RESHandler);
		_onRecv.Add((ushort)MsgId.S_QUICKSLOT_LIST, MakePacket<S_QUICKSLOT_LIST>);
		_handler.Add((ushort)MsgId.S_QUICKSLOT_LIST, PacketHandler.S_QUICKSLOT_LISTHandler);
		_onRecv.Add((ushort)MsgId.S_SET_QUICKSLOT, MakePacket<S_SET_QUICKSLOT>);
		_handler.Add((ushort)MsgId.S_SET_QUICKSLOT, PacketHandler.S_SET_QUICKSLOTHandler);
		_onRecv.Add((ushort)MsgId.S_TRADE_INVITE, MakePacket<S_TRADE_INVITE>);
		_handler.Add((ushort)MsgId.S_TRADE_INVITE, PacketHandler.S_TRADE_INVITEHandler);
		_onRecv.Add((ushort)MsgId.S_TRADE_START, MakePacket<S_TRADE_START>);
		_handler.Add((ushort)MsgId.S_TRADE_START, PacketHandler.S_TRADE_STARTHandler);
		_onRecv.Add((ushort)MsgId.S_TRADE_OFFER_UPDATE, MakePacket<S_TRADE_OFFER_UPDATE>);
		_handler.Add((ushort)MsgId.S_TRADE_OFFER_UPDATE, PacketHandler.S_TRADE_OFFER_UPDATEHandler);
		_onRecv.Add((ushort)MsgId.S_TRADE_READY_STATE, MakePacket<S_TRADE_READY_STATE>);
		_handler.Add((ushort)MsgId.S_TRADE_READY_STATE, PacketHandler.S_TRADE_READY_STATEHandler);
		_onRecv.Add((ushort)MsgId.S_TRADE_LOCKED, MakePacket<S_TRADE_LOCKED>);
		_handler.Add((ushort)MsgId.S_TRADE_LOCKED, PacketHandler.S_TRADE_LOCKEDHandler);
		_onRecv.Add((ushort)MsgId.S_TRADE_CANCELLED, MakePacket<S_TRADE_CANCELLED>);
		_handler.Add((ushort)MsgId.S_TRADE_CANCELLED, PacketHandler.S_TRADE_CANCELLEDHandler);
		_onRecv.Add((ushort)MsgId.S_TRADE_RESULT, MakePacket<S_TRADE_RESULT>);
		_handler.Add((ushort)MsgId.S_TRADE_RESULT, PacketHandler.S_TRADE_RESULTHandler);
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