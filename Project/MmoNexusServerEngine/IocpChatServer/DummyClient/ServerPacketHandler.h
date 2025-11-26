#pragma once
#include "Protocol.pb.h"

using PacketHandlerFunc = std::function<bool(PacketSessionRef&, BYTE*, int32)>;

class ServerPacketHandler
{
public:
	// [Enum] 패킷 ID (클래스 소속)
	enum : uint16
	{
		PKT_C_LOGIN_REQ = 1000,
		PKT_S_LOGIN_RES = 1001,
		PKT_C_CHAT_REQ = 1002,
		PKT_S_CHAT_RES = 1003,
		PKT_S_CHAT_NTF = 1004,
	};

	// [Init] 핸들러 등록
	static void Init()
	{
		for (int32 i = 0; i < UINT16_MAX; i++)
			GPacketHandler[i] = Handle_INVALID;
		// 이제 Handle_... 함수는 이 클래스의 멤버 함수니까 바로 찾을 수 있다.
		GPacketHandler[PKT_S_LOGIN_RES] = [](PacketSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::S_LOGIN_RES>(Handle_S_LOGIN_RES, session, buffer, len); };
		// 이제 Handle_... 함수는 이 클래스의 멤버 함수니까 바로 찾을 수 있다.
		GPacketHandler[PKT_S_CHAT_RES] = [](PacketSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::S_CHAT_RES>(Handle_S_CHAT_RES, session, buffer, len); };
		// 이제 Handle_... 함수는 이 클래스의 멤버 함수니까 바로 찾을 수 있다.
		GPacketHandler[PKT_S_CHAT_NTF] = [](PacketSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::S_CHAT_NTF>(Handle_S_CHAT_NTF, session, buffer, len); };
	}

	static bool HandlePacket(PacketSessionRef& session, BYTE* buffer, int32 len)
	{
		PacketHeader* header = reinterpret_cast<PacketHeader*>(buffer);
		return GPacketHandler[header->id](session, buffer, len);
	}
	static SendBufferRef MakeSendBuffer(Protocol::C_LOGIN_REQ& pkt) { return MakeSendBuffer(pkt, PKT_C_LOGIN_REQ); }
	static SendBufferRef MakeSendBuffer(Protocol::C_CHAT_REQ& pkt) { return MakeSendBuffer(pkt, PKT_C_CHAT_REQ); }

public:
	// [Variable] 핸들러 저장소
	static PacketHandlerFunc GPacketHandler[UINT16_MAX];

	// [Function] 핸들러 함수 선언 (모두 static 멤버로 전환)
	static bool Handle_INVALID(PacketSessionRef& session, BYTE* buffer, int32 len);
	static bool Handle_S_LOGIN_RES(PacketSessionRef& session, Protocol::S_LOGIN_RES& pkt);
	static bool Handle_S_CHAT_RES(PacketSessionRef& session, Protocol::S_CHAT_RES& pkt);
	static bool Handle_S_CHAT_NTF(PacketSessionRef& session, Protocol::S_CHAT_NTF& pkt);

private:
	template<typename PacketType, typename ProcessFunc>
	static bool HandlePacket(ProcessFunc func, PacketSessionRef& session, BYTE* buffer, int32 len)
	{
		PacketType pkt;
		if (pkt.ParseFromArray(buffer + sizeof(PacketHeader), len - sizeof(PacketHeader)) == false)
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
		ASSERT_CRASH(pkt.SerializeToArray(&header[1], dataSize));
		sendBuffer->Close(packetSize);

		return sendBuffer;
	}
};