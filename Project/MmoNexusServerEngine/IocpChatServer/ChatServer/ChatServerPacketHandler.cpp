#include "pch.h"
#include "ChatServerPacketHandler.h"

PacketHandlerFunc ChatServerPacketHandler::GPacketHandler[UINT16_MAX];

bool ChatServerPacketHandler::Handle_INVALID(PacketSessionRef& session, BYTE* buffer, int32 len)
{
	return false;
}

// [Game -> Chat] 로그인 요청? 채팅서버는 로그인 처리 안 함.
bool ChatServerPacketHandler::Handle_S2S_REQ_LOGIN(PacketSessionRef& session, Protocol::S2S_REQ_LOGIN& pkt)
{
	return false;
}

// [Game -> Chat] 채팅 방송 요청 (핵심)
bool ChatServerPacketHandler::Handle_S2S_REQ_BROADCAST_CHAT(PacketSessionRef& session, Protocol::S2S_REQ_BROADCAST_CHAT& pkt)
{
	// TODO: Redis Pub/Sub이나 전체 세션에 뿌리기
	// 여기선 그냥 "받았다"고 로그만 찍고 응답.
	std::cout << "📢 [ChatServer] Broadcast: " << pkt.message() << " from " << pkt.name() << std::endl;

	Protocol::S2S_RES_BROADCAST_CHAT resPkt;
	resPkt.set_success(true);

	auto sendBuffer = ChatServerPacketHandler::MakeSendBuffer(resPkt);
	session->Send(sendBuffer);

	return true;
}