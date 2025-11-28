#include "pch.h"
#include "DBAgentPacketHandler.h"
#include "DBConnectionPool.h"

PacketHandlerFunc DBAgentPacketHandler::GPacketHandler[UINT16_MAX];

bool DBAgentPacketHandler::Handle_INVALID(PacketSessionRef& session, BYTE* buffer, int32 len)
{
	return false;
}

// [Game -> DB] 로그인 요청 처리
bool DBAgentPacketHandler::Handle_S2S_REQ_LOGIN(PacketSessionRef& session, Protocol::S2S_REQ_LOGIN& pkt)
{
	// 1. DB 연결 대여 (Connection Pool)
	// GDBConnectionPool은 Main이나 전역 어딘가에 있어야 함
	DBConnection* conn = GDBConnectionPool->Pop();
	if (conn == nullptr)
	{
		// 연결 풀 고갈 -> 에러 처리 혹은 재시도 로직 필요
		return false;
	}

	uint64 playerId = 0;
	bool success = false;

	// 2. SQL 실행 (Scope로 묶어서 깔끔하게 관리)
	{
		// [Step 1] 기존 바인딩 찌꺼기 청소 (필수)
		conn->Unbind();

		// [Step 2] 입력 데이터 준비 (UTF-8 string -> WCHAR 변환)
		std::wstring wbName;
		wbName.assign(pkt.name().begin(), pkt.name().end());

		SQLLEN nameLen = SQL_NTS; // Null Terminated String

		// [Step 3] 출력 데이터 준비
		int32 outId = 0;
		SQLLEN outIdLen = 0;

		// [Step 4] 쿼리 준비 (Prepare)
		// "이름으로 ID를 찾아라"
		if (conn->Prepare(L"SELECT id FROM Player WHERE name = ?"))
		{
			// [Step 5] 파라미터 바인딩 (BindParam)
			// 1번 물음표(?)에 wbName을 연결한다.
			conn->BindParam(1, SQL_C_WCHAR, SQL_WVARCHAR, (wbName.size() + 1) * sizeof(WCHAR), (SQLPOINTER)wbName.c_str(), &nameLen);

			// [Step 6] 결과 컬럼 바인딩 (BindCol)
			// SELECT 결과(id)를 outId 변수에 연결한다.
			conn->BindCol(1, SQL_C_SLONG, sizeof(int32), &outId, &outIdLen);

			// [Step 7] 실행 (Execute)
			if (conn->Execute())
			{
				// [Step 8] 결과 인출 (Fetch)
				if (conn->Fetch())
				{
					// 데이터가 있다 = 계정 존재
					success = true;
					playerId = outId;
					std::cout << "✅ [DB] Login Success! Name: " << pkt.name() << " ID: " << playerId << std::endl;
				}
				else
				{
					// 데이터가 없다 = 계정 없음
					// TODO: 여기서 INSERT 구문을 실행해서 회원가입 시키는 로직 추가 (CreateAccount)
					std::cout << "⚠️ [DB] User Not Found: " << pkt.name() << std::endl;
					success = false;
				}
			}
		}
	}

	// 3. 사용한 DB 연결 반납 (필수)
	GDBConnectionPool->Push(conn);

	// 4. 응답 패킷 전송
	Protocol::S2S_RES_LOGIN resPkt;
	resPkt.set_success(success);
	resPkt.set_playerid(playerId);

	// [Gigachad's Correction] 왕복 티켓 반환
	// 이걸 안 넣으면 GameServer가 미아(Missing Child)가 된다.
	resPkt.set_playersessionid(pkt.playersessionid());

	auto sendBuffer = DBAgentPacketHandler::MakeSendBuffer(resPkt);
	session->Send(sendBuffer);

	return true;
}

// [Game -> DB] 채팅 중계 요청? DB는 중계 안 함.
bool DBAgentPacketHandler::Handle_S2S_REQ_BROADCAST_CHAT(PacketSessionRef& session, Protocol::S2S_REQ_BROADCAST_CHAT& pkt)
{
	// DB는 채팅을 뿌리는 곳이 아님. 로그 저장용이라면 모를까.
	return true;
}

bool DBAgentPacketHandler::Handle_S2S_REQ_HEART_BEAT(PacketSessionRef& session, Protocol::S2S_REQ_HEART_BEAT& pkt)
{
	// std::cout << "DB->-> PONG" << std::endl;
	Protocol::S2S_RES_HEART_BEAT resPkt;
	auto sendBuffer = DBAgentPacketHandler::MakeSendBuffer(resPkt);
	session->Send(sendBuffer);

	return true;
}