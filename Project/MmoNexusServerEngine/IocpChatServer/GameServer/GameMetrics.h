#pragma once

#include <cstdint>

namespace Protocol
{
    class S2S_REQ_LOAD_PLAYER_DATA;
    class S2S_RES_LOAD_PLAYER_DATA;
    class S2S_REQ_ITEMS_LOAD;
    class S2S_RES_ITEMS_LOAD;
    class S2S_REQ_SAVE_PLAYER_CORE;
    class S2S_RES_SAVE_PLAYER_CORE;
    class S2S_REQ_SAVE_INVENTORY;
    class S2S_RES_SAVE_INVENTORY;
    class S2S_REQ_ITEM_CREATE;
    class S2S_RES_ITEM_CREATE;
    class S2S_REQ_QUICKSLOT_LOAD;
    class S2S_RES_QUICKSLOT_LOAD;
    class S2S_REQ_SAVE_QUICKSLOT;
    class S2S_RES_SAVE_QUICKSLOT;
    class S2S_REQ_TRADE_COMMIT;
    class S2S_RES_TRADE_COMMIT;
}

namespace GameMetrics
{
    enum class ClientPacketFailureReason
    {
        Parse,
        Validate,
        Handler,
    };

    void Initialize();
    void Shutdown();

    void OnClientPacketDispatch(std::uint16_t packetId);
    void OnClientPacketHandled(std::uint16_t packetId, double elapsedSeconds);
    void OnClientPacketFailure(std::uint16_t packetId, ClientPacketFailureReason reason);

    void OnLobbyEnterStart(std::uint64_t playerId);
    void OnLobbyEnterComplete(std::uint64_t playerId);
    void OnLobbyEnterCancelled(std::uint64_t playerId);

    void OnSessionCountChanged(std::int64_t sessionCount);
    void OnIngamePlayerCountChanged(std::int64_t ingameCount);

    template<typename T>
    inline void TrackS2SRequestPacket(std::uint16_t /*packetId*/, const T& /*pkt*/)
    {
    }

    template<typename T>
    inline void TrackS2SResponsePacket(std::uint16_t /*packetId*/, const T& /*pkt*/)
    {
    }

    void TrackS2SRequestPacket(std::uint16_t packetId, const Protocol::S2S_REQ_LOAD_PLAYER_DATA& pkt);
    void TrackS2SRequestPacket(std::uint16_t packetId, const Protocol::S2S_REQ_ITEMS_LOAD& pkt);
    void TrackS2SRequestPacket(std::uint16_t packetId, const Protocol::S2S_REQ_SAVE_PLAYER_CORE& pkt);
    void TrackS2SRequestPacket(std::uint16_t packetId, const Protocol::S2S_REQ_SAVE_INVENTORY& pkt);
    void TrackS2SRequestPacket(std::uint16_t packetId, const Protocol::S2S_REQ_ITEM_CREATE& pkt);
    void TrackS2SRequestPacket(std::uint16_t packetId, const Protocol::S2S_REQ_QUICKSLOT_LOAD& pkt);
    void TrackS2SRequestPacket(std::uint16_t packetId, const Protocol::S2S_REQ_SAVE_QUICKSLOT& pkt);
    void TrackS2SRequestPacket(std::uint16_t packetId, const Protocol::S2S_REQ_TRADE_COMMIT& pkt);

    void TrackS2SResponsePacket(std::uint16_t packetId, const Protocol::S2S_RES_LOAD_PLAYER_DATA& pkt);
    void TrackS2SResponsePacket(std::uint16_t packetId, const Protocol::S2S_RES_ITEMS_LOAD& pkt);
    void TrackS2SResponsePacket(std::uint16_t packetId, const Protocol::S2S_RES_SAVE_PLAYER_CORE& pkt);
    void TrackS2SResponsePacket(std::uint16_t packetId, const Protocol::S2S_RES_SAVE_INVENTORY& pkt);
    void TrackS2SResponsePacket(std::uint16_t packetId, const Protocol::S2S_RES_ITEM_CREATE& pkt);
    void TrackS2SResponsePacket(std::uint16_t packetId, const Protocol::S2S_RES_QUICKSLOT_LOAD& pkt);
    void TrackS2SResponsePacket(std::uint16_t packetId, const Protocol::S2S_RES_SAVE_QUICKSLOT& pkt);
    void TrackS2SResponsePacket(std::uint16_t packetId, const Protocol::S2S_RES_TRADE_COMMIT& pkt);
}
