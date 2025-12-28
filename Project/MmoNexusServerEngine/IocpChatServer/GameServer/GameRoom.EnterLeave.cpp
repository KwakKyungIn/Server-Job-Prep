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

bool GameRoom::EnterRegister(PlayerSessionRef session, PlayerRef player)
{
	printf("1번 여기가 문제임\n");
	if (player == nullptr) return false;

	if (IsClosing())
		return false;

	player->SetInstanceId(_instanceId);


	// 이미 들어와 있으면 실패 (중복 Enter 방지)
	if (_players.find(player->GetPlayerId()) != _players.end())
		return false;

	// 1) 룸 소속 설정
	player->SetRoom(shared_from_this());

	auto room = shared_from_this();
	session->Post([room](PlayerSessionRef ps)
		{
			ps->SetCurrentRoom(room);
		});

	_players.insert({ player->GetPlayerId(), player });


	// playerCount 증가 + emptySince 리셋
	_playerCount.fetch_add(1, std::memory_order_acq_rel);
	_emptySinceMs.store(0, std::memory_order_release);



	// 2) AOI Zone 계산 및 등록
	int32 zoneIndex = _grid.GetZoneIndex(*player->GetPosInfo());
	player->SetZoneIndex(zoneIndex);

	Zone& enterZone = _grid.GetZone(zoneIndex);
	enterZone.players.insert(player);

	printf("🎮 [EnterRegister] Player %llu Zone[%d] at (%.1f, %.1f, %.1f)\n",
		player->GetPlayerId(), zoneIndex,
		player->GetPosInfo()->x(),
		player->GetPosInfo()->y(),
		player->GetPosInfo()->z());

	return true;
}

void GameRoom::Enter(PlayerSessionRef session, PlayerRef player)
{
	if (!session || !player)
		return;

	if (EnterRegister(session, player) == false)
		return;


	if (player == nullptr) return;

	// 1) 응답 먼저
	Protocol::S_ENTER_GAME enterPkt;
	enterPkt.set_success(true);
	enterPkt.mutable_myplayer()->CopyFrom(*player->GetPlayerInfo());
	// NOTE: 네 proto에 mapid가 실제로 있으면 유지, 없으면 이 줄 삭제
	// enterPkt.set_mapid(_mapId);

	session->Send(ClientPacketHandler::MakeSendBuffer(enterPkt));

	// 2) 스폰 전송은 그 다음
	SendEnterSpawns(session, player);

	printf("✅ [Enter-Login] Player %llu\n", player->GetPlayerId());
}

// [맵 이동 입장]
void GameRoom::EnterMapChange(PlayerSessionRef session, PlayerRef player)
{
	printf("2번 여기가 문제임\n");
	if (!session || !player)
		return;

	if (EnterRegister(session, player) == false)
		return;

	if (player == nullptr) return;

	// 1) END 응답 먼저
	Protocol::S_MAP_CHANGE_END endPkt;
	endPkt.set_token(session->GetMapChangeToken());
	endPkt.set_mapid(_mapId);
	endPkt.mutable_pos()->CopyFrom(*player->GetPosInfo()); // proto: PositionInfo pos = 3
	endPkt.set_instanceid(player->GetInstanceId());
	session->Send(ClientPacketHandler::MakeSendBuffer(endPkt));

	// 2) 입력락 해제
	session->EndMapChange();

	// 3) 스폰은 그 다음
	SendEnterSpawns(session, player);

	printf("✅ [MapChange-END] Player %llu -> Map %d (Token=%llu)\n",
		player->GetPlayerId(), _mapId, endPkt.token());
}


void GameRoom::Leave(PlayerSessionRef session, PlayerRef player)
{
	if (player == nullptr) return;

	uint64 playerId = player->GetPlayerId();
	if (_players.find(playerId) == _players.end()) return;

	int32 zoneIndex = player->GetZoneIndex();

	// 1. Zone에서 제거 (AOI)
	int32 totalZones = _grid.GetGridSizeX() * _grid.GetGridSizeY();
	if (zoneIndex >= 0 && zoneIndex < totalZones)
	{
		Zone& zone = _grid.GetZone(zoneIndex);
		zone.players.erase(player);
	}

	// 2. 전체 명단 제거
	_players.erase(playerId);
	player->SetRoom(nullptr);

	auto room = shared_from_this();
	session->Post([room](PlayerSessionRef ps)
		{
			ps->ClearCurrentRoom(room);
		});

	const int32 after = _playerCount.fetch_sub(1, std::memory_order_acq_rel) - 1;
	if (after == 0)
	{
		const uint64 nowMs = ::GetTickCount64();
		_emptySinceMs.store(nowMs, std::memory_order_release);
	}


	// 3. [Broadcast] 주변 유저들에게 "나 나갔음" 알림 (S_DESPAWN)
	{
		Protocol::S_DESPAWN despawnPkt;
		despawnPkt.add_objectids(playerId);
		SendBufferRef sendBuffer = ClientPacketHandler::MakeSendBuffer(despawnPkt);

		BroadcastToZone(sendBuffer, zoneIndex, 0);
	}

	printf("[ROOM] Player %llu Left Zone[%d].\n", playerId, zoneIndex);
}

void GameRoom::LeaveById(PlayerSessionRef session, uint64 playerId)
{
	auto it = _players.find(playerId);
	if (it == _players.end())
		return;

	PlayerRef player = it->second;

	// 기존 Leave 로직 재사용(지금 단계에선 이게 제일 빠름)
	Leave(session, player);
}

PlayerRef GameRoom::FindPlayer_ActorOnly(uint64 playerId) const
{
	auto it = _players.find(playerId);
	if (it == _players.end())
		return nullptr;
	return it->second;
}
