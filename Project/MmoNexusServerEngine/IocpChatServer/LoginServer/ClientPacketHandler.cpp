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

	// 2-1. 로그인 이름 저장 (DB 응답에서 토큰 캐시에 쓰기 위함)
	clientSession->SetLoginName(pkt.userid());

	// 3. DBAgent 연결 확인
	if (GDBAgentSession == nullptr)
	{
		printf(" [Login] DB Connection Lost!\n");
		return false;
	}

	// 4. DB 검증 요청 (비동기)
	Protocol::S2S_REQ_LOGIN reqPkt;
	reqPkt.set_playersessionid(clientSession->GetSessionId()); // 왕복 티켓
	reqPkt.set_name(pkt.userid());
	// reqPkt.set_password(pkt.password());

	auto sendBuffer = S2SPacketHandler::MakeSendBuffer(reqPkt);
	GDBAgentSession->Send(sendBuffer);

	printf(" [Login] Request DB Verification for: %s\n", pkt.userid().c_str());

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
bool ClientPacketHandler::Handle_C_MAP_CHANGE_REQ(PacketSessionRef& session, Protocol::C_MAP_CHANGE_REQ& pkt) { return false; }
bool ClientPacketHandler::Handle_C_MAP_CHANGE_ACK(PacketSessionRef& session, Protocol::C_MAP_CHANGE_ACK& pkt) { return false; }
bool ClientPacketHandler::Handle_C_PARTY_CHAT_REQ(PacketSessionRef& session, Protocol::C_PARTY_CHAT_REQ& pkt) { return false; }
bool ClientPacketHandler::Handle_C_PARTY_CREATE_REQ(PacketSessionRef& session, Protocol::C_PARTY_CREATE_REQ& pkt) { return false; }
bool ClientPacketHandler::Handle_C_PARTY_INVITE_REQ(PacketSessionRef& session, Protocol::C_PARTY_INVITE_REQ& pkt) { return false; }
bool ClientPacketHandler::Handle_C_PARTY_INVITE_ACCEPT_REQ(PacketSessionRef& session, Protocol::C_PARTY_INVITE_ACCEPT_REQ& pkt) { return false; }
bool ClientPacketHandler::Handle_C_PARTY_LEAVE_REQ(PacketSessionRef& session, Protocol::C_PARTY_LEAVE_REQ& pkt) { return false; }
bool ClientPacketHandler::Handle_C_PARTY_KICK_REQ(PacketSessionRef& session, Protocol::C_PARTY_KICK_REQ& pkt) { return false; }
bool ClientPacketHandler::Handle_C_PARTY_DISBAND_REQ(PacketSessionRef& session, Protocol::C_PARTY_DISBAND_REQ& pkt) { return false; }
bool ClientPacketHandler::Handle_C_PARTY_STATUS_REQ(PacketSessionRef& session, Protocol::C_PARTY_STATUS_REQ& pkt) { return false; }
bool ClientPacketHandler:: Handle_C_DUNGEON_ENTER_REQ(PacketSessionRef& session, Protocol::C_DUNGEON_ENTER_REQ& pkt) { return false; }
bool ClientPacketHandler:: Handle_C_DUNGEON_EXIT_REQ(PacketSessionRef& session, Protocol::C_DUNGEON_EXIT_REQ& pkt) { return false; }
bool ClientPacketHandler::Handle_C_SET_QUICKSLOT(PacketSessionRef& session, Protocol::C_SET_QUICKSLOT& pkt) { return false; }
bool ClientPacketHandler::Handle_C_TRADE_REQ(PacketSessionRef& session, Protocol::C_TRADE_REQ& pkt) { return false; }
bool ClientPacketHandler::Handle_C_TRADE_INVITE_RESP(PacketSessionRef& session, Protocol::C_TRADE_INVITE_RESP& pkt) { return false; }
bool ClientPacketHandler::Handle_C_TRADE_OFFER_SET(PacketSessionRef& session, Protocol::C_TRADE_OFFER_SET& pkt) { return false; }
bool ClientPacketHandler::Handle_C_TRADE_READY(PacketSessionRef& session, Protocol::C_TRADE_READY& pkt) { return false; }
bool ClientPacketHandler::Handle_C_TRADE_CONFIRM(PacketSessionRef& session, Protocol::C_TRADE_CONFIRM& pkt) { return false; }
bool ClientPacketHandler::Handle_C_TRADE_CANCEL(PacketSessionRef& session, Protocol::C_TRADE_CANCEL& pkt) { return false; }
bool ClientPacketHandler::Handle_C_INV_DRAG_DROP(PacketSessionRef& session, Protocol::C_INV_DRAG_DROP& pkt) { return false; }
bool ClientPacketHandler:: Handle_C_CHANNEL_CHANGE_REQ(PacketSessionRef& session, Protocol::C_CHANNEL_CHANGE_REQ& pkt) { return false; }

