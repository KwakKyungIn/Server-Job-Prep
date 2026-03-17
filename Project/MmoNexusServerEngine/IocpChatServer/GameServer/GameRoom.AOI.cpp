#include "pch.h"
#include "GameRoom.h"
#include "ClientPacketHandler.h"
#include "ExperimentUtils.h"
#include "GameMetrics.h"
#include "Player.h"
#include "PlayerSession.h"
#include "Monster.h"
#include "Projectile.h"
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

void GameRoom::BuildRoomWideVisibilityForPlayer_ActorOnly(PlayerRef player)
{
	if (!player)
		return;

	const uint64 meId = player->GetPlayerId();
	auto& visPlayers = player->VisiblePlayers_ActorOnly();
	auto& visMonsters = player->VisibleMonsters_ActorOnly();
	auto& visProjectiles = player->VisibleProjectiles_ActorOnly();

	visPlayers.clear();
	visMonsters.clear();
	visProjectiles.clear();

	for (auto& item : _players)
	{
		PlayerRef other = item.second;
		if (!other)
			continue;

		const uint64 otherId = other->GetPlayerId();
		if (otherId == meId)
			continue;

		visPlayers.insert(otherId);
		other->VisiblePlayers_ActorOnly().insert(meId);
	}

	for (auto& item : _monsters)
	{
		MonsterRef monster = item.second;
		if (!monster)
			continue;

		visMonsters.insert(monster->GetObjectId());
		monster->Viewers_ActorOnly().insert(meId);
	}

	for (auto& item : _projectiles)
	{
		ProjectileRef projectile = item.second;
		if (!projectile)
			continue;

		visProjectiles.insert(projectile->GetObjectId());
		projectile->Viewers_ActorOnly().insert(meId);
	}
}

void GameRoom::ClearRoomWideVisibilityForPlayer_ActorOnly(PlayerRef player)
{
	if (!player)
		return;

	const uint64 meId = player->GetPlayerId();

	for (auto& item : _players)
	{
		PlayerRef other = item.second;
		if (!other || other == player)
			continue;

		other->VisiblePlayers_ActorOnly().erase(meId);
	}

	for (auto& item : _monsters)
	{
		MonsterRef monster = item.second;
		if (!monster)
			continue;

		monster->Viewers_ActorOnly().erase(meId);
	}

	for (auto& item : _projectiles)
	{
		ProjectileRef projectile = item.second;
		if (!projectile)
			continue;

		projectile->Viewers_ActorOnly().erase(meId);
	}

	player->VisiblePlayers_ActorOnly().clear();
	player->VisibleMonsters_ActorOnly().clear();
	player->VisibleProjectiles_ActorOnly().clear();
}

void GameRoom::SendRoomWideSnapshotToPlayer_ActorOnly(PlayerSessionRef session, PlayerRef player, bool snapshotMode)
{
	if (!session || !player)
		return;

	Vector<PlayerRef> snapshotPlayers;
	Vector<MonsterRef> snapshotMonsters;
	Vector<ProjectileRef> snapshotProjectiles;

	snapshotPlayers.reserve((_players.size() > 0) ? (_players.size() - 1) : 0);
	snapshotMonsters.reserve(_monsters.size());
	snapshotProjectiles.reserve(_projectiles.size());

	const uint64 meId = player->GetPlayerId();
	for (auto& item : _players)
	{
		PlayerRef other = item.second;
		if (!other || other->GetPlayerId() == meId)
			continue;

		snapshotPlayers.push_back(other);
	}

	for (auto& item : _monsters)
	{
		if (item.second)
			snapshotMonsters.push_back(item.second);
	}

	for (auto& item : _projectiles)
	{
		if (item.second)
			snapshotProjectiles.push_back(item.second);
	}

	if (!snapshotMode)
	{
		SendSpawnBatchedToMe(session, snapshotPlayers, snapshotMonsters, false, 0);

		if (!snapshotProjectiles.empty())
		{
			const int32 batchProj = 64;
			for (int32 t = 0; t < static_cast<int32>(snapshotProjectiles.size()); )
			{
				Protocol::S_SPAWN pkt;
				const int32 take = min(batchProj, static_cast<int32>(snapshotProjectiles.size()) - t);
				for (int32 k = 0; k < take; ++k)
				{
					auto* info = pkt.add_projectiles();
					*info = *snapshotProjectiles[t + k]->GetProjectileInfo();
				}

				t += take;
				session->Send(ClientPacketHandler::MakeSendBuffer(pkt));
			}
		}

		return;
	}

	const uint32 snapshotId = player->NextSnapshotSeq_ActorOnly();
	Vector<Protocol::S_SPAWN> pkts;
	pkts.reserve(
		(snapshotPlayers.size() + _batchSpawnPlayers - 1) / _batchSpawnPlayers +
		(snapshotMonsters.size() + _batchSpawnMonsters - 1) / _batchSpawnMonsters +
		(snapshotProjectiles.size() + 63) / 64 +
		1);

	for (int32 i = 0; i < static_cast<int32>(snapshotPlayers.size()); )
	{
		Protocol::S_SPAWN pkt;
		pkt.set_snapshot_id(snapshotId);
		pkt.set_snapshot_begin(false);
		pkt.set_snapshot_end(false);

		const int32 take = min(_batchSpawnPlayers, static_cast<int32>(snapshotPlayers.size()) - i);
		for (int32 k = 0; k < take; ++k)
		{
			auto* info = pkt.add_players();
			*info = *snapshotPlayers[i + k]->GetPlayerInfo();
		}

		i += take;
		pkts.push_back(std::move(pkt));
	}

	for (int32 j = 0; j < static_cast<int32>(snapshotMonsters.size()); )
	{
		Protocol::S_SPAWN pkt;
		pkt.set_snapshot_id(snapshotId);
		pkt.set_snapshot_begin(false);
		pkt.set_snapshot_end(false);

		const int32 take = min(_batchSpawnMonsters, static_cast<int32>(snapshotMonsters.size()) - j);
		for (int32 k = 0; k < take; ++k)
		{
			auto* info = pkt.add_monsters();
			*info = *snapshotMonsters[j + k]->GetMonsterInfo();
		}

		j += take;
		pkts.push_back(std::move(pkt));
	}

	const int32 batchProj = 64;
	for (int32 t = 0; t < static_cast<int32>(snapshotProjectiles.size()); )
	{
		Protocol::S_SPAWN pkt;
		pkt.set_snapshot_id(snapshotId);
		pkt.set_snapshot_begin(false);
		pkt.set_snapshot_end(false);

		const int32 take = min(batchProj, static_cast<int32>(snapshotProjectiles.size()) - t);
		for (int32 k = 0; k < take; ++k)
		{
			auto* info = pkt.add_projectiles();
			*info = *snapshotProjectiles[t + k]->GetProjectileInfo();
		}

		t += take;
		pkts.push_back(std::move(pkt));
	}

	if (pkts.empty())
	{
		Protocol::S_SPAWN emptyPkt;
		emptyPkt.set_snapshot_id(snapshotId);
		emptyPkt.set_snapshot_begin(true);
		emptyPkt.set_snapshot_end(true);
		session->Send(ClientPacketHandler::MakeSendBuffer(emptyPkt));
		return;
	}

	pkts.front().set_snapshot_begin(true);
	pkts.back().set_snapshot_end(true);

	for (auto& pkt : pkts)
		session->Send(ClientPacketHandler::MakeSendBuffer(pkt));
}

void GameRoom::BroadcastRoomWidePlayerSpawn_ActorOnly(PlayerRef player)
{
	if (!player)
		return;

	Protocol::S_SPAWN pkt;
	auto* info = pkt.add_players();
	*info = *player->GetPlayerInfo();
	SendBufferRef sb = ClientPacketHandler::MakeSendBuffer(pkt);

	const int32 recipients = Broadcast(sb, player->GetPlayerId());
	GameMetrics::OnBroadcastRecipients(
		GameMetrics::HotRoomBroadcastKind::Spawn,
		GameMetrics::HotRoomBroadcastMode::Room,
		static_cast<std::size_t>(recipients));
}

int32 GameRoom::BroadcastToZone(SendBufferRef sendBuffer, int32 zoneIndex, uint64 exceptId)
{
	Vector<Zone*> nearbyZones;
	_grid.GetNearbyZones(zoneIndex, nearbyZones); 
	int32 recipients = 0;

	for (Zone* zone : nearbyZones)
	{
		for (const PlayerRef& p : zone->players)
		{
			if (p->GetPlayerId() == exceptId) continue;
			SendToPlayer(p->GetPlayerId(), sendBuffer);
			++recipients;
		}
	}

	return recipients;
}

int32 GameRoom::Broadcast(SendBufferRef sendBuffer, uint64 exceptId)
{
	int32 recipients = 0;
	for (auto& item : _players)
	{
		PlayerRef p = item.second;
		if (!p) continue;
		if (p->GetPlayerId() == exceptId) continue;
		SendToPlayer(p->GetPlayerId(), sendBuffer);
		++recipients;
	}

	return recipients;
}
