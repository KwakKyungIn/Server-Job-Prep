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
// [새로 추가] C_LEAVE_ROOM_REQ 핸들러
bool Handle_C_LEAVE_ROOM_REQ(PacketSessionRef& session, Protocol::C_LEAVE_ROOM_REQ& pkt)
{
	ChatSessionRef chatSession = std::static_pointer_cast<ChatSession>(session);
	if (chatSession == nullptr || chatSession->GetPlayer() == nullptr)
	{
		return false;
	}
	PlayerRef player = chatSession->GetPlayer();

	// 플레이어가 속한 방을 찾습니다.
	RoomRef room = player->_room;

	// 플레이어가 방에 있는지 확인합니다.
	if (room == nullptr)
	{
		// 이미 방에 없으면 성공으로 간주하고 응답만 보냅니다.
		Protocol::S_LEAVE_ROOM_ACK ackPkt;
		ackPkt.set_success(true);
		auto resBuffer = ClientPacketHandler::MakeSendBuffer(ackPkt);
		session->Send(resBuffer);
		return true;
	}

	// 방에서 플레이어를 제거합니다.
	room->Leave(player);
	if (room->GetPlayers().empty()) // 또는 GetPlayerCount() == 0
	{
		GRoomManager.RemoveRoom(room->GetId());
		std::cout << "방 ID " << room->GetId() << "이(가) 모든 플레이어가 나가서 삭제되었습니다." << std::endl;
	}

	Protocol::S_ROOM_CHAT_NTF ntfPkt;
	static std::atomic<uint64> s_messageId{ 1 };
	Protocol::ChatMessage* chatMsg = ntfPkt.mutable_chat();

	// ChatMessage 필드 채우기
	chatMsg->set_messageid(s_messageId.fetch_add(1));
	chatMsg->set_senderid(0); // 시스템 메시지이므로 senderId를 0으로 설정
	chatMsg->set_message("SYSTEM: " + player->name + " has left the room.");

	// timestamp: 현재 epoch(ms) 기준
	auto now = std::chrono::system_clock::now();
	auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
	chatMsg->set_timestamp(ms);

	auto ntfBuffer = ClientPacketHandler::MakeSendBuffer(ntfPkt);

	// 알림 패킷을 자신을 제외한 다른 플레이어들에게 브로드캐스트합니다.
	room->BroadcastWithoutSelf(ntfBuffer, player->playerId);

	// 클라이언트에 방 나가기 성공을 알리는 응답 패킷을 보냅니다.
	Protocol::S_LEAVE_ROOM_ACK resPkt;
	resPkt.set_success(true);
	auto resBuffer = ClientPacketHandler::MakeSendBuffer(resPkt);
	session->Send(resBuffer);

	return true;
}
bool Handle_C_JOIN_ROOM_REQ(PacketSessionRef& session, Protocol::C_JOIN_ROOM_REQ& pkt)
{
	ChatSessionRef chatSession = std::static_pointer_cast<ChatSession>(session);
	if (chatSession == nullptr || chatSession->GetPlayer() == nullptr)
	{
		return false;
	}
	PlayerRef player = chatSession->GetPlayer();

	int64 roomId = pkt.roomid();
	RoomRef room = GRoomManager.FindRoom(roomId);

	Protocol::S_JOIN_ROOM_RES resPkt;
	resPkt.set_success(false);

	if (room == nullptr)
	{
		resPkt.set_reason("Cannot find the room");
		auto sendBuffer = ClientPacketHandler::MakeSendBuffer(resPkt);
		session->Send(sendBuffer);
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
		resPkt.set_reason("방 입장에 실패했습니다.");
	}

	auto sendBuffer = ClientPacketHandler::MakeSendBuffer(resPkt);
	session->Send(sendBuffer);

	if (joinSuccess)
	{
		// 1. 다른 플레이어들에게 입장 알림 패킷을 보냅니다.
		Protocol::S_JOIN_ROOM_NTF ntfPkt;
		ntfPkt.set_name(player->name);
		auto ntfBuffer = ClientPacketHandler::MakeSendBuffer(ntfPkt);

		// 2. 자신을 제외한 다른 모든 플레이어에게 브로드캐스트합니다.
		room->BroadcastWithoutSelf(ntfBuffer, player->playerId);
	}

	return true; 
}
bool Handle_C_ROOM_CHAT_REQ(PacketSessionRef& session, Protocol::C_ROOM_CHAT_REQ& pkt)
{
	ChatSessionRef chatSession = std::static_pointer_cast<ChatSession>(session);
	if (chatSession == nullptr || chatSession->GetPlayer() == nullptr)
		return false;

	PlayerRef player = chatSession->GetPlayer();
	RoomRef room = player->_room;

	// 방이 없으면 응답 없이 리턴
	if (room == nullptr)
		return true;

	// 방 정보에서 roomId 가져오기
	int32 roomId = room->GetId();

	// 브로드캐스트 패킷 생성
	Protocol::S_ROOM_CHAT_NTF resPkt;
	resPkt.set_roomid(roomId);

	// ChatMessage 생성 및 채우기
	Protocol::ChatMessage* chatMsg = resPkt.mutable_chat();
	static std::atomic<uint64> s_messageId{ 1 }; // 유니크 messageId 생성기
	chatMsg->set_messageid(s_messageId.fetch_add(1));
	chatMsg->set_senderid(player->playerId);
	chatMsg->set_message(pkt.message());

	// timestamp: 현재 epoch(ms) 기준
	auto now = std::chrono::system_clock::now();
	auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
	chatMsg->set_timestamp(ms);

	// 패킷 직렬화
	auto sendBuffer = ClientPacketHandler::MakeSendBuffer(resPkt);

	// 방에 속한 모든 플레이어에게 브로드캐스트
	room->Broadcast(sendBuffer);

	return true;
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


