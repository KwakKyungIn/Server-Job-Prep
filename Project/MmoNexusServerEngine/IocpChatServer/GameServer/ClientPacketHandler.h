#pragma once
#include "Protocol.pb.h"
#include "Crc32.h" // [GIGACHAD] CRC 모듈 포함

using PacketHandlerFunc = std::function<bool(PacketSessionRef&, BYTE*, int32)>;

class ClientPacketHandler
{
public:
	enum : uint16
	{
		PKT_C_LOGIN_REQ = 1000,
		PKT_S_LOGIN_RES = 1001,
		PKT_C_ENTER_GAME_REQ = 1002,
		PKT_S_ENTER_GAME_RES = 1003,
		PKT_C_MOVE = 1004,
		PKT_S_MOVE = 1005,
		PKT_S_SPAWN = 1006,
		PKT_S_DESPAWN = 1007,
		PKT_C_CHAT_REQ = 1008,
		PKT_S_CHAT_RES = 1009,
		PKT_S_CHAT_NTF = 1010,
		PKT_S_HEART_BEAT_RES = 1011,
		PKT_C_HEART_BEAT_REQ = 1012,
	};

	static void Init()
	{
		for (int32 i = 0; i < UINT16_MAX; i++)
			GPacketHandler[i] = Handle_INVALID;
		GPacketHandler[PKT_C_LOGIN_REQ] = [](PacketSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::C_LOGIN_REQ>(Handle_C_LOGIN_REQ, session, buffer, len); };
		GPacketHandler[PKT_C_ENTER_GAME_REQ] = [](PacketSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::C_ENTER_GAME_REQ>(Handle_C_ENTER_GAME_REQ, session, buffer, len); };
		GPacketHandler[PKT_C_MOVE] = [](PacketSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::C_MOVE>(Handle_C_MOVE, session, buffer, len); };
		GPacketHandler[PKT_C_CHAT_REQ] = [](PacketSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::C_CHAT_REQ>(Handle_C_CHAT_REQ, session, buffer, len); };
		GPacketHandler[PKT_C_HEART_BEAT_REQ] = [](PacketSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::C_HEART_BEAT_REQ>(Handle_C_HEART_BEAT_REQ, session, buffer, len); };
	}

	static bool HandlePacket(PacketSessionRef& session, BYTE* buffer, int32 len)
	{
		PacketHeader* header = reinterpret_cast<PacketHeader*>(buffer);
		return GPacketHandler[header->id](session, buffer, len);
	}
	static SendBufferRef MakeSendBuffer(Protocol::S_LOGIN_RES& pkt) { return MakeSendBuffer(pkt, PKT_S_LOGIN_RES); }
	static SendBufferRef MakeSendBuffer(Protocol::S_ENTER_GAME_RES& pkt) { return MakeSendBuffer(pkt, PKT_S_ENTER_GAME_RES); }
	static SendBufferRef MakeSendBuffer(Protocol::S_MOVE& pkt) { return MakeSendBuffer(pkt, PKT_S_MOVE); }
	static SendBufferRef MakeSendBuffer(Protocol::S_SPAWN& pkt) { return MakeSendBuffer(pkt, PKT_S_SPAWN); }
	static SendBufferRef MakeSendBuffer(Protocol::S_DESPAWN& pkt) { return MakeSendBuffer(pkt, PKT_S_DESPAWN); }
	static SendBufferRef MakeSendBuffer(Protocol::S_CHAT_RES& pkt) { return MakeSendBuffer(pkt, PKT_S_CHAT_RES); }
	static SendBufferRef MakeSendBuffer(Protocol::S_CHAT_NTF& pkt) { return MakeSendBuffer(pkt, PKT_S_CHAT_NTF); }
	static SendBufferRef MakeSendBuffer(Protocol::S_HEART_BEAT_RES& pkt) { return MakeSendBuffer(pkt, PKT_S_HEART_BEAT_RES); }

public:
	static PacketHandlerFunc GPacketHandler[UINT16_MAX];
	static bool Handle_INVALID(PacketSessionRef& session, BYTE* buffer, int32 len);
	static bool Handle_C_LOGIN_REQ(PacketSessionRef& session, Protocol::C_LOGIN_REQ& pkt);
	static bool Handle_C_ENTER_GAME_REQ(PacketSessionRef& session, Protocol::C_ENTER_GAME_REQ& pkt);
	static bool Handle_C_MOVE(PacketSessionRef& session, Protocol::C_MOVE& pkt);
	static bool Handle_C_CHAT_REQ(PacketSessionRef& session, Protocol::C_CHAT_REQ& pkt);
	static bool Handle_C_HEART_BEAT_REQ(PacketSessionRef& session, Protocol::C_HEART_BEAT_REQ& pkt);

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