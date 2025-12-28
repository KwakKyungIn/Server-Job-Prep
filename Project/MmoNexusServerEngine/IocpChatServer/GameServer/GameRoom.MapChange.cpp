#include "pch.h"
#include "GameRoom.h"
#include "GameMap.h"
#include "Player.h"
#include "PlayerSession.h"
#include "ClientPacketHandler.h"
#include "Monster.h"
#include "DataManager.h"
#include "ObjectUtils.h"
#include "BattleSystem.h"
#include "Zone.h"
#include "Creature.h"
#include "GameSessionManager.h"
#include "RoomManager.h"


void GameRoom::TransferMapChangeById(PlayerSessionRef session,
	uint64 playerId,
	int32 targetMapId,
	int64 targetInstanceId,
	const Protocol::PositionInfo& spawn)
{
	if (!session || !GRoomManager)
	{
		if (session) session->CancelMapChange();
		return;
	}

	auto it = _players.find(playerId);
	if (it == _players.end() || !it->second)
	{
		session->CancelMapChange();
		return;
	}

	PlayerRef player = it->second;

	// ✅ 채널은 “player 값” 우선 (없으면 old room 값)
	int32 channelId = player->GetChannelId();
	if (channelId <= 0) channelId = _channelId;

	// ✅ 실패하면 Leave 하기 전에 컷 (유실 방지)
	auto lobby = GRoomManager->GetOrCreateLobby(channelId);
	auto newRoom = GRoomManager->GetOrCreateRoom(channelId, targetMapId, targetInstanceId);
	if (!lobby || !newRoom)
	{
		session->CancelMapChange();
		return;
	}

	// 1) OldRoom에서 Leave (기존 로직 재사용)
	//    ⚠️ Leave가 player->SetSession(nullptr) 같은 걸 한다면 아래에서 복구해준다.
	Leave(session, player);

	// 2) Player 상태 갱신 (Room 소유)
	player->SetMapId(targetMapId);
	player->SetInstanceId(targetInstanceId);
	if (player->GetPosInfo())
		player->GetPosInfo()->CopyFrom(spawn);

	// ✅ Leave가 세션 링크를 끊는 구현이면 복구(안 끊으면 이 줄은 무해)
	player->SetSession(session);

	// 3) “무소속 방지” : OldRoom thread에서 즉시 Lobby로 room 포인터를 걸어준다
	//    (실제 Adopt는 Lobby actor에서)
	player->SetRoom(lobby);

	// 4) Session 라우팅도 Lobby로 (MapChanging 중이라 실제 라우팅은 막혀있지만,
	//    disconnect 같은 예외에서 currentRoom이 필요함)
	session->Post([lobby](PlayerSessionRef s)
		{
			s->SetCurrentRoom(lobby);
		});

	const uint64 pid = player->GetPlayerId();

	// 5) Lobby가 Player를 “보관”한 뒤, NewRoom으로 전달
	lobby->Push([lobby, newRoom, session, player, pid]()
		{
			// Lobby 소유 확정
			lobby->Adopt(player);

			// NewRoom 입장
			newRoom->Push([newRoom, lobby, session, player, pid]()
				{
					newRoom->EnterMapChange(session, player);

					// ✅ 세션 FSM 종료는 “NewRoom 들어간 뒤”에
					session->Post([newRoom](PlayerSessionRef s)
						{
							s->SetCurrentRoom(newRoom);
							s->EndMapChange();
						});

					// ✅ Lobby 기록 제거 (겹침은 OK, gap만 없애면 된다)
					lobby->Push([lobby, pid]()
						{
							lobby->Detach(pid);
						});
				});
		});
}
// GameRoom.cpp
void GameRoom::SaveReturnLocation_ActorOnly(uint64 playerId)
{
	PlayerRef p = FindPlayer_ActorOnly(playerId);
	if (!p) return;

	auto pos = p->GetPosInfo();
	if (!pos) return;

	// ✅ 정책: return map/instance는 "Player가 들고있는 값" 기준
	p->SetReturnLocation(p->GetMapId(), p->GetInstanceId(), *pos);
}

