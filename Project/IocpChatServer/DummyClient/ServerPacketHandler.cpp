#include "pch.h"
#include "ServerPacketHandler.h"

PacketHandlerFunc GPacketHandler[UINT16_MAX];

// Á÷Á¢ ÄÁÅÙÃ÷ ÀÛ¾÷ÀÚ

bool Handle_INVALID(PacketSessionRef& session, BYTE* buffer, int32 len)
{
	PacketHeader* header = reinterpret_cast<PacketHeader*>(buffer);
	// TODO : Log
	return false;
}

bool Handle_S_LOGIN_RES(PacketSessionRef& session, Protocol::S_LOGIN_RES& pkt)
{
    // Process the login result based on the status and reason from the packet.
    if (pkt.status() == Protocol::CONNECT_OK)
    {
        // Login successful
        int64 playerId = pkt.playerid();
        const std::string& reason = pkt.reason();

        std::cout << "Login successful! Player ID: " << playerId << std::endl;
        std::cout << "Reason: " << reason << std::endl;

        // TODO: Add logic here to transition to the game lobby or send a
        // subsequent packet to the server.
    }
    else if (pkt.status() == Protocol::CONNECT_FAIL)
    {
        // Login failed
        const std::string& reason = pkt.reason();

        std::cout << "Login failed! Reason: " << reason << std::endl;

        // TODO: Add logic to show a login failure pop-up or keep the login screen.
    }
    else
    {
        // Unknown status
        std::cout << "Login response has an unknown status: " << pkt.status() << std::endl;
    }

    return true; // Indicates that the packet was successfully handled.
}

bool Handle_S_RECONNECT_RES(PacketSessionRef& session, Protocol::S_RECONNECT_RES& pkt)
{
	return false;
}


bool Handle_S_PRIVATE_CHAT_NTF(PacketSessionRef& session, Protocol::S_PRIVATE_CHAT_NTF& pkt)
{
	return false;
}

bool Handle_S_CREATE_ROOM_RES(PacketSessionRef& session, Protocol::S_CREATE_ROOM_RES& pkt)
{
	return false;
}

bool Handle_S_JOIN_ROOM_RES(PacketSessionRef& session, Protocol::S_JOIN_ROOM_RES& pkt)
{
	return false;
}

bool Handle_S_ROOM_CHAT_NTF(PacketSessionRef& session, Protocol::S_ROOM_CHAT_NTF& pkt)
{
	return false;
}

bool Handle_S_FRIEND_ADD_RES(PacketSessionRef& session, Protocol::S_FRIEND_ADD_RES& pkt)
{
	return false;
}

bool Handle_S_FRIEND_LIST_RES(PacketSessionRef& session, Protocol::S_FRIEND_LIST_RES& pkt)
{
	return false;
}

bool Handle_S_PRESENCE_NTF(PacketSessionRef& session, Protocol::S_PRESENCE_NTF& pkt)
{
	return false;
}

bool Handle_S_RATE_LIMIT_NTF(PacketSessionRef& session, Protocol::S_RATE_LIMIT_NTF& pkt)
{
	return false;
}

bool Handle_S_ADMIN_COMMAND_RES(PacketSessionRef& session, Protocol::S_ADMIN_COMMAND_RES& pkt)
{
	return false;
}

