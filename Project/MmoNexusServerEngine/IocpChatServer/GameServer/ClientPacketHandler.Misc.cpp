#include "pch.h"
#include "ClientPacketHandler.h"

bool ClientPacketHandler::Handle_C_HEART_BEAT_REQ(PacketSessionRef& session, Protocol::C_HEART_BEAT_REQ& pkt)
{
	return true;
}
