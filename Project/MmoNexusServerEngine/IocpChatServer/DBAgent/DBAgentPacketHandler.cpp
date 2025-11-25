#include "pch.h"
#include "DBAgentPacketHandler.h"

// 더미 핸들러 (일단 빌드 되게 껍데기만)
bool Handle_INVALID(PacketSessionRef& session, BYTE* buffer, int32 len)
{
    return false;
}

bool Handle_S2S_REQ_LOGIN(PacketSessionRef& session, Protocol::S2S_REQ_LOGIN& pkt)
{
    // TODO: 내일 여기서 DB 접속해서 유저 정보 긁어오는 로직 짤 거다.
    return true;
}

bool Handle_S2S_REQ_CHAT_LOG(PacketSessionRef& session, Protocol::S2S_REQ_CHAT_LOG& pkt)
{
    return true;
}