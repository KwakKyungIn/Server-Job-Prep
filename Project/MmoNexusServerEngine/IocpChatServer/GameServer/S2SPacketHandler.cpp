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
	//uint64 userSessionId = pkt.playersessionid();
	//auto playerSession = static_pointer_cast<PlayerSession>(GameSessionManager::GSessionManager->Find(userSessionId));

	//if (playerSession == nullptr)
	//{
	//	std::cout << "💀 [FAIL] Session Not Found! ID : " << userSessionId << std::endl;
	//	return true;
	//}

	//playerSession->PushJob(ObjectPool<Job>::MakeShared([playerSession, session, pkt]()
	//	{
	//		Protocol::S_LOGIN_RES resPkt;
	//		resPkt.set_success(pkt.success());
	//		resPkt.set_playerid(pkt.playerid());

	//		auto sendBuffer = ClientPacketHandler::MakeSendBuffer(resPkt);
	//		playerSession->Send(sendBuffer);

	//		if (pkt.success())
	//		{
	//			// 1. Player 생성 및 기본 ID 설정
	//			shared_ptr<Player> newPlayer = make_shared<Player>();
	//			newPlayer->SetSession(playerSession);
	//			newPlayer->GetPlayerInfo()->set_playerid(pkt.playerid());
	//			newPlayer->GetPlayerInfo()->set_name("Player_" + std::to_string(pkt.playerid()));

	//			// 2. 세션 연결
	//			playerSession->SetPlayer(newPlayer);

	//			std::cout << "✅ [Login Success] SessionID: " << playerSession->GetSessionId() << " -> PlayerID: " << pkt.playerid() << std::endl;

	//			// 3. [GIGACHAD FLOW] 데이터 로딩 2연타 요청
	//			// (1) 아이템 로딩 요청
	//			{
	//				Protocol::S2S_REQ_ITEMS_LOAD itemReq;
	//				itemReq.set_playerid(pkt.playerid());
	//				itemReq.set_gamesessionid(playerSession->GetSessionId());
	//				session->Send(S2SPacketHandler::MakeSendBuffer(itemReq));
	//			}

	//			// (2) 플레이어 스탯 로딩 요청 (NEW)
	//			{
	//				Protocol::S2S_REQ_LOAD_PLAYER_DATA statReq;
	//				statReq.set_playerid(pkt.playerid());
	//				statReq.set_gamesessionid(playerSession->GetSessionId());
	//				session->Send(S2SPacketHandler::MakeSendBuffer(statReq));
	//			}

	//			std::cout << "🚀 [Game] Requested Full Data Load (Items + Stats) for Player: " << pkt.playerid() << std::endl;
	//		}
	//		else
	//		{
	//			std::cout << "❌ [Login Failed] SessionID: " << playerSession->GetSessionId() << std::endl;
	//		}
	//	}));

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
	return true;
}

bool S2SPacketHandler::Handle_S2S_RES_LOAD_GAME_DATA(PacketSessionRef& session, Protocol::S2S_RES_LOAD_GAME_DATA& pkt)
{
	if (pkt.success() == false) return false;
	DataManager::Instance()->LoadFromPacket(pkt);
	return true;
}

bool S2SPacketHandler::Handle_S2S_RES_HEART_BEAT(PacketSessionRef& session, Protocol::S2S_RES_HEART_BEAT& pkt)
{
	return true;
}
