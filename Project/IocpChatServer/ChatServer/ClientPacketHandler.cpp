#include "pch.h"
#include "ClientPacketHandler.h"
#include "Player.h"
#include "Room.h"
#include "ChatSession.h"
#include "DBConnectionPool.h"
#include "RoomManager.h"

PacketHandlerFunc GPacketHandler[UINT16_MAX];

// Á÷Á¢ ÄÁÅÙÃ÷ ÀÛ¾÷ÀÚ

bool Handle_INVALID(PacketSessionRef& session, BYTE* buffer, int32 len)
{
    PacketHeader* header = reinterpret_cast<PacketHeader*>(buffer);
    // TODO : Log
    return false;
}

bool Handle_C_LOGIN_REQ(PacketSessionRef& session, Protocol::C_LOGIN_REQ& pkt)
{
	ChatSessionRef chatSession = static_pointer_cast<ChatSession>(session);

	// 1. Get DBConnection from the pool.
	DBConnection* dbConn = GDBConnectionPool->Pop();
	if (dbConn == nullptr)
	{
		Protocol::S_LOGIN_RES resPkt;
		resPkt.set_status(Protocol::CONNECT_FAIL);
		resPkt.set_reason("Failed to get DB connection. Check server status.");
		session->Send(ClientPacketHandler::MakeSendBuffer(resPkt));
		return false;
	}

	// 2. Prepare and execute the SQL query for login authentication.
	std::string name_str = pkt.name();
	std::wstring wname_str(name_str.begin(), name_str.end());

	std::wstring query = L"SELECT playerId FROM Players WHERE name = '" + wname_str + L"'";

	int64 playerId = -1;
	bool success = dbConn->Execute(query.c_str());
	if (success) {
		dbConn->BindCol(1, SQL_C_LONG, sizeof(int64), &playerId, nullptr);
		dbConn->Fetch();
	}

	dbConn->Unbind();
	GDBConnectionPool->Push(dbConn);

	// 3. Check for login success or failure.
	Protocol::S_LOGIN_RES resPkt;

	if (playerId != -1)
	{
		// 4. Handle login session: create a Player object and link it to the session.
		PlayerRef player = MakeShared<Player>();
		player->playerId = playerId;
		player->name = pkt.name();
		player->ownerSession = chatSession;

		chatSession->SetPlayer(player);

		resPkt.set_status(Protocol::CONNECT_OK);
		resPkt.set_playerid(playerId);
		resPkt.set_reason("Login successful.");
	}
	else
	{
		// Player not found in DB or query failed.
		resPkt.set_status(Protocol::CONNECT_FAIL);
		resPkt.set_reason("Player not found or DB error.");
	}

	// 5. Send the response packet to the client.
	auto sendBuffer = ClientPacketHandler::MakeSendBuffer(resPkt);
	session->Send(sendBuffer);

	return true;
}

bool Handle_C_RECONNECT_REQ(PacketSessionRef& session, Protocol::C_RECONNECT_REQ& pkt)
{
    return false;
}

bool Handle_C_PRIVATE_CHAT_REQ(PacketSessionRef& session, Protocol::C_PRIVATE_CHAT_REQ& pkt)
{
    return false;
}

bool Handle_C_CREATE_ROOM_REQ(PacketSessionRef& session, Protocol::C_CREATE_ROOM_REQ& pkt)
{
    ChatSessionRef chatSession = static_pointer_cast<ChatSession>(session);

    // 1. Validate if the session has a player.
    PlayerRef player = chatSession->GetPlayer();
    if (player == nullptr)
    {
        Protocol::S_CREATE_ROOM_RES resPkt;
        resPkt.set_success(false);
        resPkt.set_reason("Player not found in session.");
        session->Send(ClientPacketHandler::MakeSendBuffer(resPkt));
        return false;
    }

    // 2. Create a new room and add it to the room manager.
    RoomRef newRoom = MakeShared<Room>();
    int32 roomId = GRoomManager.AddRoom(newRoom);

    // 3. Set room properties and add the creating player.
    newRoom->SetId(roomId);
    newRoom->SetName(pkt.roomname()); // Set the name from the request packet
    newRoom->SetType(pkt.type());
    newRoom->Enter(player); // The creator enters the room automatically

    // 4. Send a success response to the client.
    Protocol::S_CREATE_ROOM_RES resPkt;
    resPkt.set_success(true);
    resPkt.set_reason("Room created successfully.");

    // Set RoomInfo details for the response packet
    Protocol::RoomInfo* roomInfo = resPkt.mutable_room();
    roomInfo->set_roomid(roomId);
    roomInfo->set_roomname(newRoom->GetName());
    roomInfo->set_type(newRoom->GetType());

    // Add the creating player to the RoomInfo members list
    Protocol::PlayerInfo* playerInfo = roomInfo->add_members();
    playerInfo->set_playerid(player->playerId);
    playerInfo->set_name(player->name);

    session->Send(ClientPacketHandler::MakeSendBuffer(resPkt));

    std::cout << "Player " << player->name << " created a room. Room ID: " << roomId << std::endl;

    return true;
}

bool Handle_C_JOIN_ROOM_REQ(PacketSessionRef& session, Protocol::C_JOIN_ROOM_REQ& pkt)
{
    return false;
}

bool Handle_C_ROOM_CHAT_REQ(PacketSessionRef& session, Protocol::C_ROOM_CHAT_REQ& pkt)
{
    return false;
}

bool Handle_C_FRIEND_ADD_REQ(PacketSessionRef& session, Protocol::C_FRIEND_ADD_REQ& pkt)
{
    return false;
}

bool Handle_C_FRIEND_LIST_REQ(PacketSessionRef& session, Protocol::C_FRIEND_LIST_REQ& pkt)
{
    return false;
}

bool Handle_C_ADMIN_COMMAND_REQ(PacketSessionRef& session, Protocol::C_ADMIN_COMMAND_REQ& pkt)
{
    return false;
}

