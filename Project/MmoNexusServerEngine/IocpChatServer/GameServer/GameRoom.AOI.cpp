#include "pch.h"
#include "GameRoom.h"
#include "Player.h"
#include "PlayerSession.h"
#include "Monster.h"
#include "RoomManager.h"
#include "GameRoom.Net.h"

void GameRoom::SendEnterSpawns(PlayerSessionRef session, PlayerRef player)
{
	if (player == nullptr) return;

	const int32 zoneIndex = player->GetZoneIndex();

	// 1) 주변 플레이어들에게 "나 등장" 브로드캐스트
	{
		Protocol::S_SPAWN spawnPkt;
		Protocol::PlayerInfo* pInfo = spawnPkt.add_players();
		*pInfo = *player->GetPlayerInfo();
		SendBufferRef sendBuffer = ClientPacketHandler::MakeSendBuffer(spawnPkt);

		Vector<Zone*> nearbyZones;
		_grid.GetNearbyZones(zoneIndex, nearbyZones);

		for (Zone* zone : nearbyZones)
		{
			for (const PlayerRef& other : zone->players)
			{
				if (other != player)
					SendToPlayer(other->GetPlayerId(), sendBuffer);
			}
		}
	}

	// 2) 나에게 주변 정보 스폰
	{
		Vector<Zone*> nearbyZones;
		_grid.GetNearbyZones(zoneIndex, nearbyZones);

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

			for (const MonsterRef& monster : zone->monsters)
			{
				Protocol::MonsterInfo* mInfo = spawnPkt.add_monsters();
				*mInfo = *monster->GetMonsterInfo();
			}
		}

		if (spawnPkt.players_size() > 0 || spawnPkt.monsters_size() > 0)
		{
			SendBufferRef sendBuffer = ClientPacketHandler::MakeSendBuffer(spawnPkt);
			session->Send(sendBuffer);
		}
	}
}

void GameRoom::BroadcastToZone(SendBufferRef sendBuffer, int32 zoneIndex, uint64 exceptId)
{
	Vector<Zone*> nearbyZones;
	_grid.GetNearbyZones(zoneIndex, nearbyZones); 

	for (Zone* zone : nearbyZones)
	{
		for (const PlayerRef& p : zone->players)
		{
			if (p->GetPlayerId() == exceptId) continue;
			SendToPlayer(p->GetPlayerId(), sendBuffer);
		}
	}
}

void GameRoom::Broadcast(SendBufferRef sendBuffer, uint64 exceptId)
{
	for (auto& item : _players)
	{
		PlayerRef p = item.second;
		if (!p) continue;
		if (p->GetPlayerId() == exceptId) continue;
		SendToPlayer(p->GetPlayerId(), sendBuffer);
	}
}
