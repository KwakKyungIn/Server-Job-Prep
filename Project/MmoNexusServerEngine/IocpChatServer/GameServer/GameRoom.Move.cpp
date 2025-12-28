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
#include "GameRoom.Net.h"

void GameRoom::HandleMove(PlayerSessionRef session, PlayerRef player, Protocol::C_MOVE pkt)
{

	if (player == nullptr) return;

	uint64 playerId = player->GetPlayerId();
	if (_players.find(playerId) == _players.end()) return;

	printf("[GameRoom::HandleMove] Player %llu Move -> (%.1f,%.1f,%.1f)\n",
		playerId,
		pkt.posinfo().x(), pkt.posinfo().y(), pkt.posinfo().z());


	// 1. [Validation] 맵 충돌 체크
	if (_map->CanGo(pkt.posinfo()) == false)
		return;

	// 2. [Zone Check] AOI 그리드 기준
	int32 oldZoneIndex = player->GetZoneIndex();
	int32 newZoneIndex = _grid.GetZoneIndex(pkt.posinfo());

	// 3. [Update] 위치 정보 갱신
	player->SetPosInfo(pkt.posinfo());

	// [Case A] 같은 Zone 내 이동
	if (oldZoneIndex == newZoneIndex)
	{
		Protocol::S_MOVE movePkt;
		movePkt.set_objectid(playerId);
		*movePkt.mutable_posinfo() = pkt.posinfo();
		SendBufferRef sendBuffer = ClientPacketHandler::MakeSendBuffer(movePkt);


		printf("📢 [HandleMove] Broadcasting to Zone[%d], except Player %llu\n",
			newZoneIndex, playerId);  // ← 추가


		BroadcastToZone(sendBuffer, newZoneIndex, playerId);

		printf("✅ [HandleMove] Broadcast complete\n");  // ← 추가
	}
	// [Case B] Zone 변경 발생
	else
	{
		Vector<int32> oldZones;
		_grid.GetNearbyZoneIndices(oldZoneIndex, oldZones);
		std::sort(oldZones.begin(), oldZones.end());

		Vector<int32> newZones;
		_grid.GetNearbyZoneIndices(newZoneIndex, newZones);
		std::sort(newZones.begin(), newZones.end());

		// (Old - New) : Despawn Group (사라져야 할 놈들)
		{
			Vector<int32> removedZones;
			std::set_difference(oldZones.begin(), oldZones.end(),
				newZones.begin(), newZones.end(),
				std::back_inserter(removedZones));

			// 나 -> 다른 사람들에게 "나 사라짐" 알림
			Protocol::S_DESPAWN despawnPkt;
			despawnPkt.add_objectids(playerId);
			SendBufferRef sendBuffer = ClientPacketHandler::MakeSendBuffer(despawnPkt);

			// 나에게 "너네 사라짐" 알림
			Protocol::S_DESPAWN despawnToMePkt;

			for (int32 zoneIdx : removedZones)
			{
				Zone& zone = _grid.GetZone(zoneIdx);

				// 해당 존에 있는 플레이어 처리
				for (auto& p : zone.players)
				{
					if (p->GetPlayerId() != playerId)
					{
						SendToPlayer(p->GetPlayerId(), sendBuffer);              // 걔네한테 내 정보 삭제
						despawnToMePkt.add_objectids(p->GetPlayerId()); // 내 목록에서 걔네 삭제
					}
				}
				// 몬스터 처리
				for (auto& m : zone.monsters)
				{
					despawnToMePkt.add_objectids(m->GetObjectId());
				}
			}

			if (despawnToMePkt.objectids_size() > 0)
			{
				SendBufferRef despawnToMeBuffer = ClientPacketHandler::MakeSendBuffer(despawnToMePkt);
				session->Send(despawnToMeBuffer);
			}
		}

		// (New - Old) : Spawn Group (새로 나타날 놈들)
		{
			Vector<int32> addedZones;
			std::set_difference(newZones.begin(), newZones.end(),
				oldZones.begin(), oldZones.end(),
				std::back_inserter(addedZones));

			// 나 -> 다른 사람들에게 "나 나타남"
			Protocol::S_SPAWN mySpawnPkt;
			auto* myInfo = mySpawnPkt.add_players();
			*myInfo = *player->GetPlayerInfo();
			SendBufferRef mySpawnBuffer = ClientPacketHandler::MakeSendBuffer(mySpawnPkt);

			// 나에게 "너네 나타남" (플레이어 + 몬스터)
			Protocol::S_SPAWN othersSpawnPkt;

			for (int32 zoneIdx : addedZones)
			{
				Zone& zone = _grid.GetZone(zoneIdx);

				// 플레이어 처리
				for (auto& p : zone.players)
				{
					if (p->GetPlayerId() != playerId)
					{
						SendToPlayer(p->GetPlayerId(), mySpawnBuffer); // 걔네에게 나를 보냄
						auto* otherInfo = othersSpawnPkt.add_players();
						*otherInfo = *p->GetPlayerInfo();
					}
				}
				// 몬스터 처리
				for (auto& m : zone.monsters)
				{
					auto* mInfo = othersSpawnPkt.add_monsters();
					*mInfo = *m->GetMonsterInfo();
				}
			}

			if (othersSpawnPkt.players_size() > 0 || othersSpawnPkt.monsters_size() > 0)
			{
				SendBufferRef othersSpawnBuffer = ClientPacketHandler::MakeSendBuffer(othersSpawnPkt);
				session->Send(othersSpawnBuffer);
			}
		}

		// (Old ∩ New) : Move Group (같이 이동 중인 놈들)
		{
			Vector<int32> commonZones;
			std::set_intersection(oldZones.begin(), oldZones.end(),
				newZones.begin(), newZones.end(),
				std::back_inserter(commonZones));

			Protocol::S_MOVE movePkt;
			movePkt.set_objectid(playerId);
			*movePkt.mutable_posinfo() = pkt.posinfo();
			SendBufferRef sendBuffer = ClientPacketHandler::MakeSendBuffer(movePkt);

			for (int32 zoneIdx : commonZones)
			{
				Zone& zone = _grid.GetZone(zoneIdx);
				for (auto& p : zone.players)
				{
					if (p->GetPlayerId() != playerId)
						SendToPlayer(p->GetPlayerId(), sendBuffer);
				}
			}
		}

		// 서버 내 Zone 이동 반영
		{
			Zone& oldZone = _grid.GetZone(oldZoneIndex);
			Zone& newZone = _grid.GetZone(newZoneIndex);

			oldZone.players.erase(player);
			newZone.players.insert(player);
			player->SetZoneIndex(newZoneIndex);
		}
	}
}

void GameRoom::HandleMoveById(PlayerSessionRef session, uint64 playerId, Protocol::C_MOVE pkt)
{
	auto it = _players.find(playerId);
	if (it == _players.end())
		return;

	HandleMove(session, it->second, pkt); // 기존 로직 재사용
}
