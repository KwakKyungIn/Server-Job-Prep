#pragma once
#include "Protocol_S2S.pb.h"

using PacketHandlerFunc = std::function<bool(PacketSessionRef&, BYTE*, int32)>;
extern PacketHandlerFunc GPacketHandler[UINT16_MAX];

enum : uint16
{
    PKT_S2S_REQ_LOGIN = 2000,
    PKT_S2S_RES_LOGIN = 2001,
    PKT_S2S_REQ_CHAT_LOG = 2002,
    PKT_S2S_RES_CHAT_LOG = 2003,
};

// Custom Handlers
bool Handle_INVALID(PacketSessionRef& session, BYTE* buffer, int32 len);
bool Handle_S2S_RES_LOGIN(PacketSessionRef& session, Protocol::S2S_RES_LOGIN& pkt);
bool Handle_S2S_RES_CHAT_LOG(PacketSessionRef& session, Protocol::S2S_RES_CHAT_LOG& pkt);

class S2SPacketHandler
{
public:
    static void Init()
    {
        for (int32 i = 0; i < UINT16_MAX; i++)
            GPacketHandler[i] = Handle_INVALID;
        GPacketHandler[PKT_S2S_RES_LOGIN] = [](PacketSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::S2S_RES_LOGIN>(Handle_S2S_RES_LOGIN, session, buffer, len); };
        GPacketHandler[PKT_S2S_RES_CHAT_LOG] = [](PacketSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::S2S_RES_CHAT_LOG>(Handle_S2S_RES_CHAT_LOG, session, buffer, len); };
    }

    static bool HandlePacket(PacketSessionRef& session, BYTE* buffer, int32 len)
    {
        PacketHeader* header = reinterpret_cast<PacketHeader*>(buffer);
        return GPacketHandler[header->id](session, buffer, len);
    }
    static SendBufferRef MakeSendBuffer(Protocol::S2S_REQ_LOGIN& pkt) { return MakeSendBuffer(pkt, PKT_S2S_REQ_LOGIN); }
    static SendBufferRef MakeSendBuffer(Protocol::S2S_REQ_CHAT_LOG& pkt) { return MakeSendBuffer(pkt, PKT_S2S_REQ_CHAT_LOG); }

private:
    template<typename PacketType, typename ProcessFunc>
    static bool HandlePacket(ProcessFunc func, PacketSessionRef& session, BYTE* buffer, int32 len)
    {
        PacketType pkt;
        if (pkt.ParseFromArray(buffer + sizeof(PacketHeader), len - sizeof(PacketHeader)) == false)
            return false;

        return func(session, pkt);
    }

    template<typename T>
    static SendBufferRef MakeSendBuffer(T& pkt, uint16 pktId)
    {
        const uint16 dataSize = static_cast<uint16>(pkt.ByteSizeLong());
        const uint16 packetSize = dataSize + sizeof(PacketHeader);

        SendBufferRef sendBuffer = GSendBufferManager->Open(packetSize);
        PacketHeader* header = reinterpret_cast<PacketHeader*>(sendBuffer->Buffer());
        header->size = packetSize;
        header->id = pktId;
        ASSERT_CRASH(pkt.SerializeToArray(&header[1], dataSize));
        sendBuffer->Close(packetSize);

        return sendBuffer;
    }
};