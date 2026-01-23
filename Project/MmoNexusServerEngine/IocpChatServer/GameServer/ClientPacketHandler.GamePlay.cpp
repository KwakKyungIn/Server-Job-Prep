#include "pch.h"
#include "ClientPacketHandler.h"
#include "PlayerSession.h"
#include "GameRoom.h" 

// 클라이언트 이동 패킷 처리
// 세션에서 룸 액터로 이동 로직을 넘겨서 처리한다
bool ClientPacketHandler::Handle_C_MOVE(PacketSessionRef& session, Protocol::C_MOVE& pkt)
{
	PlayerSessionRef ps = static_pointer_cast<PlayerSession>(session);
	if (!ps) return false;

	if (ps->IsMapChanging())
		return true;

	const uint64 playerId = ps->GetPlayerId_AnyThread();
	if (playerId == 0)
		return true;

	// 플레이어가 속한 룸으로 작업을 포스팅하여 동기화 문제 해결
	ps->PostRoom([playerId, pkt](PlayerSessionRef self, RoomActorRef room) mutable
		{
			if (!room) return;
			if (self->IsMapChanging()) return;
			// 게임 룸이 아니면 이동 처리를 하지 않음 (로비 등)
			if (room->GetKind() != RoomKind::Game) return;

			auto gr = std::static_pointer_cast<GameRoom>(room);
			gr->Push([gr, self, playerId, pkt]() mutable
				{
					gr->HandleMoveById(self, playerId, pkt);
				});
		});

	return true;
}

// 아이템 장착 요청 핸들러
bool ClientPacketHandler::Handle_C_EQUIP_ITEM(PacketSessionRef& session, Protocol::C_EQUIP_ITEM& pkt)
{
	PlayerSessionRef ps = static_pointer_cast<PlayerSession>(session);
	if (!ps) return false;

	if (ps->IsMapChanging())
		return true;

	const uint64 playerId = ps->GetPlayerId_AnyThread();
	if (playerId == 0)
		return true;

	// 룸 액터 큐에 작업을 넣어서 순차적으로 처리되게 함
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

// 소비 아이템 등 아이템 사용 요청 핸들러
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

// 인벤토리 내 아이템 드래그 앤 드롭 (순서 변경) 요청 핸들러
bool ClientPacketHandler::Handle_C_INV_DRAG_DROP(PacketSessionRef& session, Protocol::C_INV_DRAG_DROP& pkt)
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
					gr->HandleInvDragDropById(self, playerId, pkt);
				});
		});

	return true;
}

// 스킬 사용 요청 핸들러
// 스킬 ID와 시전 방향, 클라이언트 시간 등을 받아서 처리함
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

	// 전투 로직도 룸 액터 안에서 안전하게 돌아가야 함
	ps->PostRoom([playerId, skillId, castYaw, clientTimeMs](PlayerSessionRef self, RoomActorRef room) mutable
		{
			if (!room) return;
			if (self->IsMapChanging()) return;
			if (room->GetKind() != RoomKind::Game) return;

			auto gr = std::static_pointer_cast<GameRoom>(room);
			gr->Push([gr, self, playerId, skillId, castYaw, clientTimeMs]()
				{
					// 스킬 처리 함수 호출 (쿨타임 검사 등은 내부에서 수행)
					gr->HandleSkillById(self, playerId, skillId, castYaw, clientTimeMs);
				});
		});

	return true;
}

// 채팅 메시지 전송 요청 핸들러
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

	// 채팅도 같은 방에 있는 유저들에게 뿌려야 하므로 룸 액터로 보냄
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