#include "pch.h"
#include "DBSession.h"
#include "S2SPacketHandler.h"

// DB 세션은 전역으로 관리해서 어디서든 접근하기 쉽게 함
shared_ptr<PacketSession> G_DBSession = nullptr;

void DBSession::OnConnected()
{
	G_DBSession = static_pointer_cast<PacketSession>(shared_from_this());
	std::cout << " [GameServer] Connected To DBAgent!" << std::endl;

	// DB 에이전트랑 연결 성공하면 바로 기획 데이터(아이템, 스탯 등) 달라고 요청 보냄
	Protocol::S2S_REQ_LOAD_GAME_DATA pkt;
	auto sendBuffer = S2SPacketHandler::MakeSendBuffer(pkt);
	Send(sendBuffer);

	// 아이템 고유 ID 발급을 위한 시드값 요청
	Protocol::S2S_REQ_GAME_ITEM_UID_SEED req;
	Send(S2SPacketHandler::MakeSendBuffer(req));

	std::cout << " [GameServer] Request Loading Game Data..." << std::endl;
}

void DBSession::OnDisconnected()
{
	// 연결 끊기면 전역 포인터도 날려줌
	if (G_DBSession == shared_from_this())
		G_DBSession = nullptr;
	std::cout << " [GameServer] Disconnected From DBAgent" << std::endl;
}

void DBSession::OnRecvPacket(BYTE* buffer, int32 len)
{
	// 패킷 들어오면 핸들러한테 토스
	PacketSessionRef session = GetPacketSessionRef();
	S2SPacketHandler::HandlePacket(session, buffer, len);
}

void DBSession::OnSend(int32 len)
{
	// 보낼 때 별도 처리 없음
}

void DBSession::Ping()
{
	// 연결 살아있는지 확인하려고 주기적으로 하트비트 패킷 쏨
	std::cout << "GAME -> DB" << std::endl;
	Protocol::S2S_REQ_HEART_BEAT pkt;
	auto sendBuffer = S2SPacketHandler::MakeSendBuffer(pkt);
	Send(sendBuffer);
}