#include "pch.h"
#include "ClientPacketHandler.h"
#include "PlayerSession.h"
#include "GameRoom.h" 
#include "DataManager.h"
#include "ClientPacketHandler.MapChangeUtil.h"
#include "Player.h"

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

			int32 targetChannelId = 1;
			if (auto room = self->GetCurrentRoom_ActorOnly())
			{
				if (room->GetKind() == RoomKind::Game)
				{
					auto gr = std::static_pointer_cast<GameRoom>(room);
					targetChannelId = gr->GetChannelId();
				}
			}

			const uint64 token = MapChangeUtil::MakeMapChangeToken(playerId, self->GetSessionId());
			if (!self->TryBeginMapChange(token, targetChannelId, targetMapId, targetInstanceId, spawn))
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
			int32 targetChannelId = 0;
			int32 targetMapId = 0;
			int64 targetInstanceId = 0;
			Protocol::PositionInfo spawn;

			if (!self->TryConsumeMapChangeAck(token, targetChannelId, targetMapId, targetInstanceId, spawn))
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
			gr->Push([gr, self, playerId, targetChannelId, targetMapId, targetInstanceId, spawn]() mutable
				{
					gr->TransferMapChangeById(self, playerId, targetChannelId, targetMapId, targetInstanceId, spawn);
				});
		});

	return true;
}
bool ClientPacketHandler::Handle_C_CHANNEL_CHANGE_REQ(PacketSessionRef& session, Protocol::C_CHANNEL_CHANGE_REQ& pkt)
{
	PlayerSessionRef ps = static_pointer_cast<PlayerSession>(session);
	if (!ps) return false;

	if (ps->IsMapChanging())
		return true;

	const uint64 playerId = ps->GetPlayerId_AnyThread();
	if (playerId == 0)
		return true;

	const int32 targetChannelId = pkt.targetchannelid();
	if (targetChannelId <= 0)
		return true;

	ps->Post([playerId, targetChannelId](PlayerSessionRef self) mutable
		{
			if (self->IsMapChanging())
				return;

			auto room = self->GetCurrentRoom_ActorOnly();
			if (!room || room->GetKind() != RoomKind::Game)
				return;

			auto gr = std::static_pointer_cast<GameRoom>(room);

			// 같은 채널이면 무시
			const int32 curChannelId = gr->GetChannelId();
			if (targetChannelId == curChannelId)
				return;

			// 인스턴스(던전)면 일단 막자 (정책 바꾸고 싶으면 여기 수정)
			if (gr->IsInstanceRoom())
				return;

			// Room thread에서 현재 위치/맵을 스냅샷
			gr->Push([gr, self, playerId, targetChannelId]() mutable
				{
					PlayerRef p = gr->FindPlayer_ActorOnly(playerId);
					if (!p) return;

					const int32 targetMapId = p->GetMapId();
					const int64 targetInstanceId = 0;

					Protocol::PositionInfo spawn;
					if (auto pos = p->GetPosInfo())
						spawn.CopyFrom(*pos);

					self->Post([playerId, targetChannelId, targetMapId, targetInstanceId, spawn](PlayerSessionRef s) mutable
						{
							if (s->IsMapChanging())
								return;

							const uint64 token = MapChangeUtil::MakeMapChangeToken(playerId, s->GetSessionId());
							if (!s->TryBeginMapChange(token, targetChannelId, targetMapId, targetInstanceId, spawn))
								return;

							Protocol::S_MAP_CHANGE_BEGIN beginPkt;
							beginPkt.set_token(token);
							beginPkt.set_targetmapid(targetMapId);
							beginPkt.mutable_spawn()->CopyFrom(spawn);
							beginPkt.set_instanceid(targetInstanceId);
							beginPkt.set_targetchannelid(targetChannelId);

							s->Send(ClientPacketHandler::MakeSendBuffer(beginPkt));
						});
				});
		});

	return true;
}