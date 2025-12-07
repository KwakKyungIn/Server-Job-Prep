#include "pch.h"
#include "S2SPacketHandler.h"
#include "ClientPacketHandler.h" 
#include "GameSessionManager.h" 
#include "PlayerSession.h"
#include "Job.h" // [NEW]

PacketHandlerFunc S2SPacketHandler::GPacketHandler[UINT16_MAX];

bool S2SPacketHandler::Handle_INVALID(PacketSessionRef& session, BYTE* buffer, int32 len)
{
	return false;
}

// [DB -> Game] 로그인 결과 도착
bool S2SPacketHandler::Handle_S2S_RES_LOGIN(PacketSessionRef& session, Protocol::S2S_RES_LOGIN& pkt)
{
	// 1. 왕복 티켓 확인
	uint64 userSessionId = pkt.playersessionid();

	// 2. 유저 찾기 (ReadLock 걸고 찾음 - 매니저 내부구현에 따라 다름)
	// 찾은 후에는 Reference Count가 올라가서 삭제되지 않음.
	auto playerSession = static_pointer_cast<PlayerSession>(GameSessionManager::GSessionManager->Find(userSessionId));

	if (playerSession == nullptr)
	{
		std::cout << "💀 [FAIL] Session Not Found! ID: " << userSessionId << std::endl;
		return true;
	}

	// 3. Job 생성 (해당 유저의 잡큐에 넣는다)
	playerSession->PushJob(ObjectPool<Job>::MakeShared([playerSession, pkt]()
		{
			// --- [Logic Thread Area] ---

			// [상태 변경] 이제 여기서 플레이어 변수를 마음껏 수정해도 된다.
			// 예: playerSession->SetPlayerId(pkt.playerid()); 
			// 예: playerSession->SetStat(...);

			// [응답 전송]
			Protocol::S_LOGIN_RES resPkt;
			resPkt.set_success(pkt.success());
			resPkt.set_playerid(pkt.playerid());

			auto sendBuffer = ClientPacketHandler::MakeSendBuffer(resPkt);
			playerSession->Send(sendBuffer);

			// [Log]
			if (pkt.success())
				std::cout << "✅ [Login Success] SessionID: " << playerSession->GetSessionId() << " -> PlayerID: " << pkt.playerid() << std::endl;
			else
				std::cout << "❌ [Login Failed] SessionID: " << playerSession->GetSessionId() << std::endl;
		}));

	return true;
}

// ... (나머지 핸들러도 동일한 패턴으로 Job핑하면 됨)
bool S2SPacketHandler::Handle_S2S_RES_BROADCAST_CHAT(PacketSessionRef& session, Protocol::S2S_RES_BROADCAST_CHAT& pkt)
{
	return true;
}

bool S2SPacketHandler::Handle_S2S_RES_HEART_BEAT(PacketSessionRef& session, Protocol::S2S_RES_HEART_BEAT& pkt)
{
	return true;
}