#include "pch.h"
#include "ServerPacketHandler.h"
#include "LoadClientManager.h"

PacketHandlerFunc ServerPacketHandler::GPacketHandler[UINT16_MAX];

bool ServerPacketHandler::Handle_INVALID(PacketSessionRef& session, BYTE* buffer, int32 len)
{
	return false;
}

bool ServerPacketHandler::Handle_S_LOGIN(PacketSessionRef& session, Protocol::S_LOGIN& pkt)
{
	auto loginSession = std::dynamic_pointer_cast<LoadLoginSession>(session);
	if (!loginSession)
		return false;

	auto owner = loginSession->GetOwner();
	if (!owner)
		return false;

	owner->OnLoginResponse(pkt, ::GetTickCount64());
	return true;
}

bool ServerPacketHandler::Handle_S_ENTER_GAME(PacketSessionRef& session, Protocol::S_ENTER_GAME& pkt)
{
	auto gameSession = std::dynamic_pointer_cast<LoadGameSession>(session);
	if (!gameSession)
		return false;

	auto owner = gameSession->GetOwner();
	if (!owner)
		return false;

	owner->OnEnterGameResponse(pkt, ::GetTickCount64());
	return true;
}

bool ServerPacketHandler::Handle_S_MOVE(PacketSessionRef& session, Protocol::S_MOVE& pkt)
{
	auto gameSession = std::dynamic_pointer_cast<LoadGameSession>(session);
	if (!gameSession)
		return true;

	auto owner = gameSession->GetOwner();
	if (!owner)
		return true;

	owner->OnMoveAck(pkt, ::GetTickCount64());
	return true;
}

bool ServerPacketHandler::Handle_S_SKILL(PacketSessionRef& session, Protocol::S_SKILL& pkt)
{
	auto gameSession = std::dynamic_pointer_cast<LoadGameSession>(session);
	if (!gameSession)
		return true;

	auto owner = gameSession->GetOwner();
	if (!owner)
		return true;

	owner->OnSkillAck(pkt, ::GetTickCount64());
	return true;
}

bool ServerPacketHandler::Handle_S_HEART_BEAT_RES(PacketSessionRef& session, Protocol::S_HEART_BEAT_RES& pkt)
{
	(void)session;
	(void)pkt;
	return true;
}

bool ServerPacketHandler::Handle_S_SPAWN(PacketSessionRef& session, Protocol::S_SPAWN& pkt)
{
	(void)session;
	(void)pkt;
	return true;
}

bool ServerPacketHandler::Handle_S_DESPAWN(PacketSessionRef& session, Protocol::S_DESPAWN& pkt)
{
	(void)session;
	(void)pkt;
	return true;
}

bool ServerPacketHandler::Handle_S_CHANGE_HP(PacketSessionRef& session, Protocol::S_CHANGE_HP& pkt)
{
	(void)session;
	(void)pkt;
	return true;
}

bool ServerPacketHandler::Handle_S_ITEM_LIST(PacketSessionRef& session, Protocol::S_ITEM_LIST& pkt)
{
	(void)session;
	(void)pkt;
	return true;
}

bool ServerPacketHandler::Handle_S_CHANGE_ITEM(PacketSessionRef& session, Protocol::S_CHANGE_ITEM& pkt)
{
	(void)session;
	(void)pkt;
	return true;
}

bool ServerPacketHandler::Handle_S_REMOVE_ITEM(PacketSessionRef& session, Protocol::S_REMOVE_ITEM& pkt)
{
	(void)session;
	(void)pkt;
	return true;
}

bool ServerPacketHandler::Handle_S_EQUIP_ITEM(PacketSessionRef& session, Protocol::S_EQUIP_ITEM& pkt)
{
	(void)session;
	(void)pkt;
	return true;
}

bool ServerPacketHandler::Handle_S_CHANGE_STAT(PacketSessionRef& session, Protocol::S_CHANGE_STAT& pkt)
{
	(void)session;
	(void)pkt;
	return true;
}

bool ServerPacketHandler::Handle_S_GOLD_UPDATE(PacketSessionRef& session, Protocol::S_GOLD_UPDATE& pkt)
{
	(void)session;
	(void)pkt;
	return true;
}

bool ServerPacketHandler::Handle_S_MAP_CHANGE_BEGIN(PacketSessionRef& session, Protocol::S_MAP_CHANGE_BEGIN& pkt)
{
	(void)session;
	(void)pkt;
	return true;
}

bool ServerPacketHandler::Handle_S_MAP_CHANGE_END(PacketSessionRef& session, Protocol::S_MAP_CHANGE_END& pkt)
{
	(void)session;
	(void)pkt;
	return true;
}

bool ServerPacketHandler::Handle_S_CHAT_RES(PacketSessionRef& session, Protocol::S_CHAT_RES& pkt)
{
	(void)session;
	(void)pkt;
	return true;
}

bool ServerPacketHandler::Handle_S_CHAT_NTF(PacketSessionRef& session, Protocol::S_CHAT_NTF& pkt)
{
	(void)session;
	(void)pkt;
	return true;
}

bool ServerPacketHandler::Handle_S_PARTY_CHAT_NTF(PacketSessionRef& session, Protocol::S_PARTY_CHAT_NTF& pkt)
{
	(void)session;
	(void)pkt;
	return true;
}

bool ServerPacketHandler::Handle_S_PARTY_INFO_NTF(PacketSessionRef& session, Protocol::S_PARTY_INFO_NTF& pkt)
{
	(void)session;
	(void)pkt;
	return true;
}

bool ServerPacketHandler::Handle_S_PARTY_RESULT(PacketSessionRef& session, Protocol::S_PARTY_RESULT& pkt)
{
	(void)session;
	(void)pkt;
	return true;
}

bool ServerPacketHandler::Handle_S_PARTY_INVITE_NTF(PacketSessionRef& session, Protocol::S_PARTY_INVITE_NTF& pkt)
{
	(void)session;
	(void)pkt;
	return true;
}

bool ServerPacketHandler::Handle_S_PARTY_STATUS_NTF(PacketSessionRef& session, Protocol::S_PARTY_STATUS_NTF& pkt)
{
	(void)session;
	(void)pkt;
	return true;
}

bool ServerPacketHandler::Handle_S_DUNGEON_ENTER_RES(PacketSessionRef& session, Protocol::S_DUNGEON_ENTER_RES& pkt)
{
	(void)session;
	(void)pkt;
	return true;
}

bool ServerPacketHandler::Handle_S_DUNGEON_EXIT_RES(PacketSessionRef& session, Protocol::S_DUNGEON_EXIT_RES& pkt)
{
	(void)session;
	(void)pkt;
	return true;
}

bool ServerPacketHandler::Handle_S_QUICKSLOT_LIST(PacketSessionRef& session, Protocol::S_QUICKSLOT_LIST& pkt)
{
	(void)session;
	(void)pkt;
	return true;
}

bool ServerPacketHandler::Handle_S_SET_QUICKSLOT(PacketSessionRef& session, Protocol::S_SET_QUICKSLOT& pkt)
{
	(void)session;
	(void)pkt;
	return true;
}

bool ServerPacketHandler::Handle_S_TRADE_INVITE(PacketSessionRef& session, Protocol::S_TRADE_INVITE& pkt)
{
	(void)session;
	(void)pkt;
	return true;
}

bool ServerPacketHandler::Handle_S_TRADE_START(PacketSessionRef& session, Protocol::S_TRADE_START& pkt)
{
	(void)session;
	(void)pkt;
	return true;
}

bool ServerPacketHandler::Handle_S_TRADE_OFFER_UPDATE(PacketSessionRef& session, Protocol::S_TRADE_OFFER_UPDATE& pkt)
{
	(void)session;
	(void)pkt;
	return true;
}

bool ServerPacketHandler::Handle_S_TRADE_READY_STATE(PacketSessionRef& session, Protocol::S_TRADE_READY_STATE& pkt)
{
	(void)session;
	(void)pkt;
	return true;
}

bool ServerPacketHandler::Handle_S_TRADE_LOCKED(PacketSessionRef& session, Protocol::S_TRADE_LOCKED& pkt)
{
	(void)session;
	(void)pkt;
	return true;
}

bool ServerPacketHandler::Handle_S_TRADE_CANCELLED(PacketSessionRef& session, Protocol::S_TRADE_CANCELLED& pkt)
{
	(void)session;
	(void)pkt;
	return true;
}

bool ServerPacketHandler::Handle_S_TRADE_RESULT(PacketSessionRef& session, Protocol::S_TRADE_RESULT& pkt)
{
	(void)session;
	(void)pkt;
	return true;
}
