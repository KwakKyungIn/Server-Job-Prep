#include "pch.h"
#include "ClientPacketHandler.h"
#include "ClientSession.h"
#include "S2SPacketHandler.h"
#include "DBAgentSession.h"

// [GIGACHAD] 전역 변수 참조
extern shared_ptr<DBAgentSession> GDBAgentSession;

PacketHandlerFunc ClientPacketHandler::GPacketHandler[UINT16_MAX];

bool ClientPacketHandler::Handle_INVALID(PacketSessionRef& session, BYTE* buffer, int32 len)
{
	return false;
}

// [REQ] 클라이언트 -> 로그인 서버 : "로그인 시켜줘 (ID/PW)"
bool ClientPacketHandler::Handle_C_LOGIN(PacketSessionRef& session, Protocol::C_LOGIN& pkt)
{
	// 1. 세션 변환
	// LoginServer는 가벼운 ClientSession 사용
	ClientSessionRef clientSession = static_pointer_cast<ClientSession>(session);

	// 2. 패킷 검증
	if (pkt.userid().empty())
		return false;

	// 3. DBAgent 연결 확인
	if (GDBAgentSession == nullptr)
	{
		printf("❌ [Login] DB Connection Lost!\n");
		return false;
	}

	// 4. DB 검증 요청 (비동기)
	Protocol::S2S_REQ_LOGIN reqPkt;
	reqPkt.set_playersessionid(clientSession->GetSessionId()); // 왕복 티켓
	reqPkt.set_name(pkt.userid());
	// reqPkt.set_password(pkt.password());

	auto sendBuffer = S2SPacketHandler::MakeSendBuffer(reqPkt);
	GDBAgentSession->Send(sendBuffer);

	printf("🚀 [Login] Request DB Verification for: %s\n", pkt.userid().c_str());

	return true;
}

// LoginServer는 아래 패킷들을 처리하지 않음 (GameServer용)
bool ClientPacketHandler::Handle_C_ENTER_GAME(PacketSessionRef& session, Protocol::C_ENTER_GAME& pkt) { return false; }
bool ClientPacketHandler::Handle_C_MOVE(PacketSessionRef& session, Protocol::C_MOVE& pkt) { return false; }
bool ClientPacketHandler::Handle_C_SKILL(PacketSessionRef& session, Protocol::C_SKILL& pkt) { return false; }
bool ClientPacketHandler::Handle_C_USE_ITEM(PacketSessionRef& session, Protocol::C_USE_ITEM& pkt) { return false; }
bool ClientPacketHandler::Handle_C_EQUIP_ITEM(PacketSessionRef& session, Protocol::C_EQUIP_ITEM& pkt) { return false; }
bool ClientPacketHandler::Handle_C_CHAT_REQ(PacketSessionRef& session, Protocol::C_CHAT_REQ& pkt) { return false; }
bool ClientPacketHandler::Handle_C_HEART_BEAT_REQ(PacketSessionRef& session, Protocol::C_HEART_BEAT_REQ& pkt) { return true; }