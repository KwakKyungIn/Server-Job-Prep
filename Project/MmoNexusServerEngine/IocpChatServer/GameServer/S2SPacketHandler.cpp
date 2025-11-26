#include "pch.h"
#include "S2SPacketHandler.h"
//#include "GameSessionManager.h" // 클라 세션 찾기용

PacketHandlerFunc S2SPacketHandler::GPacketHandler[UINT16_MAX];

bool S2SPacketHandler::Handle_INVALID(PacketSessionRef& session, BYTE* buffer, int32 len)
{
	return false;
}

// [DB -> Game] 로그인 결과 도착
bool S2SPacketHandler::Handle_S2S_RES_LOGIN(PacketSessionRef& session, Protocol::S2S_RES_LOGIN& pkt)
{
	if (pkt.success())
	{
		// TODO: 해당 유저 세션 찾아서 '로그인 성공' 패킷(S_LOGIN_RES) 보내기
		// GameSessionManager::Find(pkt.playerId())->Send(...)
		std::cout << " [DB Success] Login OK! ID: " << pkt.playerid() << std::endl;
	}
	else
	{
		// 실패 처리
		std::cout << " [DB Fail] Login Failed." << std::endl;
	}
	return true;
}

// [Chat -> Game] 채팅 방송 결과 (성공 여부만 옴)
bool S2SPacketHandler::Handle_S2S_RES_BROADCAST_CHAT(PacketSessionRef& session, Protocol::S2S_RES_BROADCAST_CHAT& pkt)
{
	// 굳이 로그 안 남겨도 됨
	return true;
}