#include "pch.h"
#include "S2SPacketHandler.h"
#include "ClientPacketHandler.h" // 클라에게 보낼 패킷 생성용
#include "GameSessionManager.h"  // 유저 찾기용
#include "PlayerSession.h"

PacketHandlerFunc S2SPacketHandler::GPacketHandler[UINT16_MAX];

bool S2SPacketHandler::Handle_INVALID(PacketSessionRef& session, BYTE* buffer, int32 len)
{
	return false;
}

// [DB -> Game] 로그인 결과 도착 (여기서 클라에게 최종 응답)
bool S2SPacketHandler::Handle_S2S_RES_LOGIN(PacketSessionRef& session, Protocol::S2S_RES_LOGIN& pkt)
{
	// 1. 왕복 티켓 확인 (Context Recovery)
	uint64 userSessionId = pkt.playersessionid();

	// 2. 대기 중이던 유저 찾기 (O(logN))
	auto playerSession = GameSessionManager::GSessionManager->Find(userSessionId);
	if (playerSession == nullptr)
	{
		// 유저가 로그인 요청 후 못 참고 나감 (종종 발생)
		std::cout << "💀 [FAIL] Session Not Found! ID: " << userSessionId << std::endl;

		return true;
	}

	// 3. 클라에게 보낼 패킷 구성
	Protocol::S_LOGIN_RES resPkt;
	resPkt.set_success(pkt.success());
	resPkt.set_playerid(pkt.playerid()); // DB에서 발급받은 UID (캐릭터 ID)

	// 4. 유저에게 최종 전송
	auto sendBuffer = ClientPacketHandler::MakeSendBuffer(resPkt);
	playerSession->Send(sendBuffer);

	// [Log] 성공 여부 출력
	if (pkt.success())
		std::cout << "[Login Success] SessionID: " << userSessionId << " -> PlayerID: " << pkt.playerid() << std::endl;
	else
		std::cout << "[Login Failed] SessionID: " << userSessionId << std::endl;

	return true;
}

// [Chat -> Game] 채팅 전송 결과
bool S2SPacketHandler::Handle_S2S_RES_BROADCAST_CHAT(PacketSessionRef& session, Protocol::S2S_RES_BROADCAST_CHAT& pkt)
{
	return true;
}

bool S2SPacketHandler::Handle_S2S_RES_HEART_BEAT(PacketSessionRef& session, Protocol::S2S_RES_HEART_BEAT& pkt)
{
	return true;
}