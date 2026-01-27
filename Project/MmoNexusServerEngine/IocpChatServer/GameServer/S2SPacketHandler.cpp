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

// DB 서버로부터 로그인 결과를 받았을 때 호출됨
// 여기서 바로 처리하지 않고 로직 스레드로 넘기는 게 중요함
bool S2SPacketHandler::Handle_S2S_RES_LOGIN(PacketSessionRef& session, Protocol::S2S_RES_LOGIN& pkt)
{

	return true;
}

// DB에서 플레이어 기본 스탯 정보 로딩이 끝났다는 응답
// 비동기 IO라서 게임 서버는 다른 일 하다가 이 패킷이 오면 그제서야 다음 단계로 넘어감
bool S2SPacketHandler::Handle_S2S_RES_LOAD_PLAYER_DATA(PacketSessionRef& session, Protocol::S2S_RES_LOAD_PLAYER_DATA& pkt)
{
	// 패킷에 들어있는 세션 ID로 현재 접속 중인 세션을 찾음
	PlayerSessionRef ps = GameSessionManager::GSessionManager->FindBySessionId(pkt.gamesessionid());
	if (!ps) return true; // 세션이 이미 끊겼으면 무시

	if (!pkt.success())
	{
		ps->Post([](PlayerSessionRef self)
			{
				// 실패 시 클라한테 에러 알림 보내고 끊어야 함
				// 나중에 구현 예정
			});
		return true;
	}

	const uint64 playerId = pkt.playerid(); // DB가 보내준 ID가 가장 정확함
	if (playerId == 0) return true;

	// 아직 게임 방에 들어가지 않고 로비(대기 상태)에 있을 테니 로비 ID를 찾음
	const int32 ch = ps->GetPendingChannelId_AnyThread();
	if (ch <= 0 || GRoomManager == nullptr)
		return true;

	auto lobby = GRoomManager->GetOrCreateLobby(ch);
	if (!lobby) return true;

	// [Context Switch] 네트워크 스레드 -> 로비 액터 스레드
	// 동기화 문제를 피하기 위해 람다로 포장해서 JobQueue에 넣음
	lobby->Push([playerId, pkt, lobby]() mutable
		{
			lobby->OnStatLoaded(playerId, pkt);
		});

	return true;
}

// DB에서 아이템 목록 로딩 완료
// 스탯 로딩 -> 아이템 로딩 -> 퀵슬롯 로딩 순서로 진행됨 (체인 방식)
bool S2SPacketHandler::Handle_S2S_RES_ITEMS_LOAD(PacketSessionRef& session, Protocol::S2S_RES_ITEMS_LOAD& pkt)
{
	PlayerSessionRef ps = GameSessionManager::GSessionManager->FindBySessionId(pkt.gamesessionid());
	if (!ps) return true;

	if (!pkt.success())
	{
		ps->Post([](PlayerSessionRef self)
			{
				// 로딩 실패 처리
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

	// 역시 로비 스레드로 넘겨서 안전하게 아이템 세팅
	lobby->Push([playerId, pkt, lobby]() mutable
		{
			lobby->OnItemsLoaded(playerId, pkt);
		});

	return true;
}

// 서버 켜질 때 DB에서 기획 데이터(GameData) 로딩 완료
bool S2SPacketHandler::Handle_S2S_RES_LOAD_GAME_DATA(PacketSessionRef& session, Protocol::S2S_RES_LOAD_GAME_DATA& pkt)
{
	if (pkt.success() == false) return false;

	// 데이터 매니저에 밀어넣음. 이건 서버 켜질 때 한 번만 하니까 락 안 걸어도 됨
	DataManager::Instance()->LoadFromPacket(pkt);
	printf("3번 잘되냐ㅕ\n");
	return true;
}

bool S2SPacketHandler::Handle_S2S_RES_HEART_BEAT(PacketSessionRef& session, Protocol::S2S_RES_HEART_BEAT& pkt)
{
	return true;
}

// 플레이어 스탯 저장 성공 응답
// Write-Back 패턴: Redis에 Dirty Flag를 켜놨다가 DB 저장이 성공하면 끔
bool S2SPacketHandler::Handle_S2S_RES_SAVE_PLAYER_CORE(PacketSessionRef& session, Protocol::S2S_RES_SAVE_PLAYER_CORE& pkt)
{
	const uint64 pid = pkt.playerid();

	if (pkt.success())
		// 저장 성공했으니 Redis의 Dirty Flag를 해제해서 중복 저장을 막음
		Persistence::PersistenceService::I().ClearDirtyOnCommitSuccess(pid, /*coreOk=*/true, /*invOk=*/false, /*qsOk=*/false);

	// 자동 커밋 서비스에게 작업 끝났다고 알림
	Persistence::AutoCommitService::I().OnCommitFinished(pid);

	return true;
}

// 인벤토리 저장 성공 응답
bool S2SPacketHandler::Handle_S2S_RES_SAVE_INVENTORY(PacketSessionRef& session, Protocol::S2S_RES_SAVE_INVENTORY& pkt)
{
	const uint64 pid = pkt.playerid();

	if (pkt.success())
		// 인벤토리 쪽 Dirty Flag 해제
		Persistence::PersistenceService::I().ClearDirtyOnCommitSuccess(pid, /*coreOk=*/false, /*invOk=*/true,/*qsOk=*/false);

	Persistence::AutoCommitService::I().OnCommitFinished(pid);

	return true;
}

// 아이템 거래 결과 처리
// 2단계 커밋(2PC)이나 원자적 처리가 필요한 중요한 작업
bool S2SPacketHandler::Handle_S2S_RES_TRADE_COMMIT(PacketSessionRef& session, Protocol::S2S_RES_TRADE_COMMIT& pkt)
{
	if (GRoomManager == nullptr)
		return true;

	// 거래는 방 안에서 일어나므로 해당 게임 룸을 찾음
	auto room = GRoomManager->FindRoom(pkt.channelid(), pkt.mapid(), pkt.instanceid());
	if (!room)
		return true;

	// 룸 액터 스레드로 작업을 넘김
	room->Push([room, pkt]() mutable
		{
			room->OnTradeCommitResult(pkt);
		});

	return true;
}

// 아이템 생성 결과
bool S2SPacketHandler::Handle_S2S_RES_ITEM_CREATE(PacketSessionRef& session, Protocol::S2S_RES_ITEM_CREATE& pkt)
{
	// 비동기 생성 결과가 오면 서비스 레이어로 넘겨서 후처리
	// Persistence::AutoCommitService::I().OnItemCreateResult(pkt);
	return true;
}

// 아이템 UID 발급용 시드값 수신
// DB에 매번 UID 달라고 하면 느리니까, 서버가 켜질 때 1000개 단위로 범위를 할당받음
bool S2SPacketHandler::Handle_S2S_RES_GAME_ITEM_UID_SEED(PacketSessionRef& session, Protocol::S2S_RES_GAME_ITEM_UID_SEED& pkt)
{
	if (!pkt.success() || pkt.next_uid() == 0)
	{
		std::cout << " [UID] Seed load failed" << std::endl;
		return true;
	}

	// 메모리 상의 UID 생성기 초기화
	GameItemUidGen::Init(pkt.next_uid());
	std::cout << " [UID] Seed initialized. next_uid=" << pkt.next_uid() << std::endl;
	return true;
}

// 퀵슬롯 데이터 로딩 완료
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

	// 로비 스레드에서 퀵슬롯 정보 반영
	lobby->Push([playerId, pkt, lobby]() mutable
		{
			lobby->OnQuickSlotsLoaded(playerId, pkt);
		});

	return true;
}

// 퀵슬롯 저장 성공 응답
bool S2SPacketHandler::Handle_S2S_RES_SAVE_QUICKSLOT(PacketSessionRef& session, Protocol::S2S_RES_SAVE_QUICKSLOT& pkt)
{
	const uint64 pid = pkt.playerid();

	if (pkt.success())
		// 퀵슬롯 Dirty Flag 해제
		Persistence::PersistenceService::I().ClearDirtyOnCommitSuccess(pid, /*coreOk=*/false, /*invOk=*/false, /*qsOk=*/true);

	Persistence::AutoCommitService::I().OnCommitFinished(pid);
	return true;
}