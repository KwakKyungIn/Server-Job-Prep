#include "pch.h"
#include "ClientPacketHandler.h"


// [Definition] 정적 멤버 메모리 할당
PacketHandlerFunc ClientPacketHandler::GPacketHandler[UINT16_MAX];

// [INVALID] 잘못된 패킷 처리
bool ClientPacketHandler::Handle_INVALID(PacketSessionRef& session, BYTE* buffer, int32 len)
{
	PacketHeader* header = reinterpret_cast<PacketHeader*>(buffer);
	// 로그: 잘못된 패킷 수신
	return false;
}

// [LOGIN] 유저 로그인 요청
bool ClientPacketHandler::Handle_C_LOGIN_REQ(PacketSessionRef& session, Protocol::C_LOGIN_REQ& pkt)
{
	// TODO: 1. 유저 검증 로직
	// TODO: 2. DB에 로그인 요청 (S2S 패킷 전송)
	// 예: S2SPacketHandler::MakeSendBuffer(s2sPkt) -> dbSession->Send()

	return true;
}

// [CHAT] 유저 채팅 요청
bool ClientPacketHandler::Handle_C_CHAT_REQ(PacketSessionRef& session, Protocol::C_CHAT_REQ& pkt)
{
	// TODO: 채팅 중계 로직 (Broadcasting)
	// 예: RoomManager->Broadcast(pkt)

	return true;
}