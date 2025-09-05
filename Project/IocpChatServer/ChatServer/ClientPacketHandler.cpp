#include "pch.h"
#include "ClientPacketHandler.h"
#include "Player.h"
#include "Room.h"
#include "ChatSession.h"
#include "DBConnectionPool.h"
#include "RoomManager.h"
#include <chrono>

PacketHandlerFunc GPacketHandler[UINT16_MAX];

// 직접 컨텐츠 작업자

bool Handle_INVALID(PacketSessionRef& session, BYTE* buffer, int32 len)
{
	PacketHeader* header = reinterpret_cast<PacketHeader*>(buffer);
	// TODO : Log
	return false;
}

// ===================== LOGIN =====================
bool Handle_C_LOGIN_REQ(PacketSessionRef& session, Protocol::C_LOGIN_REQ& pkt)
{
	ChatSessionRef chatSession = static_pointer_cast<ChatSession>(session);

	DBConnection* dbConn = GDBConnectionPool->Pop();
	if (!dbConn)
	{
		Protocol::S_LOGIN_RES resPkt;
		resPkt.set_status(Protocol::CONNECT_FAIL);
		resPkt.set_reason("Failed to get DB connection. Check server status.");
		session->Send(ClientPacketHandler::MakeSendBuffer(resPkt));
		return false;
	}

	int64 playerId = -1;
	bool success = false;

	std::string name_str = pkt.name();
	std::wstring wname_str(name_str.begin(), name_str.end());

	try
	{
		std::wstring query = L"SELECT playerId FROM Players WHERE name = ?";
		if (dbConn->Prepare(query.c_str()))
		{
			SQLLEN nameLen = SQL_NTS;
			dbConn->BindParam(1, SQL_C_WCHAR, SQL_WVARCHAR, (SQLULEN)wname_str.length(),
				(SQLPOINTER)wname_str.c_str(), &nameLen);

			if (dbConn->Execute())
			{
				dbConn->BindCol(1, SQL_C_LONG, sizeof(int64), &playerId, nullptr);
				success = dbConn->Fetch();
			}
		}
	}
	catch (...) { success = false; }

	dbConn->Unbind();
	GDBConnectionPool->Push(dbConn);

	Protocol::S_LOGIN_RES resPkt;
	if (success && playerId != -1)
	{
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
		resPkt.set_status(Protocol::CONNECT_FAIL);
		resPkt.set_reason("Player not found or DB error.");
	}

	session->Send(ClientPacketHandler::MakeSendBuffer(resPkt));
	return true;
}

// ===================== CREATE ROOM =====================
bool Handle_C_CREATE_ROOM_REQ(PacketSessionRef& session, Protocol::C_CREATE_ROOM_REQ& pkt)
{
	ChatSessionRef chatSession = static_pointer_cast<ChatSession>(session);

	PlayerRef player = chatSession->GetPlayer();
	if (!player)
	{
		Protocol::S_CREATE_ROOM_RES resPkt;
		resPkt.set_success(false);
		resPkt.set_reason("Player not found in session.");
		session->Send(ClientPacketHandler::MakeSendBuffer(resPkt));
		return false;
	}

	RoomRef newRoom = MakeShared<Room>();
	int32 roomId = GRoomManager.AddRoom(newRoom);

	newRoom->SetId(roomId);
	newRoom->SetName(pkt.roomname());
	newRoom->SetType(pkt.type());
	newRoom->Enter(player);

	Protocol::S_CREATE_ROOM_RES resPkt;
	resPkt.set_success(true);
	resPkt.set_reason("Room created successfully.");

	Protocol::RoomInfo* roomInfo = resPkt.mutable_room();
	roomInfo->set_roomid(roomId);
	roomInfo->set_roomname(newRoom->GetName());
	roomInfo->set_type(newRoom->GetType());

	Protocol::PlayerInfo* playerInfo = roomInfo->add_members();
	playerInfo->set_playerid(player->playerId);
	playerInfo->set_name(player->name);

	session->Send(ClientPacketHandler::MakeSendBuffer(resPkt));

	std::cout << "[RoomCreated] " << player->name << " -> Room " << roomId << std::endl;
	return true;
}
// ===================== LEAVE ROOM =====================
bool Handle_C_LEAVE_ROOM_REQ(PacketSessionRef& session, Protocol::C_LEAVE_ROOM_REQ& pkt)
{
	ChatSessionRef chatSession = std::static_pointer_cast<ChatSession>(session);
	if (!chatSession || !chatSession->GetPlayer())
		return false;

	PlayerRef player = chatSession->GetPlayer();
	RoomRef room = player->_room;

	if (!room)
	{
		Protocol::S_LEAVE_ROOM_ACK ackPkt;
		ackPkt.set_success(true);
		session->Send(ClientPacketHandler::MakeSendBuffer(ackPkt));
		return true;
	}

	room->Leave(player);
	player->_room = nullptr;

	if (room->GetPlayers().empty())
	{
		GRoomManager.RemoveRoom(room->GetId());
		std::cout << "[RoomRemoved] Room " << room->GetId() << " deleted (empty)." << std::endl;
	}

	Protocol::S_ROOM_CHAT_NTF ntfPkt;
	static std::atomic<uint64> s_messageId{ 1 };
	Protocol::ChatMessage* chatMsg = ntfPkt.mutable_chat();

	chatMsg->set_messageid(s_messageId.fetch_add(1));
	chatMsg->set_senderid(0);
	chatMsg->set_message("SYSTEM: " + player->name + " has left the room.");

	auto now = std::chrono::system_clock::now();
	auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
	chatMsg->set_timestamp(ms);

	auto ntfBuffer = ClientPacketHandler::MakeSendBuffer(ntfPkt);
	room->BroadcastWithoutSelf(ntfBuffer, player->playerId);

	Protocol::S_LEAVE_ROOM_ACK resPkt;
	resPkt.set_success(true);
	session->Send(ClientPacketHandler::MakeSendBuffer(resPkt));

	return true;
}

// ===================== JOIN ROOM =====================
bool Handle_C_JOIN_ROOM_REQ(PacketSessionRef& session, Protocol::C_JOIN_ROOM_REQ& pkt)
{
	ChatSessionRef chatSession = std::static_pointer_cast<ChatSession>(session);
	if (!chatSession || !chatSession->GetPlayer())
		return false;

	PlayerRef player = chatSession->GetPlayer();
	RoomRef room = GRoomManager.FindRoom(pkt.roomid());

	Protocol::S_JOIN_ROOM_RES resPkt;
	resPkt.set_success(false);

	if (!room)
	{
		resPkt.set_reason("Cannot find the room");
		session->Send(ClientPacketHandler::MakeSendBuffer(resPkt));
		return true;
	}

	bool joinSuccess = room->Enter(player);

	if (joinSuccess)
	{
		resPkt.set_success(true);

		Protocol::RoomInfo* roomInfo = resPkt.mutable_room();
		roomInfo->set_roomid(room->GetId());
		roomInfo->set_type(room->GetType());
		roomInfo->set_roomname(room->GetName());

		for (const auto& member : room->GetPlayers())
		{
			Protocol::PlayerInfo* memberInfo = roomInfo->add_members();
			memberInfo->set_playerid(member.second->playerId);
			memberInfo->set_name(member.second->name);
		}
	}
	else
	{
		resPkt.set_reason("Failed to join the room");
	}

	session->Send(ClientPacketHandler::MakeSendBuffer(resPkt));

	if (joinSuccess)
	{
		Protocol::S_JOIN_ROOM_NTF ntfPkt;
		ntfPkt.set_name(player->name);
		auto ntfBuffer = ClientPacketHandler::MakeSendBuffer(ntfPkt);
		room->BroadcastWithoutSelf(ntfBuffer, player->playerId);
	}

	return true;
}
bool Handle_C_ROOM_CHAT_REQ(PacketSessionRef& session, Protocol::C_ROOM_CHAT_REQ& pkt)
{
	ChatSessionRef chatSession = std::static_pointer_cast<ChatSession>(session);
	if (!chatSession || !chatSession->GetPlayer())
		return false;

	PlayerRef player = chatSession->GetPlayer();
	RoomRef room = player->_room;

	if (!room)
		return true;

	int32 roomId = room->GetId();

	Protocol::S_ROOM_CHAT_NTF resPkt;
	resPkt.set_roomid(roomId);

	Protocol::ChatMessage* chatMsg = resPkt.mutable_chat();
	static std::atomic<uint64> s_messageId{ 1 };
	chatMsg->set_messageid(s_messageId.fetch_add(1));
	chatMsg->set_senderid(player->playerId);
	chatMsg->set_message(pkt.message());

	auto now = std::chrono::system_clock::now();
	auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
	chatMsg->set_timestamp(ms);

	auto sendBuffer = ClientPacketHandler::MakeSendBuffer(resPkt);

	room->Broadcast(sendBuffer);

	// === 로그 추가 ===
	room->LogChat(player->playerId, player->name, pkt.message());
	room->LogChatToDB(player->playerId, player->name, pkt.message());

	return true;
}

bool Handle_C_ADMIN_COMMAND_REQ(PacketSessionRef& session, Protocol::C_ADMIN_COMMAND_REQ& pkt)
{
	return false;
}