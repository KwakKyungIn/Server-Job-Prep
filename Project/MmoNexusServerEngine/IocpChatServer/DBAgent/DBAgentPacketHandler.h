#pragma once
#include "Protocol_S2S.pb.h"
#include "Crc32.h" // [GIGACHAD] CRC 모듈 포함

using PacketHandlerFunc = std::function<bool(PacketSessionRef&, BYTE*, int32)>;

class DBAgentPacketHandler
{
public:
	enum : uint16
	{
		PKT_S2S_REQ_LOGIN = 2000,
		PKT_S2S_RES_LOGIN = 2001,
		PKT_S2S_REQ_BROADCAST_CHAT = 2002,
		PKT_S2S_REQ_LOAD_PLAYER_DATA = 2003,
		PKT_S2S_RES_LOAD_PLAYER_DATA = 2004,
		PKT_S2S_REQ_ITEMS_LOAD = 2005,
		PKT_S2S_RES_ITEMS_LOAD = 2006,
		PKT_S2S_REQ_LOAD_GAME_DATA = 2007,
		PKT_S2S_RES_LOAD_GAME_DATA = 2008,
		PKT_S2S_RES_BROADCAST_CHAT = 2009,
		PKT_S2S_RES_HEART_BEAT = 2010,
		PKT_S2S_REQ_HEART_BEAT = 2011,
		PKT_S2S_REQ_PARTY_SYNC = 2012,
		PKT_S2S_RES_PARTY_SYNC = 2013,
		PKT_S2S_REQ_PARTY_CHAT = 2014,
		PKT_S2S_RES_PARTY_CHAT = 2015,
	};

	static void Init()
	{
		for (int32 i = 0; i < UINT16_MAX; i++)
			GPacketHandler[i] = Handle_INVALID;
		GPacketHandler[PKT_S2S_REQ_LOGIN] = [](PacketSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::S2S_REQ_LOGIN>(Handle_S2S_REQ_LOGIN, session, buffer, len); };
		GPacketHandler[PKT_S2S_REQ_BROADCAST_CHAT] = [](PacketSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::S2S_REQ_BROADCAST_CHAT>(Handle_S2S_REQ_BROADCAST_CHAT, session, buffer, len); };
		GPacketHandler[PKT_S2S_REQ_LOAD_PLAYER_DATA] = [](PacketSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::S2S_REQ_LOAD_PLAYER_DATA>(Handle_S2S_REQ_LOAD_PLAYER_DATA, session, buffer, len); };
		GPacketHandler[PKT_S2S_REQ_ITEMS_LOAD] = [](PacketSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::S2S_REQ_ITEMS_LOAD>(Handle_S2S_REQ_ITEMS_LOAD, session, buffer, len); };
		GPacketHandler[PKT_S2S_REQ_LOAD_GAME_DATA] = [](PacketSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::S2S_REQ_LOAD_GAME_DATA>(Handle_S2S_REQ_LOAD_GAME_DATA, session, buffer, len); };
		GPacketHandler[PKT_S2S_REQ_HEART_BEAT] = [](PacketSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::S2S_REQ_HEART_BEAT>(Handle_S2S_REQ_HEART_BEAT, session, buffer, len); };
		GPacketHandler[PKT_S2S_REQ_PARTY_SYNC] = [](PacketSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::S2S_REQ_PARTY_SYNC>(Handle_S2S_REQ_PARTY_SYNC, session, buffer, len); };
		GPacketHandler[PKT_S2S_REQ_PARTY_CHAT] = [](PacketSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::S2S_REQ_PARTY_CHAT>(Handle_S2S_REQ_PARTY_CHAT, session, buffer, len); };
	}

	static bool HandlePacket(PacketSessionRef& session, BYTE* buffer, int32 len)
	{
		PacketHeader* header = reinterpret_cast<PacketHeader*>(buffer);
		return GPacketHandler[header->id](session, buffer, len);
	}
	static SendBufferRef MakeSendBuffer(Protocol::S2S_RES_LOGIN& pkt) { return MakeSendBuffer(pkt, PKT_S2S_RES_LOGIN); }
	static SendBufferRef MakeSendBuffer(Protocol::S2S_RES_LOAD_PLAYER_DATA& pkt) { return MakeSendBuffer(pkt, PKT_S2S_RES_LOAD_PLAYER_DATA); }
	static SendBufferRef MakeSendBuffer(Protocol::S2S_RES_ITEMS_LOAD& pkt) { return MakeSendBuffer(pkt, PKT_S2S_RES_ITEMS_LOAD); }
	static SendBufferRef MakeSendBuffer(Protocol::S2S_RES_LOAD_GAME_DATA& pkt) { return MakeSendBuffer(pkt, PKT_S2S_RES_LOAD_GAME_DATA); }
	static SendBufferRef MakeSendBuffer(Protocol::S2S_RES_BROADCAST_CHAT& pkt) { return MakeSendBuffer(pkt, PKT_S2S_RES_BROADCAST_CHAT); }
	static SendBufferRef MakeSendBuffer(Protocol::S2S_RES_HEART_BEAT& pkt) { return MakeSendBuffer(pkt, PKT_S2S_RES_HEART_BEAT); }
	static SendBufferRef MakeSendBuffer(Protocol::S2S_RES_PARTY_SYNC& pkt) { return MakeSendBuffer(pkt, PKT_S2S_RES_PARTY_SYNC); }
	static SendBufferRef MakeSendBuffer(Protocol::S2S_RES_PARTY_CHAT& pkt) { return MakeSendBuffer(pkt, PKT_S2S_RES_PARTY_CHAT); }

public:
	static PacketHandlerFunc GPacketHandler[UINT16_MAX];
	static bool Handle_INVALID(PacketSessionRef& session, BYTE* buffer, int32 len);
	static bool Handle_S2S_REQ_LOGIN(PacketSessionRef& session, Protocol::S2S_REQ_LOGIN& pkt);
	static bool Handle_S2S_REQ_BROADCAST_CHAT(PacketSessionRef& session, Protocol::S2S_REQ_BROADCAST_CHAT& pkt);
	static bool Handle_S2S_REQ_LOAD_PLAYER_DATA(PacketSessionRef& session, Protocol::S2S_REQ_LOAD_PLAYER_DATA& pkt);
	static bool Handle_S2S_REQ_ITEMS_LOAD(PacketSessionRef& session, Protocol::S2S_REQ_ITEMS_LOAD& pkt);
	static bool Handle_S2S_REQ_LOAD_GAME_DATA(PacketSessionRef& session, Protocol::S2S_REQ_LOAD_GAME_DATA& pkt);
	static bool Handle_S2S_REQ_HEART_BEAT(PacketSessionRef& session, Protocol::S2S_REQ_HEART_BEAT& pkt);
	static bool Handle_S2S_REQ_PARTY_SYNC(PacketSessionRef& session, Protocol::S2S_REQ_PARTY_SYNC& pkt);
	static bool Handle_S2S_REQ_PARTY_CHAT(PacketSessionRef& session, Protocol::S2S_REQ_PARTY_CHAT& pkt);

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