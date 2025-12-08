#include "pch.h"
#include "GameRoom.h"
#include "GameMap.h"
#include "Player.h"
#include "PlayerSession.h"
#include "ClientPacketHandler.h"

GameRoom::GameRoom()
{
	_jobQueue = MakeShared<JobQueue>();
}

GameRoom::~GameRoom()
{
}

void GameRoom::Init(int32 mapId, int32 sizeX, int32 sizeY, int32 zoneSize)
{
	_map = MakeShared<GameMap>();
	_map->Init(mapId, sizeX, sizeY);

	// [Spatial Partitioning Init]
	_zoneCellSize = zoneSize;
	_gridSizeX = (sizeX + zoneSize - 1) / zoneSize;
	_gridSizeY = (sizeY + zoneSize - 1) / zoneSize;

	_zones.resize(_gridSizeX * _gridSizeY);

	printf("[GameRoom] Init MapId: %d, Grid: %dx%d, CellSize: %d\n", mapId, _gridSizeX, _gridSizeY, zoneSize);
}

void GameRoom::Update()
{
	// TODO: 몬스터 AI, 리스폰 로직 등 주기적 작업
}

void GameRoom::Enter(PlayerSessionRef session)
{
	// [Refactoring] 세션은 단지 통로일 뿐, 주인공은 Player다.
	// 로그인 단계에서 이미 생성된 Player 객체를 가져온다.
	PlayerRef player = session->GetPlayer();
	if (player == nullptr)
		return;

	// 중복 입장 체크
	if (_players.find(player->GetPlayerId()) != _players.end())
		return;

	// 1. 방 설정 (Session이 아니라 Player가 방을 기억함)
	player->SetRoom(shared_from_this());

	// 2. 전체 명단 등록
	_players.insert({ player->GetPlayerId(), player });

	// 3. [AOI] Zone 진입 처리
	// PlayerInfo는 이제 player 객체가 들고 있으므로 바로 접근 가능
	int32 zoneIndex = GetZoneIndex(*player->GetPosInfo());
	player->SetZoneIndex(zoneIndex);
	_zones[zoneIndex].players.insert(player);

	// 4. [Broadcast] 주변 9개 Zone에 "내가 들어왔음" 알림 (S_SPAWN)
	{
		Protocol::S_SPAWN spawnPkt;
		Protocol::PlayerInfo* pInfo = spawnPkt.add_players();
		*pInfo = *player->GetPlayerInfo();
		SendBufferRef sendBuffer = ClientPacketHandler::MakeSendBuffer(spawnPkt);

		Vector<Zone*> nearbyZones;
		GetNearbyZones(zoneIndex, nearbyZones);

		for (Zone* zone : nearbyZones)
		{
			for (const PlayerRef& other : zone->players)
			{
				if (other != player)
					other->GetSession()->Send(sendBuffer);
			}
		}
	}

	// 5. [To Me] 나에게 "주변 유저들 정보" 알림 (S_SPAWN)
	{
		Vector<Zone*> nearbyZones;
		GetNearbyZones(zoneIndex, nearbyZones);

		Protocol::S_SPAWN spawnPkt;
		for (Zone* zone : nearbyZones)
		{
			for (const PlayerRef& other : zone->players)
			{
				if (other != player)
				{
					Protocol::PlayerInfo* pInfo = spawnPkt.add_players();
					*pInfo = *other->GetPlayerInfo();
				}
			}
		}

		if (spawnPkt.players_size() > 0)
		{
			SendBufferRef sendBuffer = ClientPacketHandler::MakeSendBuffer(spawnPkt);
			session->Send(sendBuffer);

			printf("[ROOM] Player %llu Entered Zone[%d]. Received %d neighbors.\n", player->GetPlayerId(), zoneIndex, spawnPkt.players_size());
		}
	}
}

void GameRoom::Leave(PlayerSessionRef session)
{
	// 세션에서 플레이어 정보를 가져옴
	PlayerRef player = session->GetPlayer();
	if (player == nullptr) return;

	uint64 playerId = player->GetPlayerId();

	// 명단 체크
	if (_players.find(playerId) == _players.end()) return;

	int32 zoneIndex = player->GetZoneIndex();

	// 1. Zone에서 제거
	if (zoneIndex >= 0 && zoneIndex < _zones.size())
	{
		_zones[zoneIndex].players.erase(player);
	}

	// 2. 전체 명단 제거
	_players.erase(playerId);

	// [Refactoring] 방 정보 해제 (Player에게 알림)
	player->SetRoom(nullptr);

	// 3. [Broadcast] 주변 유저들에게 "나 나갔음" 알림 (S_DESPAWN)
	{
		Protocol::S_DESPAWN despawnPkt;
		despawnPkt.add_playerids(playerId);
		SendBufferRef sendBuffer = ClientPacketHandler::MakeSendBuffer(despawnPkt);

		BroadcastToZone(sendBuffer, zoneIndex, 0);
	}

	printf("[ROOM] Player %llu Left Zone[%d].\n", playerId, zoneIndex);
}

void GameRoom::HandleMove(PlayerSessionRef session, Protocol::C_MOVE pkt)
{
	// [Refactoring] Session -> Player 접근
	PlayerRef player = session->GetPlayer();
	if (player == nullptr) return;

	uint64 playerId = player->GetPlayerId();

	// 방에 없는 유저라면 무시
	if (_players.find(playerId) == _players.end()) return;

	// 1. [Validation]
	if (_map->CanGo(pkt.posinfo()) == false)
		return;

	// 2. [Zone Check]
	int32 oldZoneIndex = player->GetZoneIndex();
	int32 newZoneIndex = GetZoneIndex(pkt.posinfo());

	// 3. [Update] 정보 갱신 (Player 객체 내부 데이터 수정)
	player->SetPosInfo(pkt.posinfo());

	// [Case A] 같은 Zone 내 이동
	if (oldZoneIndex == newZoneIndex)
	{
		Protocol::S_MOVE movePkt;
		movePkt.set_playerid(playerId);
		*movePkt.mutable_posinfo() = pkt.posinfo();
		SendBufferRef sendBuffer = ClientPacketHandler::MakeSendBuffer(movePkt);

		BroadcastToZone(sendBuffer, newZoneIndex, playerId);
	}
	// [Case B] Zone 변경 발생
	else
	{
		Vector<int32> oldZones;
		GetNearbyZoneIndices(oldZoneIndex, oldZones);
		std::sort(oldZones.begin(), oldZones.end());

		Vector<int32> newZones;
		GetNearbyZoneIndices(newZoneIndex, newZones);
		std::sort(newZones.begin(), newZones.end());

		// (Old - New) : Despawn Group
		{
			Vector<int32> removedZones;
			std::set_difference(oldZones.begin(), oldZones.end(),
				newZones.begin(), newZones.end(),
				std::back_inserter(removedZones));

			Protocol::S_DESPAWN despawnPkt;
			despawnPkt.add_playerids(playerId);
			SendBufferRef sendBuffer = ClientPacketHandler::MakeSendBuffer(despawnPkt);

			Protocol::S_DESPAWN despawnToMePkt;

			for (int32 zoneIdx : removedZones)
			{
				Zone& zone = _zones[zoneIdx];
				for (auto& p : zone.players)
				{
					if (p->GetPlayerId() != playerId)
					{
						p->GetSession()->Send(sendBuffer);
						despawnToMePkt.add_playerids(p->GetPlayerId());
					}
				}
			}

			if (despawnToMePkt.playerids_size() > 0)
			{
				SendBufferRef despawnToMeBuffer = ClientPacketHandler::MakeSendBuffer(despawnToMePkt);
				session->Send(despawnToMeBuffer);
			}
		}

		// (New - Old) : Spawn Group
		{
			Vector<int32> addedZones;
			std::set_difference(newZones.begin(), newZones.end(),
				oldZones.begin(), oldZones.end(),
				std::back_inserter(addedZones));

			Protocol::S_SPAWN mySpawnPkt;
			auto* myInfo = mySpawnPkt.add_players();
			*myInfo = *player->GetPlayerInfo();
			SendBufferRef mySpawnBuffer = ClientPacketHandler::MakeSendBuffer(mySpawnPkt);

			Protocol::S_SPAWN othersSpawnPkt;

			for (int32 zoneIdx : addedZones)
			{
				Zone& zone = _zones[zoneIdx];
				for (auto& p : zone.players)
				{
					if (p->GetPlayerId() != playerId)
					{
						p->GetSession()->Send(mySpawnBuffer);
						auto* otherInfo = othersSpawnPkt.add_players();
						*otherInfo = *p->GetPlayerInfo();
					}
				}
			}

			if (othersSpawnPkt.players_size() > 0)
			{
				SendBufferRef othersSpawnBuffer = ClientPacketHandler::MakeSendBuffer(othersSpawnPkt);
				session->Send(othersSpawnBuffer);
			}
		}

		// (Old ∩ New) : Move Group
		{
			Vector<int32> commonZones;
			std::set_intersection(oldZones.begin(), oldZones.end(),
				newZones.begin(), newZones.end(),
				std::back_inserter(commonZones));

			Protocol::S_MOVE movePkt;
			movePkt.set_playerid(playerId);
			*movePkt.mutable_posinfo() = pkt.posinfo();
			SendBufferRef sendBuffer = ClientPacketHandler::MakeSendBuffer(movePkt);

			for (int32 zoneIdx : commonZones)
			{
				Zone& zone = _zones[zoneIdx];
				for (auto& p : zone.players)
				{
					if (p->GetPlayerId() != playerId)
						p->GetSession()->Send(sendBuffer);
				}
			}
		}

		// 서버 내 Zone 이동 반영
		_zones[oldZoneIndex].players.erase(player);
		_zones[newZoneIndex].players.insert(player);
		player->SetZoneIndex(newZoneIndex);
	}
}

// [Helper Impl]
void GameRoom::GetNearbyZoneIndices(int32 zoneIndex, Vector<int32>& outIndices)
{
	outIndices.clear();
	if (zoneIndex < 0 || zoneIndex >= _zones.size()) return;

	int32 x = zoneIndex % _gridSizeX;
	int32 y = zoneIndex / _gridSizeX;

	for (int32 dy = -1; dy <= 1; dy++)
	{
		for (int32 dx = -1; dx <= 1; dx++)
		{
			int32 nx = x + dx;
			int32 ny = y + dy;
			if (nx >= 0 && nx < _gridSizeX && ny >= 0 && ny < _gridSizeY)
			{
				int32 index = ny * _gridSizeX + nx;
				outIndices.push_back(index);
			}
		}
	}
}


int32 GameRoom::GetZoneIndex(const Protocol::PositionInfo& posInfo)
{
	int32 x = static_cast<int32>(posInfo.x());
	int32 y = static_cast<int32>(posInfo.z());

	int32 minX = _map->GetMinX();
	int32 minY = _map->GetMinY();
	int32 maxX = _map->GetMaxX();
	int32 maxY = _map->GetMaxY();

	if (x < minX) x = minX;
	if (x >= maxX) x = maxX - 1;
	if (y < minY) y = minY;
	if (y >= maxY) y = maxY - 1;

	int32 zoneX = (x - minX) / _zoneCellSize;
	int32 zoneY = (y - minY) / _zoneCellSize;

	return zoneY * _gridSizeX + zoneX;
}

void GameRoom::GetNearbyZones(int32 zoneIndex, Vector<Zone*>& outZones)
{
	outZones.clear();
	if (zoneIndex < 0 || zoneIndex >= _zones.size()) return;

	int32 x = zoneIndex % _gridSizeX;
	int32 y = zoneIndex / _gridSizeX;

	for (int32 dy = -1; dy <= 1; dy++)
	{
		for (int32 dx = -1; dx <= 1; dx++)
		{
			int32 nx = x + dx;
			int32 ny = y + dy;
			if (nx >= 0 && nx < _gridSizeX && ny >= 0 && ny < _gridSizeY)
			{
				int32 index = ny * _gridSizeX + nx;
				outZones.push_back(&_zones[index]);
			}
		}
	}
}

void GameRoom::BroadcastToZone(SendBufferRef sendBuffer, int32 zoneIndex, uint64 exceptId)
{
	Vector<Zone*> nearbyZones;
	GetNearbyZones(zoneIndex, nearbyZones);

	for (Zone* zone : nearbyZones)
	{
		for (const PlayerRef& p : zone->players)
		{
			if (p->GetPlayerId() == exceptId) continue;
			p->GetSession()->Send(sendBuffer);
		}
	}
}

void GameRoom::Broadcast(SendBufferRef sendBuffer, uint64 exceptId)
{
	for (auto& item : _players)
	{
		PlayerRef p = item.second;
		if (p->GetPlayerId() == exceptId) continue;
		p->GetSession()->Send(sendBuffer);
	}
}