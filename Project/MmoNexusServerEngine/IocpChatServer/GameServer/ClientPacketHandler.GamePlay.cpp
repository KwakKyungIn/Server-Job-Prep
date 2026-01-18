#include "pch.h"
#include "ClientPacketHandler.h"
#include "PlayerSession.h"
#include "GameRoom.h" 

bool ClientPacketHandler::Handle_C_MOVE(PacketSessionRef& session, Protocol::C_MOVE& pkt)
{
	PlayerSessionRef ps = static_pointer_cast<PlayerSession>(session);
	if (!ps) return false;

	if (ps->IsMapChanging())
		return true;

	const uint64 playerId = ps->GetPlayerId_AnyThread();
	if (playerId == 0)
		return true;

	ps->PostRoom([playerId, pkt](PlayerSessionRef self, RoomActorRef room) mutable
		{
			if (!room) return;
			if (self->IsMapChanging()) return;
			if (room->GetKind() != RoomKind::Game) return;

			auto gr = std::static_pointer_cast<GameRoom>(room);
			gr->Push([gr, self, playerId, pkt]() mutable
				{
					gr->HandleMoveById(self, playerId, pkt);
				});
		});

	return true;
}

bool ClientPacketHandler::Handle_C_EQUIP_ITEM(PacketSessionRef& session, Protocol::C_EQUIP_ITEM& pkt)
{
	PlayerSessionRef ps = static_pointer_cast<PlayerSession>(session);
	if (!ps) return false;

	if (ps->IsMapChanging())
		return true;

	const uint64 playerId = ps->GetPlayerId_AnyThread();
	if (playerId == 0)
		return true;

	ps->PostRoom([playerId, pkt](PlayerSessionRef self, RoomActorRef room) mutable
		{
			if (!room) return;
			if (self->IsMapChanging()) return;
			if (room->GetKind() != RoomKind::Game) return;

			auto gr = std::static_pointer_cast<GameRoom>(room);
			gr->Push([gr, self, playerId, pkt]() mutable
				{
					gr->HandleEquipItemById(self, playerId, pkt);
				});
		});

	return true;
}

bool ClientPacketHandler::Handle_C_USE_ITEM(PacketSessionRef& session, Protocol::C_USE_ITEM& pkt)
{
	PlayerSessionRef ps = static_pointer_cast<PlayerSession>(session);
	if (!ps) return false;

	if (ps->IsMapChanging())
		return true;

	const uint64 playerId = ps->GetPlayerId_AnyThread();
	if (playerId == 0)
		return true;

	ps->PostRoom([playerId, pkt](PlayerSessionRef self, RoomActorRef room) mutable
		{
			if (!room) return;
			if (self->IsMapChanging()) return;
			if (room->GetKind() != RoomKind::Game) return;

			auto gr = std::static_pointer_cast<GameRoom>(room);
			gr->Push([gr, self, playerId, pkt]() mutable
				{
					gr->HandleUseItemById(self, playerId, pkt);
				});
		});

	return true;
}

bool ClientPacketHandler::Handle_C_SKILL(PacketSessionRef& session, Protocol::C_SKILL& pkt)
{
	PlayerSessionRef ps = static_pointer_cast<PlayerSession>(session);
	if (!ps) return true;

	if (ps->IsMapChanging())
		return true;

	const int32 skillId = pkt.skillid();
	const float castYaw = pkt.castyaw();
	const uint32 clientTimeMs = pkt.client_time_ms();

	const uint64 playerId = ps->GetPlayerId_AnyThread();
	if (playerId == 0)
		return true;

	ps->PostRoom([playerId, skillId, castYaw, clientTimeMs](PlayerSessionRef self, RoomActorRef room) mutable
		{
			if (!room) return;
			if (self->IsMapChanging()) return;
			if (room->GetKind() != RoomKind::Game) return;

			auto gr = std::static_pointer_cast<GameRoom>(room);
			gr->Push([gr, self, playerId, skillId, castYaw, clientTimeMs]()
				{
					//  NEW: HandleSkillById 시그니처 확장 필요
					gr->HandleSkillById(self, playerId, skillId, castYaw, clientTimeMs);
				});
		});

	return true;
}

bool ClientPacketHandler::Handle_C_CHAT_REQ(PacketSessionRef& session, Protocol::C_CHAT_REQ& pkt)
{
	PlayerSessionRef ps = static_pointer_cast<PlayerSession>(session);
	if (!ps) return true;

	if (ps->IsMapChanging())
		return true;

	const std::string msg = pkt.message();
	const uint64 playerId = ps->GetPlayerId_AnyThread();
	if (playerId == 0)
		return true;

	ps->PostRoom([playerId, msg](PlayerSessionRef self, RoomActorRef room) mutable
		{
			if (!room) return;
			if (self->IsMapChanging()) return;
			if (room->GetKind() != RoomKind::Game) return;

			auto gr = std::static_pointer_cast<GameRoom>(room);
			gr->Push([gr, self, playerId, msg]() mutable
				{
					gr->HandleChatById(self, playerId, msg);
				});
		});

	return true;
}
