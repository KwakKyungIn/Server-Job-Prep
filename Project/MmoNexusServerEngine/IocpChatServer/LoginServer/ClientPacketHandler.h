#pragma once
#include "Protocol.pb.h"
#include "Crc32.h" // [GIGACHAD] CRC 모듈 포함

using PacketHandlerFunc = std::function<bool(PacketSessionRef&, BYTE*, int32)>;

class ClientPacketHandler
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
		PKT_C_MAP_CHANGE_REQ = 1018,
		PKT_S_MAP_CHANGE_BEGIN = 1019,
		PKT_C_MAP_CHANGE_ACK = 1020,
		PKT_S_MAP_CHANGE_END = 1021,
		PKT_C_CHANNEL_CHANGE_REQ = 1022,
		PKT_C_CHAT_REQ = 1023,
		PKT_S_CHAT_RES = 1024,
		PKT_S_CHAT_NTF = 1025,
		PKT_S_HEART_BEAT_RES = 1026,
		PKT_C_HEART_BEAT_REQ = 1027,
		PKT_C_PARTY_CHAT_REQ = 1028,
		PKT_S_PARTY_CHAT_NTF = 1029,
		PKT_S_PARTY_INFO_NTF = 1030,
		PKT_C_PARTY_CREATE_REQ = 1031,
		PKT_C_PARTY_INVITE_REQ = 1032,
		PKT_C_PARTY_INVITE_ACCEPT_REQ = 1033,
		PKT_C_PARTY_LEAVE_REQ = 1034,
		PKT_C_PARTY_KICK_REQ = 1035,
		PKT_C_PARTY_DISBAND_REQ = 1036,
		PKT_S_PARTY_RESULT = 1037,
		PKT_S_PARTY_INVITE_NTF = 1038,
		PKT_C_PARTY_STATUS_REQ = 1039,
		PKT_S_PARTY_STATUS_NTF = 1040,
		PKT_C_DUNGEON_ENTER_REQ = 1041,
		PKT_S_DUNGEON_ENTER_RES = 1042,
		PKT_C_DUNGEON_EXIT_REQ = 1043,
		PKT_S_DUNGEON_EXIT_RES = 1044,
	};

	static void Init()
	{
		for (int32 i = 0; i < UINT16_MAX; i++)
			GPacketHandler[i] = Handle_INVALID;
		GPacketHandler[PKT_C_LOGIN] = [](PacketSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::C_LOGIN>(Handle_C_LOGIN, session, buffer, len); };
		GPacketHandler[PKT_C_ENTER_GAME] = [](PacketSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::C_ENTER_GAME>(Handle_C_ENTER_GAME, session, buffer, len); };
		GPacketHandler[PKT_C_MOVE] = [](PacketSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::C_MOVE>(Handle_C_MOVE, session, buffer, len); };
		GPacketHandler[PKT_C_SKILL] = [](PacketSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::C_SKILL>(Handle_C_SKILL, session, buffer, len); };
		GPacketHandler[PKT_C_USE_ITEM] = [](PacketSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::C_USE_ITEM>(Handle_C_USE_ITEM, session, buffer, len); };
		GPacketHandler[PKT_C_EQUIP_ITEM] = [](PacketSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::C_EQUIP_ITEM>(Handle_C_EQUIP_ITEM, session, buffer, len); };
		GPacketHandler[PKT_C_MAP_CHANGE_REQ] = [](PacketSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::C_MAP_CHANGE_REQ>(Handle_C_MAP_CHANGE_REQ, session, buffer, len); };
		GPacketHandler[PKT_C_MAP_CHANGE_ACK] = [](PacketSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::C_MAP_CHANGE_ACK>(Handle_C_MAP_CHANGE_ACK, session, buffer, len); };
		GPacketHandler[PKT_C_CHANNEL_CHANGE_REQ] = [](PacketSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::C_CHANNEL_CHANGE_REQ>(Handle_C_CHANNEL_CHANGE_REQ, session, buffer, len); };
		GPacketHandler[PKT_C_CHAT_REQ] = [](PacketSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::C_CHAT_REQ>(Handle_C_CHAT_REQ, session, buffer, len); };
		GPacketHandler[PKT_C_HEART_BEAT_REQ] = [](PacketSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::C_HEART_BEAT_REQ>(Handle_C_HEART_BEAT_REQ, session, buffer, len); };
		GPacketHandler[PKT_C_PARTY_CHAT_REQ] = [](PacketSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::C_PARTY_CHAT_REQ>(Handle_C_PARTY_CHAT_REQ, session, buffer, len); };
		GPacketHandler[PKT_C_PARTY_CREATE_REQ] = [](PacketSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::C_PARTY_CREATE_REQ>(Handle_C_PARTY_CREATE_REQ, session, buffer, len); };
		GPacketHandler[PKT_C_PARTY_INVITE_REQ] = [](PacketSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::C_PARTY_INVITE_REQ>(Handle_C_PARTY_INVITE_REQ, session, buffer, len); };
		GPacketHandler[PKT_C_PARTY_INVITE_ACCEPT_REQ] = [](PacketSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::C_PARTY_INVITE_ACCEPT_REQ>(Handle_C_PARTY_INVITE_ACCEPT_REQ, session, buffer, len); };
		GPacketHandler[PKT_C_PARTY_LEAVE_REQ] = [](PacketSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::C_PARTY_LEAVE_REQ>(Handle_C_PARTY_LEAVE_REQ, session, buffer, len); };
		GPacketHandler[PKT_C_PARTY_KICK_REQ] = [](PacketSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::C_PARTY_KICK_REQ>(Handle_C_PARTY_KICK_REQ, session, buffer, len); };
		GPacketHandler[PKT_C_PARTY_DISBAND_REQ] = [](PacketSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::C_PARTY_DISBAND_REQ>(Handle_C_PARTY_DISBAND_REQ, session, buffer, len); };
		GPacketHandler[PKT_C_PARTY_STATUS_REQ] = [](PacketSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::C_PARTY_STATUS_REQ>(Handle_C_PARTY_STATUS_REQ, session, buffer, len); };
		GPacketHandler[PKT_C_DUNGEON_ENTER_REQ] = [](PacketSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::C_DUNGEON_ENTER_REQ>(Handle_C_DUNGEON_ENTER_REQ, session, buffer, len); };
		GPacketHandler[PKT_C_DUNGEON_EXIT_REQ] = [](PacketSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::C_DUNGEON_EXIT_REQ>(Handle_C_DUNGEON_EXIT_REQ, session, buffer, len); };
	}

	static bool HandlePacket(PacketSessionRef& session, BYTE* buffer, int32 len)
	{
		PacketHeader* header = reinterpret_cast<PacketHeader*>(buffer);
		return GPacketHandler[header->id](session, buffer, len);
	}
	static SendBufferRef MakeSendBuffer(Protocol::S_LOGIN& pkt) { return MakeSendBuffer(pkt, PKT_S_LOGIN); }
	static SendBufferRef MakeSendBuffer(Protocol::S_ENTER_GAME& pkt) { return MakeSendBuffer(pkt, PKT_S_ENTER_GAME); }
	static SendBufferRef MakeSendBuffer(Protocol::S_MOVE& pkt) { return MakeSendBuffer(pkt, PKT_S_MOVE); }
	static SendBufferRef MakeSendBuffer(Protocol::S_SPAWN& pkt) { return MakeSendBuffer(pkt, PKT_S_SPAWN); }
	static SendBufferRef MakeSendBuffer(Protocol::S_DESPAWN& pkt) { return MakeSendBuffer(pkt, PKT_S_DESPAWN); }
	static SendBufferRef MakeSendBuffer(Protocol::S_SKILL& pkt) { return MakeSendBuffer(pkt, PKT_S_SKILL); }
	static SendBufferRef MakeSendBuffer(Protocol::S_CHANGE_HP& pkt) { return MakeSendBuffer(pkt, PKT_S_CHANGE_HP); }
	static SendBufferRef MakeSendBuffer(Protocol::S_ITEM_LIST& pkt) { return MakeSendBuffer(pkt, PKT_S_ITEM_LIST); }
	static SendBufferRef MakeSendBuffer(Protocol::S_CHANGE_ITEM& pkt) { return MakeSendBuffer(pkt, PKT_S_CHANGE_ITEM); }
	static SendBufferRef MakeSendBuffer(Protocol::S_REMOVE_ITEM& pkt) { return MakeSendBuffer(pkt, PKT_S_REMOVE_ITEM); }
	static SendBufferRef MakeSendBuffer(Protocol::S_EQUIP_ITEM& pkt) { return MakeSendBuffer(pkt, PKT_S_EQUIP_ITEM); }
	static SendBufferRef MakeSendBuffer(Protocol::S_CHANGE_STAT& pkt) { return MakeSendBuffer(pkt, PKT_S_CHANGE_STAT); }
	static SendBufferRef MakeSendBuffer(Protocol::S_MAP_CHANGE_BEGIN& pkt) { return MakeSendBuffer(pkt, PKT_S_MAP_CHANGE_BEGIN); }
	static SendBufferRef MakeSendBuffer(Protocol::S_MAP_CHANGE_END& pkt) { return MakeSendBuffer(pkt, PKT_S_MAP_CHANGE_END); }
	static SendBufferRef MakeSendBuffer(Protocol::S_CHAT_RES& pkt) { return MakeSendBuffer(pkt, PKT_S_CHAT_RES); }
	static SendBufferRef MakeSendBuffer(Protocol::S_CHAT_NTF& pkt) { return MakeSendBuffer(pkt, PKT_S_CHAT_NTF); }
	static SendBufferRef MakeSendBuffer(Protocol::S_HEART_BEAT_RES& pkt) { return MakeSendBuffer(pkt, PKT_S_HEART_BEAT_RES); }
	static SendBufferRef MakeSendBuffer(Protocol::S_PARTY_CHAT_NTF& pkt) { return MakeSendBuffer(pkt, PKT_S_PARTY_CHAT_NTF); }
	static SendBufferRef MakeSendBuffer(Protocol::S_PARTY_INFO_NTF& pkt) { return MakeSendBuffer(pkt, PKT_S_PARTY_INFO_NTF); }
	static SendBufferRef MakeSendBuffer(Protocol::S_PARTY_RESULT& pkt) { return MakeSendBuffer(pkt, PKT_S_PARTY_RESULT); }
	static SendBufferRef MakeSendBuffer(Protocol::S_PARTY_INVITE_NTF& pkt) { return MakeSendBuffer(pkt, PKT_S_PARTY_INVITE_NTF); }
	static SendBufferRef MakeSendBuffer(Protocol::S_PARTY_STATUS_NTF& pkt) { return MakeSendBuffer(pkt, PKT_S_PARTY_STATUS_NTF); }
	static SendBufferRef MakeSendBuffer(Protocol::S_DUNGEON_ENTER_RES& pkt) { return MakeSendBuffer(pkt, PKT_S_DUNGEON_ENTER_RES); }
	static SendBufferRef MakeSendBuffer(Protocol::S_DUNGEON_EXIT_RES& pkt) { return MakeSendBuffer(pkt, PKT_S_DUNGEON_EXIT_RES); }

public:
	static PacketHandlerFunc GPacketHandler[UINT16_MAX];
	static bool Handle_INVALID(PacketSessionRef& session, BYTE* buffer, int32 len);
	static bool Handle_C_LOGIN(PacketSessionRef& session, Protocol::C_LOGIN& pkt);
	static bool Handle_C_ENTER_GAME(PacketSessionRef& session, Protocol::C_ENTER_GAME& pkt);
	static bool Handle_C_MOVE(PacketSessionRef& session, Protocol::C_MOVE& pkt);
	static bool Handle_C_SKILL(PacketSessionRef& session, Protocol::C_SKILL& pkt);
	static bool Handle_C_USE_ITEM(PacketSessionRef& session, Protocol::C_USE_ITEM& pkt);
	static bool Handle_C_EQUIP_ITEM(PacketSessionRef& session, Protocol::C_EQUIP_ITEM& pkt);
	static bool Handle_C_MAP_CHANGE_REQ(PacketSessionRef& session, Protocol::C_MAP_CHANGE_REQ& pkt);
	static bool Handle_C_MAP_CHANGE_ACK(PacketSessionRef& session, Protocol::C_MAP_CHANGE_ACK& pkt);
	static bool Handle_C_CHANNEL_CHANGE_REQ(PacketSessionRef& session, Protocol::C_CHANNEL_CHANGE_REQ& pkt);
	static bool Handle_C_CHAT_REQ(PacketSessionRef& session, Protocol::C_CHAT_REQ& pkt);
	static bool Handle_C_HEART_BEAT_REQ(PacketSessionRef& session, Protocol::C_HEART_BEAT_REQ& pkt);
	static bool Handle_C_PARTY_CHAT_REQ(PacketSessionRef& session, Protocol::C_PARTY_CHAT_REQ& pkt);
	static bool Handle_C_PARTY_CREATE_REQ(PacketSessionRef& session, Protocol::C_PARTY_CREATE_REQ& pkt);
	static bool Handle_C_PARTY_INVITE_REQ(PacketSessionRef& session, Protocol::C_PARTY_INVITE_REQ& pkt);
	static bool Handle_C_PARTY_INVITE_ACCEPT_REQ(PacketSessionRef& session, Protocol::C_PARTY_INVITE_ACCEPT_REQ& pkt);
	static bool Handle_C_PARTY_LEAVE_REQ(PacketSessionRef& session, Protocol::C_PARTY_LEAVE_REQ& pkt);
	static bool Handle_C_PARTY_KICK_REQ(PacketSessionRef& session, Protocol::C_PARTY_KICK_REQ& pkt);
	static bool Handle_C_PARTY_DISBAND_REQ(PacketSessionRef& session, Protocol::C_PARTY_DISBAND_REQ& pkt);
	static bool Handle_C_PARTY_STATUS_REQ(PacketSessionRef& session, Protocol::C_PARTY_STATUS_REQ& pkt);
	static bool Handle_C_DUNGEON_ENTER_REQ(PacketSessionRef& session, Protocol::C_DUNGEON_ENTER_REQ& pkt);
	static bool Handle_C_DUNGEON_EXIT_REQ(PacketSessionRef& session, Protocol::C_DUNGEON_EXIT_REQ& pkt);

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