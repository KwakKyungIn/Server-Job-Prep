#include "pch.h"
#include "ClientPacketHandler.h"

PacketHandlerFunc ClientPacketHandler::GPacketHandler[UINT16_MAX];

bool ClientPacketHandler::Handle_INVALID(PacketSessionRef& session, BYTE* buffer, int32 len)
{
	return false;
}