#include "pch.h"
#include "S2SPacketHandler.h"

// [Definition] 정적 멤버 메모리 할당 (이거 빼먹으면 링크 에러 남)
PacketHandlerFunc S2SPacketHandler::GPacketHandler[UINT16_MAX];

// [INVALID]
bool S2SPacketHandler::Handle_INVALID(PacketSessionRef& session, BYTE* buffer, int32 len)
{
	return false;
}

// [LOGIN RES] DB -> Chat
// 채팅 서버가 DB에 로그인 요청할 일은 없지만, 핸들러 인터페이스상 존재함.
bool S2SPacketHandler::Handle_S2S_RES_LOGIN(PacketSessionRef& session, Protocol::S2S_RES_LOGIN& pkt)
{
	return true;
}

// [BROADCAST RES] Chat -> Game -> Chat? 
// 이건 ChatServer가 GameServer한테 보낸 요청의 응답을 받는 경우.
bool S2SPacketHandler::Handle_S2S_RES_BROADCAST_CHAT(PacketSessionRef& session, Protocol::S2S_RES_BROADCAST_CHAT& pkt)
{
	// 만약 ChatServer가 다른 서버로 중계를 요청했다면 여기서 결과를 받음.
	return true;
}

// [추가 필요] 만약 채팅 로그 저장 기능(S2S_REQ_CHAT_LOG)이 생기면 
// 여기에 Handle_S2S_RES_CHAT_LOG 가 추가될 것임.