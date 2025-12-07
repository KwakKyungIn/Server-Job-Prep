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
	// 맵 크기를 Zone 크기로 나누어 격자 개수 계산 (올림 처리)
	_gridSizeX = (sizeX + zoneSize - 1) / zoneSize;
	_gridSizeY = (sizeY + zoneSize - 1) / zoneSize;

	// 1차원 배열로 2D 격자 할당
	_zones.resize(_gridSizeX * _gridSizeY);

	printf("[GameRoom] Init MapId: %d, Grid: %dx%d, CellSize: %d\n", mapId, _gridSizeX, _gridSizeY, zoneSize);
}

void GameRoom::Update()
{
	// TODO: 몬스터 AI, 리스폰 로직 등 주기적 작업
}

void GameRoom::Enter(PlayerSessionRef session)
{
	// 중복 입장 체크
	if (_players.find(session->GetPlayerId()) != _players.end())
		return;

	// 1. Player 객체 생성 및 세션 연결 (Wrapper 패턴)
	PlayerRef player = MakeShared<Player>();
	player->SetSession(session);
	// PlayerSession이 들고 있는 정보로 초기화 (DB에서 로드된 정보 등)
	// [주의] PlayerSession에 GetPlayerInfo()가 구현되어 있어야 함
	player->Init(*session->GetPlayerInfo());

	session->SetRoom(shared_from_this());

	// 2. 전체 명단 등록
	_players.insert({ player->GetPlayerId(), player });

	// 3. [AOI] Zone 진입 처리
	int32 zoneIndex = GetZoneIndex(*player->GetPosInfo());
	player->SetZoneIndex(zoneIndex);
	_zones[zoneIndex].players.insert(player);

	// 4. [Broadcast] 주변 9개 Zone에 "내가 들어왔음" 알림 (S_SPAWN)
	{
		Protocol::S_SPAWN spawnPkt;
		Protocol::PlayerInfo* pInfo = spawnPkt.add_players();
		*pInfo = *player->GetPlayerInfo();
		SendBufferRef sendBuffer = ClientPacketHandler::MakeSendBuffer(spawnPkt);

		// 나 자신을 포함한 주변 Zone 유저들에게 전송 (단, 나한테는 안 보냄)
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
				if (other != player) // 나는 제외
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
	uint64 playerId = session->GetPlayerId();
	auto it = _players.find(playerId);
	if (it == _players.end()) return;

	PlayerRef player = it->second;
	int32 zoneIndex = player->GetZoneIndex();

	// 1. Zone에서 제거
	if (zoneIndex >= 0 && zoneIndex < _zones.size())
	{
		_zones[zoneIndex].players.erase(player);
	}

	// 2. 전체 명단 제거
	_players.erase(playerId);
	session->SetRoom(nullptr);

	// 3. [Broadcast] 주변 유저들에게 "나 나갔음" 알림 (S_DESPAWN)
	{
		Protocol::S_DESPAWN despawnPkt;
		despawnPkt.add_playerids(playerId);
		SendBufferRef sendBuffer = ClientPacketHandler::MakeSendBuffer(despawnPkt);

		// 주변 Zone에 뿌림 (나가는 사람은 이미 Zone/Map에서 빠졌으므로 exceptId 체크 불필요)
		BroadcastToZone(sendBuffer, zoneIndex, 0);
	}

	printf("[ROOM] Player %llu Left Zone[%d].\n", playerId, zoneIndex);
}

void GameRoom::HandleMove(PlayerSessionRef session, Protocol::C_MOVE pkt)
{
	uint64 playerId = session->GetPlayerId();
	auto it = _players.find(playerId);
	if (it == _players.end()) return;

	PlayerRef player = it->second;

	// 1. [Validation]
	if (_map->CanGo(pkt.posinfo()) == false)
		return;

	// 2. [Zone Check]
	int32 oldZoneIndex = player->GetZoneIndex();
	int32 newZoneIndex = GetZoneIndex(pkt.posinfo());

	// 3. [Update] 정보 갱신
	player->SetPosInfo(pkt.posinfo());
	

	// [Case A] 같은 Zone 내 이동 (가장 빈번함 -> 최적화)
	if (oldZoneIndex == newZoneIndex)
	{
		Protocol::S_MOVE movePkt;
		movePkt.set_playerid(playerId);
		*movePkt.mutable_posinfo() = pkt.posinfo();
		SendBufferRef sendBuffer = ClientPacketHandler::MakeSendBuffer(movePkt);

		// 주변 9개 존에 S_MOVE 전송 (나 제외)
		BroadcastToZone(sendBuffer, newZoneIndex, playerId);
	}
	// [Case B] Zone 변경 발생 (정밀 제어 필요)
	else
	{
		// 1. Zone Index 목록 구하기
		Vector<int32> oldZones;
		GetNearbyZoneIndices(oldZoneIndex, oldZones);
		std::sort(oldZones.begin(), oldZones.end()); // 집합 연산을 위해 정렬 필수

		Vector<int32> newZones;
		GetNearbyZoneIndices(newZoneIndex, newZones);
		std::sort(newZones.begin(), newZones.end());

		// 2. [Despawn Group] (Old - New) : 이제 안 보이게 될 애들
		{
			Vector<int32> removedZones;
			std::set_difference(oldZones.begin(), oldZones.end(),
				newZones.begin(), newZones.end(),
				std::back_inserter(removedZones));

			// 다른 플레이어들에게 "나(A)가 사라졌어" 알림
			Protocol::S_DESPAWN despawnPkt;
			despawnPkt.add_playerids(playerId);
			SendBufferRef sendBuffer = ClientPacketHandler::MakeSendBuffer(despawnPkt);

			// 나(A)에게 "그들이 사라졌어" 알릴 패킷
			Protocol::S_DESPAWN despawnToMePkt;

			for (int32 zoneIdx : removedZones)
			{
				Zone& zone = _zones[zoneIdx];
				for (auto& p : zone.players)
				{
					if (p->GetPlayerId() != playerId)
					{
						// 그들에게 나를 Despawn
						p->GetSession()->Send(sendBuffer);

						// 나에게도 그들을 Despawn
						despawnToMePkt.add_playerids(p->GetPlayerId());
					}
				}
			}

			// 나에게 사라진 플레이어들 정보 전송
			if (despawnToMePkt.playerids_size() > 0)
			{
				SendBufferRef despawnToMeBuffer = ClientPacketHandler::MakeSendBuffer(despawnToMePkt);
				session->Send(despawnToMeBuffer);
			}
		}

		// 3. [Spawn Group] (New - Old) : 새로 보게 될 애들
		{
			Vector<int32> addedZones;
			std::set_difference(newZones.begin(), newZones.end(),
				oldZones.begin(), oldZones.end(),
				std::back_inserter(addedZones));

			// 다른 플레이어들에게 "나(A)가 왔어" 알림
			Protocol::S_SPAWN mySpawnPkt;
			auto* myInfo = mySpawnPkt.add_players();
			*myInfo = *player->GetPlayerInfo();
			SendBufferRef mySpawnBuffer = ClientPacketHandler::MakeSendBuffer(mySpawnPkt);

			// 나(A)에게 "거기 누가 있어" 알릴 패킷
			Protocol::S_SPAWN othersSpawnPkt;

			for (int32 zoneIdx : addedZones)
			{
				Zone& zone = _zones[zoneIdx];
				for (auto& p : zone.players)
				{
					if (p->GetPlayerId() != playerId)
					{
						// 그들에게 나를 Spawn
						p->GetSession()->Send(mySpawnBuffer);

						// 나에게 보낼 정보에 그들을 추가
						auto* otherInfo = othersSpawnPkt.add_players();
						*otherInfo = *p->GetPlayerInfo();
					}
				}
			}

			// 나에게 새로 보이는 플레이어들 정보 전송
			if (othersSpawnPkt.players_size() > 0)
			{
				SendBufferRef othersSpawnBuffer = ClientPacketHandler::MakeSendBuffer(othersSpawnPkt);
				session->Send(othersSpawnBuffer);
			}
		}

		// 4. [Move Group] (Old ∩ New) : 계속 같이 있는 애들 (여기에만 S_MOVE 전송!)
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

		// 5. 서버 내 Zone 이동 처리
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
	int32 y = static_cast<int32>(posInfo.z()); // Unity Z = Server Y

	// 범위 초과 방지 (Clamp)
	int32 minX = _map->GetMinX();
	int32 minY = _map->GetMinY();
	int32 maxX = _map->GetMaxX();
	int32 maxY = _map->GetMaxY();

	if (x < minX) x = minX;
	if (x >= maxX) x = maxX - 1;
	if (y < minY) y = minY;
	if (y >= maxY) y = maxY - 1;

	// 인덱스 계산
	int32 zoneX = (x - minX) / _zoneCellSize;
	int32 zoneY = (y - minY) / _zoneCellSize;

	return zoneY * _gridSizeX + zoneX;
}

void GameRoom::GetNearbyZones(int32 zoneIndex, Vector<Zone*>& outZones)
{
	outZones.clear();

	if (zoneIndex < 0 || zoneIndex >= _zones.size())
		return;

	int32 x = zoneIndex % _gridSizeX;
	int32 y = zoneIndex / _gridSizeX;

	// 9-Grid Logic (상하좌우 및 대각선 포함)
	for (int32 dy = -1; dy <= 1; dy++)
	{
		for (int32 dx = -1; dx <= 1; dx++)
		{
			int32 nx = x + dx;
			int32 ny = y + dy;

			// 유효한 Grid 좌표인지 확인
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
	// 전체 유저에게 전송 (채팅 등)
	for (auto& item : _players)
	{
		PlayerRef p = item.second;
		if (p->GetPlayerId() == exceptId) continue;

		p->GetSession()->Send(sendBuffer);
	}
}