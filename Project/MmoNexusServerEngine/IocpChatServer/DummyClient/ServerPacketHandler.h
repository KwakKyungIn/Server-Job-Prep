#pragma once
#include "Protocol.pb.h"
#include "Crc32.h" // [GIGACHAD] CRC 모듈 포함

using PacketHandlerFunc = std::function<bool(PacketSessionRef&, BYTE*, int32)>;

class ServerPacketHandler
{
public:
	enum : uint16
	{
		PKT_C_LOGIN = 1000,
		PKT_S_LOGIN = 1001,
		PKT_C_ENTER_GAME = 1002,
		PKT_S_ENTER_GAME = 1003,
		PKT_C_MOVE = 1004,
		PKT_S_MOVE = 1005,
		PKT_S_SPAWN = 1006,
		PKT_S_DESPAWN = 1007,
		PKT_C_SKILL = 1008,
		PKT_S_SKILL = 1009,
		PKT_S_CHANGE_HP = 1010,
		PKT_S_ITEM_LIST = 1011,
		PKT_C_USE_ITEM = 1012,
		PKT_S_CHANGE_ITEM = 1013,
		PKT_S_REMOVE_ITEM = 1014,
		PKT_C_EQUIP_ITEM = 1015,
		PKT_S_EQUIP_ITEM = 1016,
		PKT_S_CHANGE_STAT = 1017,
		PKT_S_GOLD_UPDATE = 1018,
		PKT_C_MAP_CHANGE_REQ = 1019,
		PKT_S_MAP_CHANGE_BEGIN = 1020,
		PKT_C_MAP_CHANGE_ACK = 1021,
		PKT_S_MAP_CHANGE_END = 1022,
		PKT_C_CHANNEL_CHANGE_REQ = 1023,
		PKT_C_CHAT_REQ = 1024,
		PKT_S_CHAT_RES = 1025,
		PKT_S_CHAT_NTF = 1026,
		PKT_S_HEART_BEAT_RES = 1027,
		PKT_C_HEART_BEAT_REQ = 1028,
		PKT_C_PARTY_CHAT_REQ = 1029,
		PKT_S_PARTY_CHAT_NTF = 1030,
		PKT_S_PARTY_INFO_NTF = 1031,
		PKT_C_PARTY_CREATE_REQ = 1032,
		PKT_C_PARTY_INVITE_REQ = 1033,
		PKT_C_PARTY_INVITE_ACCEPT_REQ = 1034,
		PKT_C_PARTY_LEAVE_REQ = 1035,
		PKT_C_PARTY_KICK_REQ = 1036,
		PKT_C_PARTY_DISBAND_REQ = 1037,
		PKT_S_PARTY_RESULT = 1038,
		PKT_S_PARTY_INVITE_NTF = 1039,
		PKT_C_PARTY_STATUS_REQ = 1040,
		PKT_S_PARTY_STATUS_NTF = 1041,
		PKT_C_DUNGEON_ENTER_REQ = 1042,
		PKT_S_DUNGEON_ENTER_RES = 1043,
		PKT_C_DUNGEON_EXIT_REQ = 1044,
		PKT_S_DUNGEON_EXIT_RES = 1045,
		PKT_S_QUICKSLOT_LIST = 1046,
		PKT_C_SET_QUICKSLOT = 1047,
		PKT_S_SET_QUICKSLOT = 1048,
		PKT_C_TRADE_REQ = 1049,
		PKT_S_TRADE_INVITE = 1050,
		PKT_C_TRADE_INVITE_RESP = 1051,
		PKT_S_TRADE_START = 1052,
		PKT_C_TRADE_OFFER_SET = 1053,
		PKT_C_TRADE_GOLD_SET = 1054,
		PKT_S_TRADE_OFFER_UPDATE = 1055,
		PKT_C_TRADE_READY = 1056,
		PKT_S_TRADE_READY_STATE = 1057,
		PKT_S_TRADE_LOCKED = 1058,
		PKT_C_TRADE_CONFIRM = 1059,
		PKT_C_TRADE_CANCEL = 1060,
		PKT_S_TRADE_CANCELLED = 1061,
		PKT_S_TRADE_RESULT = 1062,
		PKT_C_INV_DRAG_DROP = 1063,
	};

	static void Init()
	{
		for (int32 i = 0; i < UINT16_MAX; i++)
			GPacketHandler[i] = Handle_INVALID;
		GPacketHandler[PKT_S_LOGIN] = [](PacketSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::S_LOGIN>(Handle_S_LOGIN, session, buffer, len); };
		GPacketHandler[PKT_S_ENTER_GAME] = [](PacketSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::S_ENTER_GAME>(Handle_S_ENTER_GAME, session, buffer, len); };
		GPacketHandler[PKT_S_MOVE] = [](PacketSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::S_MOVE>(Handle_S_MOVE, session, buffer, len); };
		GPacketHandler[PKT_S_SPAWN] = [](PacketSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::S_SPAWN>(Handle_S_SPAWN, session, buffer, len); };
		GPacketHandler[PKT_S_DESPAWN] = [](PacketSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::S_DESPAWN>(Handle_S_DESPAWN, session, buffer, len); };
		GPacketHandler[PKT_S_SKILL] = [](PacketSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::S_SKILL>(Handle_S_SKILL, session, buffer, len); };
		GPacketHandler[PKT_S_CHANGE_HP] = [](PacketSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::S_CHANGE_HP>(Handle_S_CHANGE_HP, session, buffer, len); };
		GPacketHandler[PKT_S_ITEM_LIST] = [](PacketSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::S_ITEM_LIST>(Handle_S_ITEM_LIST, session, buffer, len); };
		GPacketHandler[PKT_S_CHANGE_ITEM] = [](PacketSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::S_CHANGE_ITEM>(Handle_S_CHANGE_ITEM, session, buffer, len); };
		GPacketHandler[PKT_S_REMOVE_ITEM] = [](PacketSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::S_REMOVE_ITEM>(Handle_S_REMOVE_ITEM, session, buffer, len); };
		GPacketHandler[PKT_S_EQUIP_ITEM] = [](PacketSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::S_EQUIP_ITEM>(Handle_S_EQUIP_ITEM, session, buffer, len); };
		GPacketHandler[PKT_S_CHANGE_STAT] = [](PacketSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::S_CHANGE_STAT>(Handle_S_CHANGE_STAT, session, buffer, len); };
		GPacketHandler[PKT_S_GOLD_UPDATE] = [](PacketSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::S_GOLD_UPDATE>(Handle_S_GOLD_UPDATE, session, buffer, len); };
		GPacketHandler[PKT_S_MAP_CHANGE_BEGIN] = [](PacketSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::S_MAP_CHANGE_BEGIN>(Handle_S_MAP_CHANGE_BEGIN, session, buffer, len); };
		GPacketHandler[PKT_S_MAP_CHANGE_END] = [](PacketSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::S_MAP_CHANGE_END>(Handle_S_MAP_CHANGE_END, session, buffer, len); };
		GPacketHandler[PKT_S_CHAT_RES] = [](PacketSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::S_CHAT_RES>(Handle_S_CHAT_RES, session, buffer, len); };
		GPacketHandler[PKT_S_CHAT_NTF] = [](PacketSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::S_CHAT_NTF>(Handle_S_CHAT_NTF, session, buffer, len); };
		GPacketHandler[PKT_S_HEART_BEAT_RES] = [](PacketSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::S_HEART_BEAT_RES>(Handle_S_HEART_BEAT_RES, session, buffer, len); };
		GPacketHandler[PKT_S_PARTY_CHAT_NTF] = [](PacketSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::S_PARTY_CHAT_NTF>(Handle_S_PARTY_CHAT_NTF, session, buffer, len); };
		GPacketHandler[PKT_S_PARTY_INFO_NTF] = [](PacketSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::S_PARTY_INFO_NTF>(Handle_S_PARTY_INFO_NTF, session, buffer, len); };
		GPacketHandler[PKT_S_PARTY_RESULT] = [](PacketSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::S_PARTY_RESULT>(Handle_S_PARTY_RESULT, session, buffer, len); };
		GPacketHandler[PKT_S_PARTY_INVITE_NTF] = [](PacketSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::S_PARTY_INVITE_NTF>(Handle_S_PARTY_INVITE_NTF, session, buffer, len); };
		GPacketHandler[PKT_S_PARTY_STATUS_NTF] = [](PacketSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::S_PARTY_STATUS_NTF>(Handle_S_PARTY_STATUS_NTF, session, buffer, len); };
		GPacketHandler[PKT_S_DUNGEON_ENTER_RES] = [](PacketSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::S_DUNGEON_ENTER_RES>(Handle_S_DUNGEON_ENTER_RES, session, buffer, len); };
		GPacketHandler[PKT_S_DUNGEON_EXIT_RES] = [](PacketSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::S_DUNGEON_EXIT_RES>(Handle_S_DUNGEON_EXIT_RES, session, buffer, len); };
		GPacketHandler[PKT_S_QUICKSLOT_LIST] = [](PacketSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::S_QUICKSLOT_LIST>(Handle_S_QUICKSLOT_LIST, session, buffer, len); };
		GPacketHandler[PKT_S_SET_QUICKSLOT] = [](PacketSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::S_SET_QUICKSLOT>(Handle_S_SET_QUICKSLOT, session, buffer, len); };
		GPacketHandler[PKT_S_TRADE_INVITE] = [](PacketSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::S_TRADE_INVITE>(Handle_S_TRADE_INVITE, session, buffer, len); };
		GPacketHandler[PKT_S_TRADE_START] = [](PacketSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::S_TRADE_START>(Handle_S_TRADE_START, session, buffer, len); };
		GPacketHandler[PKT_S_TRADE_OFFER_UPDATE] = [](PacketSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::S_TRADE_OFFER_UPDATE>(Handle_S_TRADE_OFFER_UPDATE, session, buffer, len); };
		GPacketHandler[PKT_S_TRADE_READY_STATE] = [](PacketSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::S_TRADE_READY_STATE>(Handle_S_TRADE_READY_STATE, session, buffer, len); };
		GPacketHandler[PKT_S_TRADE_LOCKED] = [](PacketSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::S_TRADE_LOCKED>(Handle_S_TRADE_LOCKED, session, buffer, len); };
		GPacketHandler[PKT_S_TRADE_CANCELLED] = [](PacketSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::S_TRADE_CANCELLED>(Handle_S_TRADE_CANCELLED, session, buffer, len); };
		GPacketHandler[PKT_S_TRADE_RESULT] = [](PacketSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::S_TRADE_RESULT>(Handle_S_TRADE_RESULT, session, buffer, len); };
	}

	static bool HandlePacket(PacketSessionRef& session, BYTE* buffer, int32 len)
	{
		PacketHeader* header = reinterpret_cast<PacketHeader*>(buffer);
		return GPacketHandler[header->id](session, buffer, len);
	}
	static SendBufferRef MakeSendBuffer(Protocol::C_LOGIN& pkt) { return MakeSendBuffer(pkt, PKT_C_LOGIN); }
	static SendBufferRef MakeSendBuffer(Protocol::C_ENTER_GAME& pkt) { return MakeSendBuffer(pkt, PKT_C_ENTER_GAME); }
	static SendBufferRef MakeSendBuffer(Protocol::C_MOVE& pkt) { return MakeSendBuffer(pkt, PKT_C_MOVE); }
	static SendBufferRef MakeSendBuffer(Protocol::C_SKILL& pkt) { return MakeSendBuffer(pkt, PKT_C_SKILL); }
	static SendBufferRef MakeSendBuffer(Protocol::C_USE_ITEM& pkt) { return MakeSendBuffer(pkt, PKT_C_USE_ITEM); }
	static SendBufferRef MakeSendBuffer(Protocol::C_EQUIP_ITEM& pkt) { return MakeSendBuffer(pkt, PKT_C_EQUIP_ITEM); }
	static SendBufferRef MakeSendBuffer(Protocol::C_MAP_CHANGE_REQ& pkt) { return MakeSendBuffer(pkt, PKT_C_MAP_CHANGE_REQ); }
	static SendBufferRef MakeSendBuffer(Protocol::C_MAP_CHANGE_ACK& pkt) { return MakeSendBuffer(pkt, PKT_C_MAP_CHANGE_ACK); }
	static SendBufferRef MakeSendBuffer(Protocol::C_CHANNEL_CHANGE_REQ& pkt) { return MakeSendBuffer(pkt, PKT_C_CHANNEL_CHANGE_REQ); }
	static SendBufferRef MakeSendBuffer(Protocol::C_CHAT_REQ& pkt) { return MakeSendBuffer(pkt, PKT_C_CHAT_REQ); }
	static SendBufferRef MakeSendBuffer(Protocol::C_HEART_BEAT_REQ& pkt) { return MakeSendBuffer(pkt, PKT_C_HEART_BEAT_REQ); }
	static SendBufferRef MakeSendBuffer(Protocol::C_PARTY_CHAT_REQ& pkt) { return MakeSendBuffer(pkt, PKT_C_PARTY_CHAT_REQ); }
	static SendBufferRef MakeSendBuffer(Protocol::C_PARTY_CREATE_REQ& pkt) { return MakeSendBuffer(pkt, PKT_C_PARTY_CREATE_REQ); }
	static SendBufferRef MakeSendBuffer(Protocol::C_PARTY_INVITE_REQ& pkt) { return MakeSendBuffer(pkt, PKT_C_PARTY_INVITE_REQ); }
	static SendBufferRef MakeSendBuffer(Protocol::C_PARTY_INVITE_ACCEPT_REQ& pkt) { return MakeSendBuffer(pkt, PKT_C_PARTY_INVITE_ACCEPT_REQ); }
	static SendBufferRef MakeSendBuffer(Protocol::C_PARTY_LEAVE_REQ& pkt) { return MakeSendBuffer(pkt, PKT_C_PARTY_LEAVE_REQ); }
	static SendBufferRef MakeSendBuffer(Protocol::C_PARTY_KICK_REQ& pkt) { return MakeSendBuffer(pkt, PKT_C_PARTY_KICK_REQ); }
	static SendBufferRef MakeSendBuffer(Protocol::C_PARTY_DISBAND_REQ& pkt) { return MakeSendBuffer(pkt, PKT_C_PARTY_DISBAND_REQ); }
	static SendBufferRef MakeSendBuffer(Protocol::C_PARTY_STATUS_REQ& pkt) { return MakeSendBuffer(pkt, PKT_C_PARTY_STATUS_REQ); }
	static SendBufferRef MakeSendBuffer(Protocol::C_DUNGEON_ENTER_REQ& pkt) { return MakeSendBuffer(pkt, PKT_C_DUNGEON_ENTER_REQ); }
	static SendBufferRef MakeSendBuffer(Protocol::C_DUNGEON_EXIT_REQ& pkt) { return MakeSendBuffer(pkt, PKT_C_DUNGEON_EXIT_REQ); }
	static SendBufferRef MakeSendBuffer(Protocol::C_SET_QUICKSLOT& pkt) { return MakeSendBuffer(pkt, PKT_C_SET_QUICKSLOT); }
	static SendBufferRef MakeSendBuffer(Protocol::C_TRADE_REQ& pkt) { return MakeSendBuffer(pkt, PKT_C_TRADE_REQ); }
	static SendBufferRef MakeSendBuffer(Protocol::C_TRADE_INVITE_RESP& pkt) { return MakeSendBuffer(pkt, PKT_C_TRADE_INVITE_RESP); }
	static SendBufferRef MakeSendBuffer(Protocol::C_TRADE_OFFER_SET& pkt) { return MakeSendBuffer(pkt, PKT_C_TRADE_OFFER_SET); }
	static SendBufferRef MakeSendBuffer(Protocol::C_TRADE_GOLD_SET& pkt) { return MakeSendBuffer(pkt, PKT_C_TRADE_GOLD_SET); }
	static SendBufferRef MakeSendBuffer(Protocol::C_TRADE_READY& pkt) { return MakeSendBuffer(pkt, PKT_C_TRADE_READY); }
	static SendBufferRef MakeSendBuffer(Protocol::C_TRADE_CONFIRM& pkt) { return MakeSendBuffer(pkt, PKT_C_TRADE_CONFIRM); }
	static SendBufferRef MakeSendBuffer(Protocol::C_TRADE_CANCEL& pkt) { return MakeSendBuffer(pkt, PKT_C_TRADE_CANCEL); }
	static SendBufferRef MakeSendBuffer(Protocol::C_INV_DRAG_DROP& pkt) { return MakeSendBuffer(pkt, PKT_C_INV_DRAG_DROP); }

public:
	static PacketHandlerFunc GPacketHandler[UINT16_MAX];
	static bool Handle_INVALID(PacketSessionRef& session, BYTE* buffer, int32 len);
	static bool Handle_S_LOGIN(PacketSessionRef& session, Protocol::S_LOGIN& pkt);
	static bool Handle_S_ENTER_GAME(PacketSessionRef& session, Protocol::S_ENTER_GAME& pkt);
	static bool Handle_S_MOVE(PacketSessionRef& session, Protocol::S_MOVE& pkt);
	static bool Handle_S_SPAWN(PacketSessionRef& session, Protocol::S_SPAWN& pkt);
	static bool Handle_S_DESPAWN(PacketSessionRef& session, Protocol::S_DESPAWN& pkt);
	static bool Handle_S_SKILL(PacketSessionRef& session, Protocol::S_SKILL& pkt);
	static bool Handle_S_CHANGE_HP(PacketSessionRef& session, Protocol::S_CHANGE_HP& pkt);
	static bool Handle_S_ITEM_LIST(PacketSessionRef& session, Protocol::S_ITEM_LIST& pkt);
	static bool Handle_S_CHANGE_ITEM(PacketSessionRef& session, Protocol::S_CHANGE_ITEM& pkt);
	static bool Handle_S_REMOVE_ITEM(PacketSessionRef& session, Protocol::S_REMOVE_ITEM& pkt);
	static bool Handle_S_EQUIP_ITEM(PacketSessionRef& session, Protocol::S_EQUIP_ITEM& pkt);
	static bool Handle_S_CHANGE_STAT(PacketSessionRef& session, Protocol::S_CHANGE_STAT& pkt);
	static bool Handle_S_GOLD_UPDATE(PacketSessionRef& session, Protocol::S_GOLD_UPDATE& pkt);
	static bool Handle_S_MAP_CHANGE_BEGIN(PacketSessionRef& session, Protocol::S_MAP_CHANGE_BEGIN& pkt);
	static bool Handle_S_MAP_CHANGE_END(PacketSessionRef& session, Protocol::S_MAP_CHANGE_END& pkt);
	static bool Handle_S_CHAT_RES(PacketSessionRef& session, Protocol::S_CHAT_RES& pkt);
	static bool Handle_S_CHAT_NTF(PacketSessionRef& session, Protocol::S_CHAT_NTF& pkt);
	static bool Handle_S_HEART_BEAT_RES(PacketSessionRef& session, Protocol::S_HEART_BEAT_RES& pkt);
	static bool Handle_S_PARTY_CHAT_NTF(PacketSessionRef& session, Protocol::S_PARTY_CHAT_NTF& pkt);
	static bool Handle_S_PARTY_INFO_NTF(PacketSessionRef& session, Protocol::S_PARTY_INFO_NTF& pkt);
	static bool Handle_S_PARTY_RESULT(PacketSessionRef& session, Protocol::S_PARTY_RESULT& pkt);
	static bool Handle_S_PARTY_INVITE_NTF(PacketSessionRef& session, Protocol::S_PARTY_INVITE_NTF& pkt);
	static bool Handle_S_PARTY_STATUS_NTF(PacketSessionRef& session, Protocol::S_PARTY_STATUS_NTF& pkt);
	static bool Handle_S_DUNGEON_ENTER_RES(PacketSessionRef& session, Protocol::S_DUNGEON_ENTER_RES& pkt);
	static bool Handle_S_DUNGEON_EXIT_RES(PacketSessionRef& session, Protocol::S_DUNGEON_EXIT_RES& pkt);
	static bool Handle_S_QUICKSLOT_LIST(PacketSessionRef& session, Protocol::S_QUICKSLOT_LIST& pkt);
	static bool Handle_S_SET_QUICKSLOT(PacketSessionRef& session, Protocol::S_SET_QUICKSLOT& pkt);
	static bool Handle_S_TRADE_INVITE(PacketSessionRef& session, Protocol::S_TRADE_INVITE& pkt);
	static bool Handle_S_TRADE_START(PacketSessionRef& session, Protocol::S_TRADE_START& pkt);
	static bool Handle_S_TRADE_OFFER_UPDATE(PacketSessionRef& session, Protocol::S_TRADE_OFFER_UPDATE& pkt);
	static bool Handle_S_TRADE_READY_STATE(PacketSessionRef& session, Protocol::S_TRADE_READY_STATE& pkt);
	static bool Handle_S_TRADE_LOCKED(PacketSessionRef& session, Protocol::S_TRADE_LOCKED& pkt);
	static bool Handle_S_TRADE_CANCELLED(PacketSessionRef& session, Protocol::S_TRADE_CANCELLED& pkt);
	static bool Handle_S_TRADE_RESULT(PacketSessionRef& session, Protocol::S_TRADE_RESULT& pkt);

private:
	template<typename PacketType, typename ProcessFunc>
	static bool HandlePacket(ProcessFunc func, PacketSessionRef& session, BYTE* buffer, int32 len)
	{
		PacketHeader* header = reinterpret_cast<PacketHeader*>(buffer);
		int32 dataSize = len - sizeof(PacketHeader);

		// [GIGACHAD] 1. CRC Check (무결성 검사)
		// 보낼 때 Body만 계산했다고 가정.
		uint32 calcCrc = Crc32::Compute(buffer + sizeof(PacketHeader), dataSize);
		if (header->crc != calcCrc)
		{
			// CRC 불일치 = 데이터 깨짐 or 변조
			return false; 
		}

		// [GIGACHAD] 2. Seq Check (Replay Attack 방지)
		if (session->CheckRecvSeq(header->seq) == false)
		{
			// 이미 처리한 패킷이 다시 옴
			return false;
		}

		// [GIGACHAD] 3. Decrypt (암호화 해제)
		XorCrypt(buffer + sizeof(PacketHeader), dataSize);

		// [GIGACHAD] 4. Parse
		PacketType pkt;
		if (pkt.ParseFromArray(buffer + sizeof(PacketHeader), dataSize) == false)
			return false;

		return func(session, pkt);
	}

	template<typename T>
	static SendBufferRef MakeSendBuffer(T& pkt, uint16 pktId)
	{
		const uint16 dataSize = static_cast<uint16>(pkt.ByteSizeLong());
		const uint16 packetSize = dataSize + sizeof(PacketHeader);

		SendBufferRef sendBuffer = GSendBufferManager->Open(packetSize);
		PacketHeader* header = reinterpret_cast<PacketHeader*>(sendBuffer->Buffer());
		header->size = packetSize;
		header->id = pktId;
		
		// [Seq]와 [CRC]는 여기서 0으로 둠. (Session::Send에서 채움)
		header->seq = 0;
		header->crc = 0;

		// 1. 직렬화
		ASSERT_CRASH(pkt.SerializeToArray(&header[1], dataSize));

		// 2. 암호화 (Seq, CRC 계산 전에 본문을 먼저 암호화해두는 게 일반적)
		XorCrypt(reinterpret_cast<BYTE*>(&header[1]), dataSize);

		sendBuffer->Close(packetSize);

		return sendBuffer;
	}

	static void XorCrypt(BYTE* buffer, int32 len)
	{
		const BYTE xorKey = 0x5A; 
		for (int32 i = 0; i < len; i++)
			buffer[i] ^= xorKey;
	}
};