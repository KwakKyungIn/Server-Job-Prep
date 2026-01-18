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
#include "PersistenceService.h"
#include "AutoCommitService.h"
#include "GameItemUidGen.h"

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
				// TODO: 실패 처리(클라에게 에러패킷/디스커넥트 등)
			});
		return true;
	}

	const uint64 playerId = pkt.playerid(); //  패킷에 있음. 이게 제일 정직함.
	if (playerId == 0) return true;

	const int32 ch = ps->GetPendingChannelId_AnyThread();
	if (ch <= 0 || GRoomManager == nullptr)
		return true;

	auto lobby = GRoomManager->GetOrCreateLobby(ch);
	if (!lobby) return true;

	lobby->Push([playerId, pkt, lobby]() mutable
		{
			lobby->OnStatLoaded(playerId, pkt);
		});

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
				// TODO: 실패 처리(클라에게 에러패킷/디스커넥트 등)
			});
		return true;
	}

	const uint64 playerId = pkt.playerid();
	if (playerId == 0) return true;

	const int32 ch = ps->GetPendingChannelId_AnyThread();
	if (ch <= 0 || GRoomManager == nullptr)
		return true;

	auto lobby = GRoomManager->GetOrCreateLobby(ch);
	if (!lobby) return true;

	lobby->Push([playerId, pkt, lobby]() mutable
		{
			lobby->OnItemsLoaded(playerId, pkt);
		});

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

bool S2SPacketHandler::Handle_S2S_RES_SAVE_PLAYER_CORE(PacketSessionRef& session, Protocol::S2S_RES_SAVE_PLAYER_CORE& pkt)
{
	const uint64 pid = pkt.playerid();

	if (pkt.success())
		Persistence::PersistenceService::I().ClearDirtyOnCommitSuccess(pid, /*coreOk=*/true, /*invOk=*/false, /*qsOk=*/false);

	Persistence::AutoCommitService::I().OnCommitFinished(pid);

	return true;
}


bool S2SPacketHandler::Handle_S2S_RES_SAVE_INVENTORY(PacketSessionRef& session, Protocol::S2S_RES_SAVE_INVENTORY& pkt)
{
	const uint64 pid = pkt.playerid();

	if (pkt.success())
		Persistence::PersistenceService::I().ClearDirtyOnCommitSuccess(pid, /*coreOk=*/false, /*invOk=*/true,/*qsOk=*/false);

	Persistence::AutoCommitService::I().OnCommitFinished(pid);

	return true;
}


bool S2SPacketHandler::Handle_S2S_RES_ITEM_CREATE(PacketSessionRef& session, Protocol::S2S_RES_ITEM_CREATE& pkt)
{
	// 성공/실패 포함해서 서비스에 넘겨라 (실패면 서비스가 재시도/로그)
	//Persistence::AutoCommitService::I().OnItemCreateResult(pkt);
	return true;
}

bool S2SPacketHandler::Handle_S2S_RES_GAME_ITEM_UID_SEED(PacketSessionRef& session, Protocol::S2S_RES_GAME_ITEM_UID_SEED& pkt)
{
	if (!pkt.success() || pkt.next_uid() == 0)
	{
		std::cout << " [UID] Seed load failed" << std::endl;
		return true;
	}

	GameItemUidGen::Init(pkt.next_uid());
	std::cout << " [UID] Seed initialized. next_uid=" << pkt.next_uid() << std::endl;
	return true;
}

bool S2SPacketHandler::Handle_S2S_RES_QUICKSLOT_LOAD(PacketSessionRef& session, Protocol::S2S_RES_QUICKSLOT_LOAD& pkt)
{
	PlayerSessionRef ps = GameSessionManager::GSessionManager->FindBySessionId(pkt.gamesessionid());
	if (!ps) return true;

	const uint64 playerId = pkt.playerid();
	if (playerId == 0) return true;

	const int32 ch = ps->GetPendingChannelId_AnyThread();
	if (ch <= 0 || GRoomManager == nullptr)
		return true;

	auto lobby = GRoomManager->GetOrCreateLobby(ch);
	if (!lobby) return true;

	lobby->Push([playerId, pkt, lobby]() mutable
		{
			lobby->OnQuickSlotsLoaded(playerId, pkt);
		});

	return true;
}


bool S2SPacketHandler::Handle_S2S_RES_SAVE_QUICKSLOT(PacketSessionRef& session, Protocol::S2S_RES_SAVE_QUICKSLOT& pkt)
{
	const uint64 pid = pkt.playerid();

	if (pkt.success())
		Persistence::PersistenceService::I().ClearDirtyOnCommitSuccess(pid, /*coreOk=*/false, /*invOk=*/false, /*qsOk=*/true);

	Persistence::AutoCommitService::I().OnCommitFinished(pid);
	return true;
}
