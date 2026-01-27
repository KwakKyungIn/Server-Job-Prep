#include "pch.h"
#include "GameServerSession.h"
#include "S2SPacketHandler.h"

namespace
{
	static void XorCrypt(BYTE* buffer, int32 len)
	{
		const BYTE xorKey = 0x5A;
		for (int32 i = 0; i < len; i++)
			buffer[i] ^= xorKey;
	}

	static SendBufferRef MakeS2SResHeartbeat()
	{
		Protocol::S2S_RES_HEART_BEAT pkt;

		const uint16 dataSize = static_cast<uint16>(pkt.ByteSizeLong());
		const uint16 packetSize = dataSize + sizeof(PacketHeader);

		SendBufferRef sendBuffer = GSendBufferManager->Open(packetSize);
		PacketHeader* header = reinterpret_cast<PacketHeader*>(sendBuffer->Buffer());
		header->size = packetSize;
		header->id = S2SPacketHandler::PKT_S2S_RES_HEART_BEAT;
		header->seq = 0;
		header->crc = 0;

		ASSERT_CRASH(pkt.SerializeToArray(&header[1], dataSize));
		XorCrypt(reinterpret_cast<BYTE*>(&header[1]), dataSize);

		sendBuffer->Close(packetSize);
		return sendBuffer;
	}
}

void GameServerSession::OnConnected()
{
	std::cout << " [LoginServer] GameServer Connected!" << std::endl;
}

void GameServerSession::OnDisconnected()
{
	std::cout << " [LoginServer] GameServer Disconnected" << std::endl;
}

void GameServerSession::OnRecvPacket(BYTE* buffer, int32 len)
{
	PacketSessionRef session = GetPacketSessionRef();
	// GameServer가 보낸 패킷 처리 (S2S 핸들러)
	// ※ LoginServer S2S 핸들러는 RES 위주로 생성되므로,
	//    REQ_HEART_BEAT는 여기서 직접 처리한다.
	PacketHeader* header = reinterpret_cast<PacketHeader*>(buffer);
	if (header->id == S2SPacketHandler::PKT_S2S_REQ_HEART_BEAT)
	{
		const int32 dataSize = len - sizeof(PacketHeader);
		const uint32 calcCrc = Crc32::Compute(buffer + sizeof(PacketHeader), dataSize);
		if (header->crc != calcCrc)
			return;
		if (CheckRecvSeq(header->seq) == false)
			return;

		Send(MakeS2SResHeartbeat());
		return;
	}

	S2SPacketHandler::HandlePacket(session, buffer, len);
}

void GameServerSession::OnSend(int32 len)
{
}
