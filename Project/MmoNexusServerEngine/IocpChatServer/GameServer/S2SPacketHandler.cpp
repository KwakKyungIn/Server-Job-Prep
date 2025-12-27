#include "pch.h"
#include "S2SPacketHandler.h"
#include "ClientPacketHandler.h"
#include "GameSessionManager.h"
#include "PlayerSession.h"
#include "Job.h"
#include "Player.h"
#include "DataManager.h"
#include "GameRoom.h"
#include "RoomManager.h"
#include "LobbyRoom.h"


extern shared_ptr<RoomManager> GRoomManager;

PacketHandlerFunc S2SPacketHandler::GPacketHandler[UINT16_MAX];

bool S2SPacketHandler::Handle_INVALID(PacketSessionRef& session, BYTE* buffer, int32 len)
{
	return false;
}

// [DB -> Game] 로그인 결과 도착
bool S2SPacketHandler::Handle_S2S_RES_LOGIN(PacketSessionRef& session, Protocol::S2S_RES_LOGIN& pkt)
{
	
	return true;
}

// [DB -> Game] 플레이어 정보(Stat) 로딩 완료 (NEW)
bool S2SPacketHandler::Handle_S2S_RES_LOAD_PLAYER_DATA(PacketSessionRef& session, Protocol::S2S_RES_LOAD_PLAYER_DATA& pkt)
{
	PlayerSessionRef ps = GameSessionManager::GSessionManager->FindBySessionId(pkt.gamesessionid());
	if (!ps) return true;

	if (!pkt.success())
	{
		ps->Post([](PlayerSessionRef self)
			{
				// 필요하면 실패 처리
			});
		return true;
	}

	const uint64 playerId = GameSessionManager::GSessionManager->GetPlayerIdBySessionId(pkt.gamesessionid());
	if (playerId == 0) return true;

	if (GLobbyRoom)
	{
		GLobbyRoom->Push([playerId, pkt]() mutable
			{
				GLobbyRoom->OnStatLoaded(playerId, pkt);
			});
	}
	printf("1번 잘되냐ㅕ\n");
	return true;
}

// [DB -> Game] 아이템 로딩 완료
bool S2SPacketHandler::Handle_S2S_RES_ITEMS_LOAD(PacketSessionRef& session, Protocol::S2S_RES_ITEMS_LOAD& pkt)
{
	PlayerSessionRef ps = GameSessionManager::GSessionManager->FindBySessionId(pkt.gamesessionid());
	if (!ps) return true;

	if (!pkt.success())
	{
		ps->Post([](PlayerSessionRef self)
			{
				// 필요하면 실패 처리
			});
		return true;
	}
	
	const uint64 playerId = GameSessionManager::GSessionManager->GetPlayerIdBySessionId(pkt.gamesessionid());
	if (playerId == 0) return true;

	if (GLobbyRoom)
	{
		GLobbyRoom->Push([playerId, pkt]() mutable
			{
				GLobbyRoom->OnItemsLoaded(playerId, pkt);
			});
	}
	printf("2번 잘되냐ㅕ\n");
	return true;
}

bool S2SPacketHandler::Handle_S2S_RES_LOAD_GAME_DATA(PacketSessionRef& session, Protocol::S2S_RES_LOAD_GAME_DATA& pkt)
{
	if (pkt.success() == false) return false;
	DataManager::Instance()->LoadFromPacket(pkt);
	printf("3번 잘되냐ㅕ\n");
	return true;
}

bool S2SPacketHandler::Handle_S2S_RES_HEART_BEAT(PacketSessionRef& session, Protocol::S2S_RES_HEART_BEAT& pkt)
{
	return true;
}
