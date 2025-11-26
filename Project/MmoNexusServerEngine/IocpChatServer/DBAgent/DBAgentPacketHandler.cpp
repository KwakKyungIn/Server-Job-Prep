#include "pch.h"
#include "DBAgentPacketHandler.h"
#include "DBConnectionPool.h"

PacketHandlerFunc DBAgentPacketHandler::GPacketHandler[UINT16_MAX];

bool DBAgentPacketHandler::Handle_INVALID(PacketSessionRef& session, BYTE* buffer, int32 len)
{
	return false;
}

// [Game -> DB] 로그인 요청 처리 (SQL 실행)
bool DBAgentPacketHandler::Handle_S2S_REQ_LOGIN(PacketSessionRef& session, Protocol::S2S_REQ_LOGIN& pkt)
{
	// 1. DB 연결
	DBConnection* conn = GDBConnectionPool->Pop();
	if (conn == nullptr) return false;

	uint64 playerId = 0;
	bool success = false;

	// 2. SQL 실행 (간소화 버전)
	// 실제론 Prepare -> Bind -> Execute
	{
		// TODO: SELECT id FROM Player WHERE name = ?
		// (아까 작성한 ODBC 코드 복붙하면 됨)
		// 테스트용으로 무조건 성공 처리:
		success = true;
		playerId = 100; // 임시 ID
	}

	GDBConnectionPool->Push(conn);

	// 3. 응답 전송
	Protocol::S2S_RES_LOGIN resPkt;
	resPkt.set_success(success);
	resPkt.set_playerid(playerId);

	auto sendBuffer = DBAgentPacketHandler::MakeSendBuffer(resPkt);
	session->Send(sendBuffer);

	return true;
}

// [Game -> DB] 채팅 중계 요청? DB는 중계 안 함. (로그 저장 요청이면 모를까)
bool DBAgentPacketHandler::Handle_S2S_REQ_BROADCAST_CHAT(PacketSessionRef& session, Protocol::S2S_REQ_BROADCAST_CHAT& pkt)
{
	// DB는 채팅을 뿌리는 곳이 아님. 그냥 무시하거나 로그 저장용으로 사용.
	return true;
}