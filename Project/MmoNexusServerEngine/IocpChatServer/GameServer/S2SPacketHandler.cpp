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
	auto playerSession = static_pointer_cast<PlayerSession>(GameSessionManager::GSessionManager->Find(pkt.gamesessionid()));
	if (playerSession == nullptr) return true;

	playerSession->PushJob(ObjectPool<Job>::MakeShared([playerSession, pkt]()
		{
			auto player = playerSession->GetPlayer();
			if (player == nullptr) return;

			if (pkt.success())
			{
				// 1. DB에서 가져온 기본 스탯(Level, HP, Exp) 적용
				// CopyFrom을 쓰면 편리하지만, 필요한 것만 쏙쏙 뽑아 넣는 게 안전할 때도 있음.
				// 여기선 StatInfo 전체 구조가 같으니 CopyFrom 사용.
				player->GetStatInfo()->CopyFrom(pkt.statinfo());

				// 2. [Critical] 레벨이 세팅되었으니, 최종 스탯(MaxHP, Atk 등) 재계산!
				// 이 함수 안에서 DataManager를 조회하여 1레벨(혹은 N레벨)의 템플릿 스탯을 적용함.
				player->RefreshStats();

				std::cout << "👤 [Player] Stat Loaded. Lv:" << player->GetStatInfo()->level()
					<< " HP:" << player->GetStatInfo()->hp() << std::endl;


				if (GRoomManager)
				{
					int32 channelId = player->GetChannelId();
					int32 mapId = player->GetMapId();

					auto room = GRoomManager->GetOrCreateRoom(channelId, mapId);
					if (room)
					{
						room->PushJob(&GameRoom::Enter, playerSession);
						std::cout << "🚪 [GameRoom] Player Entered Room! (Channel "
							<< channelId << ", Map " << mapId << ")" << std::endl;
					}
				}

			}
		}));

	return true;
}

// [DB -> Game] 아이템 로딩 완료
bool S2SPacketHandler::Handle_S2S_RES_ITEMS_LOAD(PacketSessionRef& session, Protocol::S2S_RES_ITEMS_LOAD& pkt)
{
	auto playerSession = static_pointer_cast<PlayerSession>(GameSessionManager::GSessionManager->Find(pkt.gamesessionid()));
	if (playerSession == nullptr) return true;

	playerSession->PushJob(ObjectPool<Job>::MakeShared([playerSession, pkt]()
		{
			auto player = playerSession->GetPlayer();
			if (player == nullptr) return;

			if (pkt.success())
			{
				player->SetItems(pkt.items());

				// 아이템이 로드되었으니 장착 효과 적용을 위해 스탯 재계산
				player->RefreshStats();

				Protocol::S_ITEM_LIST clientPkt;
				clientPkt.mutable_items()->CopyFrom(pkt.items());
				auto sendBuffer = ClientPacketHandler::MakeSendBuffer(clientPkt);
				playerSession->Send(sendBuffer);

				std::cout << "📦 [Inventory] Synced " << pkt.items_size() << " items." << std::endl;
			}
		}));

	return true;
}

bool S2SPacketHandler::Handle_S2S_RES_LOAD_GAME_DATA(PacketSessionRef& session, Protocol::S2S_RES_LOAD_GAME_DATA& pkt)
{
	if (pkt.success() == false) return false;
	DataManager::Instance()->LoadFromPacket(pkt);
	return true;
}

bool S2SPacketHandler::Handle_S2S_RES_BROADCAST_CHAT(PacketSessionRef& session, Protocol::S2S_RES_BROADCAST_CHAT& pkt)
{
	return true;
}

bool S2SPacketHandler::Handle_S2S_RES_HEART_BEAT(PacketSessionRef& session, Protocol::S2S_RES_HEART_BEAT& pkt)
{
	return true;
}

bool S2SPacketHandler::Handle_S2S_RES_PARTY_SYNC(PacketSessionRef& session, Protocol::S2S_RES_PARTY_SYNC& pkt)
{
	std::cout << "[GameServer] PARTY_SYNC_ACK partyId=" << pkt.partyid()
		<< " success=" << pkt.success()
		<< " ver=" << pkt.version() << std::endl;
	return true;
}

bool S2SPacketHandler::Handle_S2S_RES_PARTY_CHAT(PacketSessionRef& session, Protocol::S2S_RES_PARTY_CHAT& pkt)
{
	std::cout << "[GameServer] PARTY_CHAT_ACK partyId=" << pkt.partyid()
		<< " success=" << pkt.success()
		<< " senderId=" << pkt.senderid()
		<< " name=" << pkt.sendername()
		<< " msg=" << pkt.message()
		<< " ver=" << pkt.version() << std::endl;

	// Step1: 아직 파티 브로드캐스트는 안 한다. (Step3에서)
	return true;
}
