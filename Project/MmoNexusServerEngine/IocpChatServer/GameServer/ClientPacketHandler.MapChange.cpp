#include "pch.h"
#include "ClientPacketHandler.h"
#include "PlayerSession.h"
#include "GameRoom.h" 
#include "DataManager.h"
#include "ClientPacketHandler.MapChangeUtil.h"

bool ClientPacketHandler::Handle_C_MAP_CHANGE_REQ(PacketSessionRef& session, Protocol::C_MAP_CHANGE_REQ& pkt)
{
	PlayerSessionRef ps = static_pointer_cast<PlayerSession>(session);
	if (!ps) return false;

	if (ps->IsMapChanging())
		return true;

	const uint64 playerId = ps->GetPlayerId_AnyThread();
	if (playerId == 0)
		return true;

	const int32 targetMapId = pkt.targetmapid();
	const int64 targetInstanceId = 0; // 월드 이동은 0

	DataManager* dm = DataManager::Instance();

	if (!dm || !dm->IsValidMapId(targetMapId) || !dm->IsWorldMapId(targetMapId))
	{
		std::cout << "❌ [MapChange] rejected. targetMapId=" << targetMapId << std::endl;
		return true;
	}

	const MapConfig* cfg = dm->GetMapConfig(targetMapId);
	if (!cfg)
		return true;

	Protocol::PositionInfo spawn;
	spawn.set_x(cfg->spawnX);
	spawn.set_y(cfg->spawnY);
	spawn.set_z(cfg->spawnZ);

	ps->Post([playerId, targetMapId, targetInstanceId, spawn](PlayerSessionRef self) mutable
		{
			if (self->IsMapChanging())
				return;

			const uint64 token = MapChangeUtil::MakeMapChangeToken(playerId, self->GetSessionId());
			if (!self->TryBeginMapChange(token, targetMapId, targetInstanceId, spawn))
				return;

			Protocol::S_MAP_CHANGE_BEGIN beginPkt;
			beginPkt.set_token(token);
			beginPkt.set_targetmapid(targetMapId);
			beginPkt.mutable_spawn()->CopyFrom(spawn);
			beginPkt.set_instanceid(targetInstanceId);

			self->Send(MakeSendBuffer(beginPkt));
		});

	return true;
}

bool ClientPacketHandler::Handle_C_MAP_CHANGE_ACK(PacketSessionRef& session, Protocol::C_MAP_CHANGE_ACK& pkt)
{
	PlayerSessionRef ps = static_pointer_cast<PlayerSession>(session);
	if (!ps) return false;

	const uint64 token = pkt.token();

	ps->Post([token](PlayerSessionRef self)
		{
			int32 targetMapId = 0;
			int64 targetInstanceId = 0;
			Protocol::PositionInfo spawn;

			if (!self->TryConsumeMapChangeAck(token, targetMapId, targetInstanceId, spawn))
				return;

			const uint64 playerId = self->GetPlayerId_AnyThread();
			if (playerId == 0)
			{
				self->CancelMapChange();
				return;
			}

			auto room = self->GetCurrentRoom_ActorOnly();
			if (!room || room->GetKind() != RoomKind::Game)
			{
				self->CancelMapChange();
				return;
			}

			auto gr = std::static_pointer_cast<GameRoom>(room);
			gr->Push([gr, self, playerId, targetMapId, targetInstanceId, spawn]() mutable
				{
					gr->TransferMapChangeById(self, playerId, targetMapId, targetInstanceId, spawn);
				});
		});

	return true;
}
