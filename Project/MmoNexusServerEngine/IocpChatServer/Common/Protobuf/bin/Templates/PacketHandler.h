#pragma once
#include "{{proto_file}}.pb.h"

using PacketHandlerFunc = std::function<bool(PacketSessionRef&, BYTE*, int32)>;

class {{output}}
{
public:
	// [Enum] 패킷 ID (클래스 소속)
	enum : uint16
	{
	{%- for pkt in parser.total_pkt %}
		PKT_{{pkt.name}} = {{pkt.id}},
	{%- endfor %}
	};

	// [Init] 핸들러 등록
	static void Init()
	{
		for (int32 i = 0; i < UINT16_MAX; i++)
			GPacketHandler[i] = Handle_INVALID;

{%- for pkt in parser.recv_pkt %}
		// 이제 Handle_... 함수는 이 클래스의 멤버 함수니까 바로 찾을 수 있다.
		GPacketHandler[PKT_{{pkt.name}}] = [](PacketSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::{{pkt.name}}>(Handle_{{pkt.name}}, session, buffer, len); };
{%- endfor %}
	}

	static bool HandlePacket(PacketSessionRef& session, BYTE* buffer, int32 len)
	{
		PacketHeader* header = reinterpret_cast<PacketHeader*>(buffer);
		return GPacketHandler[header->id](session, buffer, len);
	}

{%- for pkt in parser.send_pkt %}
	static SendBufferRef MakeSendBuffer(Protocol::{{pkt.name}}& pkt) { return MakeSendBuffer(pkt, PKT_{{pkt.name}}); }
{%- endfor %}

public:
	// [Variable] 핸들러 저장소
	static PacketHandlerFunc GPacketHandler[UINT16_MAX];

	// [Function] 핸들러 함수 선언 (모두 static 멤버로 전환)
	static bool Handle_INVALID(PacketSessionRef& session, BYTE* buffer, int32 len);

	{%- for pkt in parser.recv_pkt %}
	static bool Handle_{{pkt.name}}(PacketSessionRef& session, Protocol::{{pkt.name}}& pkt);
	{%- endfor %}

private:
	template<typename PacketType, typename ProcessFunc>
	static bool HandlePacket(ProcessFunc func, PacketSessionRef& session, BYTE* buffer, int32 len)
	{
		// [GIGACHAD] 1. 복호화 (Body Only)
		// buffer는 현재 [Header][Encrypted Body]
		// 헤더는 이미 읽었으니 건드리지 말고, 뒷부분만 복호화
		int32 dataSize = len - sizeof(PacketHeader);
		
		// 내부 static 함수 호출
		XorCrypt(buffer + sizeof(PacketHeader), dataSize);

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
		
		// [GIGACHAD] 1. 직렬화
		ASSERT_CRASH(pkt.SerializeToArray(&header[1], dataSize));

		// [GIGACHAD] 2. 암호화 (Body Only)
		// &header[1] 부터 dataSize 만큼 암호화
		XorCrypt(reinterpret_cast<BYTE*>(&header[1]), dataSize);

		sendBuffer->Close(packetSize);

		return sendBuffer;
	}

	// [Encryption Logic] 내장형 XOR
	static void XorCrypt(BYTE* buffer, int32 len)
	{
		// 단순 XOR 키 (Server-Client 동일해야 함)
		const BYTE xorKey = 0x5A; 
		
		// 루프 돌면서 비트 반전
		for (int32 i = 0; i < len; i++)
		{
			buffer[i] ^= xorKey;
		}
	}
};