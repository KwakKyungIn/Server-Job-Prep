#include "pch.h"
#include "ClientPacketHandler.h"
#include "S2SPacketHandler.h" // S2S 패킷 생성용
#include "PlayerSession.h"
#include "GameSessionManager.h"

// [Gigachad] Global DB Session Reference
// GameServer.cpp나 DBSession.cpp 어딘가에서 이 포인터를 세팅해줘야 한다.
// (DBSession::OnConnected에서 G_DBSession = ... 해주는 거 잊지 마라)
extern shared_ptr<PacketSession> G_DBSession;

PacketHandlerFunc ClientPacketHandler::GPacketHandler[UINT16_MAX];

bool ClientPacketHandler::Handle_INVALID(PacketSessionRef& session, BYTE* buffer, int32 len)
{
	// [Security] 비정상 패킷 로깅
	// std::cout << "[Warning] Invalid Packet Received" << std::endl;
	return false;
}

// [LOGIN] 클라 -> 게임 -> DB
bool ClientPacketHandler::Handle_C_LOGIN_REQ(PacketSessionRef& session, Protocol::C_LOGIN_REQ& pkt)
{
	// 1. 유저 ID 확보 (Base Session ID 사용)
	// 이 ID가 나중에 DB 돌고 왔을 때 유저를 찾는 열쇠(Key)가 된다.
	uint64 mySessionId = session->GetSessionId();

	// 2. S2S 패킷 생성 (Context Packing)
	Protocol::S2S_REQ_LOGIN s2sPkt;
	s2sPkt.set_playersessionid(mySessionId); // [KEY POINT] 왕복 티켓
	s2sPkt.set_name(pkt.name());

	// 3. DB Agent로 전송
	if (G_DBSession == nullptr || G_DBSession->IsConnected() == false)
	{
		// DB 연결 끊김 -> 유저에게 에러 처리 필요하지만, 지금은 리턴
		// std::cout << "[Error] DB Agent Disconnected!" << std::endl;
		return false;
	}

	auto sendBuffer = S2SPacketHandler::MakeSendBuffer(s2sPkt);
	G_DBSession->Send(sendBuffer);

	return true;
}

// [CHAT] 클라 -> 게임 -> (채팅서버)
bool ClientPacketHandler::Handle_C_CHAT_REQ(PacketSessionRef& session, Protocol::C_CHAT_REQ& pkt)
{
	// TODO: ChatServer 연결 후 S2S_REQ_BROADCAST_CHAT 전송
	return true;
}

bool ClientPacketHandler::Handle_C_HEART_BEAT_REQ(PacketSessionRef& session, Protocol::C_HEART_BEAT_REQ& pkt)
{
	// Pong은 서버 부하에 따라 생략하거나, 특정 주기로만 응답
	return true;
}