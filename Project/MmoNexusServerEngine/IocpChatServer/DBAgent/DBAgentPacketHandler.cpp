#include "pch.h"
#include "DBAgentPacketHandler.h"
#include "DBConnectionPool.h"
#include "GameSession.h" // [필수] GameSession 클래스를 알기 위해 추가
#include "Job.h"         // [필수] Job을 생성하기 위해 추가

PacketHandlerFunc DBAgentPacketHandler::GPacketHandler[UINT16_MAX];

bool DBAgentPacketHandler::Handle_INVALID(PacketSessionRef& session, BYTE* buffer, int32 len)
{
	return false;
}

// [Game -> DB] 로그인 요청 처리
bool DBAgentPacketHandler::Handle_S2S_REQ_LOGIN(PacketSessionRef& session, Protocol::S2S_REQ_LOGIN& pkt)
{
	// [FIX] GameSessionRef가 없으면 그냥 shared_ptr<GameSession> 쓰면 된다.
	// 부모(PacketSession)를 자식(GameSession)으로 변환해야 PushJob을 쓸 수 있다.
	shared_ptr<GameSession> gameSession = static_pointer_cast<GameSession>(session);

	// [JOB WRAPPING] 
	// 기존 로직을 그대로 람다([]) 안으로 옮긴다.
	// 이제 이 코드는 네트워크 스레드가 아니라, 로직 스레드에서 실행된다.
	gameSession->PushJob(ObjectPool<Job>::MakeShared([gameSession, pkt]()
		{
			// ==========================================================
			//  여기서부터 기존 코드 복사 붙여넣기 (Logic Thread 실행)
			// ==========================================================

			// 1. DB 연결 대여 (Connection Pool)
			DBConnection* conn = GDBConnectionPool->Pop();
			if (conn == nullptr)
			{
				// 연결 풀 고갈 -> 에러 처리 혹은 재시도 로직 필요
				return; // 람다 내부라 return false가 아니라 그냥 return
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
				if (conn->Prepare(L"SELECT id FROM Player WHERE name = ?"))
				{
					// [Step 5] 파라미터 바인딩
					conn->BindParam(1, SQL_C_WCHAR, SQL_WVARCHAR, (wbName.size() + 1) * sizeof(WCHAR), (SQLPOINTER)wbName.c_str(), &nameLen);

					// [Step 6] 결과 컬럼 바인딩
					conn->BindCol(1, SQL_C_SLONG, sizeof(int32), &outId, &outIdLen);

					// [Step 7] 실행 (Execute) - 여기가 제일 느림 (Blocking)
					if (conn->Execute())
					{
						// [Step 8] 결과 인출 (Fetch)
						if (conn->Fetch())
						{
							success = true;
							playerId = outId;
							std::cout << "✅ [DB] Login Success! Name: " << pkt.name() << " ID: " << playerId << std::endl;
						}
						else
						{
							std::cout << "⚠️ [DB] User Not Found: " << pkt.name() << std::endl;
							success = false;
							// TODO: CreateAccount
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
			resPkt.set_playersessionid(pkt.playersessionid()); // 왕복 티켓

			auto sendBuffer = DBAgentPacketHandler::MakeSendBuffer(resPkt);
			gameSession->Send(sendBuffer);

			// ==========================================================
			//  기존 코드 끝
			// ==========================================================
		}));

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