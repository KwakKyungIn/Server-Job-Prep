#include "pch.h"
#include "ClientPacketHandler.h"

PacketHandlerFunc ClientPacketHandler::GPacketHandler[UINT16_MAX];

// 정의되지 않은 패킷이 들어왔을 때 처리하는 예외 핸들러
// 일단은 그냥 false 리턴하고 무시하게 해둠
bool ClientPacketHandler::Handle_INVALID(PacketSessionRef& session, BYTE* buffer, int32 len)
{
	return false;
}