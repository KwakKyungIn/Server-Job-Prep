#include "pch.h"
#include "S2SPacketHandler.h"

// [Definition]
PacketHandlerFunc S2SPacketHandler::GPacketHandler[UINT16_MAX];

bool S2SPacketHandler::Handle_INVALID(PacketSessionRef& session, BYTE* buffer, int32 len)
{
    return false;
}

bool S2SPacketHandler::Handle_S2S_RES_LOGIN(PacketSessionRef& session, Protocol::S2S_RES_LOGIN& pkt)
{
    return true;
}

bool S2SPacketHandler::Handle_S2S_RES_LOAD_PLAYER_DATA(PacketSessionRef& session, Protocol::S2S_RES_LOAD_PLAYER_DATA& pkt)
{
    // ChatServer는 원칙적으로 이 응답을 받을 일이 거의 없음
    return true;
}

bool S2SPacketHandler::Handle_S2S_RES_ITEMS_LOAD(PacketSessionRef& session, Protocol::S2S_RES_ITEMS_LOAD& pkt)
{
    return true;
}

bool S2SPacketHandler::Handle_S2S_RES_LOAD_GAME_DATA(PacketSessionRef& session, Protocol::S2S_RES_LOAD_GAME_DATA& pkt)
{
    return true;
}

bool S2SPacketHandler::Handle_S2S_RES_BROADCAST_CHAT(PacketSessionRef& session, Protocol::S2S_RES_BROADCAST_CHAT& pkt)
{
    return true;
}

bool S2SPacketHandler::Handle_S2S_RES_HEART_BEAT(PacketSessionRef& session, Protocol::S2S_RES_HEART_BEAT& pkt)
{
    return true;
}

// Party RES (ChatServer가 요청 보낼 일은 거의 없지만, 인터페이스상 채워둠)
bool S2SPacketHandler::Handle_S2S_RES_PARTY_SYNC(PacketSessionRef& session, Protocol::S2S_RES_PARTY_SYNC& pkt)
{
    return true;
}

bool S2SPacketHandler::Handle_S2S_RES_PARTY_CHAT(PacketSessionRef& session, Protocol::S2S_RES_PARTY_CHAT& pkt)
{
    return true;
}
