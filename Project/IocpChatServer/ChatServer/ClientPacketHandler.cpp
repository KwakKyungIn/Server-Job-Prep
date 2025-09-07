#include "pch.h"
#include "ClientPacketHandler.h"
#include "Player.h"
#include "Room.h"
#include "ChatSession.h"
#include "DBConnectionPool.h"
#include "RoomManager.h"
#include <chrono>
#include "Metrics.h" // [METRICS] 추가

PacketHandlerFunc GPacketHandler[UINT16_MAX];

// 직접 컨텐츠 작업자

bool Handle_INVALID(PacketSessionRef& session, BYTE* buffer, int32 len)
{
    // [METRICS] 앱 패킷 IN 카운트
    GMetrics.app_packets_in.fetch_add(1, std::memory_order_relaxed);
    PacketHeader* header = reinterpret_cast<PacketHeader*>(buffer);
    // TODO : Log
    return false;
}

// ===================== LOGIN =====================
bool Handle_C_LOGIN_REQ(PacketSessionRef& session, Protocol::C_LOGIN_REQ& pkt)
{
    // [METRICS] 앱 패킷 IN
    GMetrics.app_packets_in.fetch_add(1, std::memory_order_relaxed);

    ChatSessionRef chatSession = static_pointer_cast<ChatSession>(session);

    DBConnection* dbConn = GDBConnectionPool->Pop();
    if (!dbConn)
    {
        // [METRICS] 커넥션 획득 실패
        GMetrics.conn_acquire_fail.fetch_add(1, std::memory_order_relaxed);

        Protocol::S_LOGIN_RES resPkt;
        resPkt.set_status(Protocol::CONNECT_FAIL);
        resPkt.set_reason("Failed to get DB connection. Check server status.");
        session->Send(ClientPacketHandler::MakeSendBuffer(resPkt));
        return false;
    }

    // [METRICS] 풀 pop
    GMetrics.conn_pool_pop_total.fetch_add(1, std::memory_order_relaxed);

    int64 playerId = -1;
    bool success = false;

    std::string name_str = pkt.name();
    std::wstring wname_str(name_str.begin(), name_str.end());

    try
    {
        std::wstring query = L"SELECT playerId FROM Players WHERE name = ?";
        if (dbConn->Prepare(query.c_str()))
        {
            // [METRICS] Prepare 성공 → 쿼리 시도 카운트(성공/실패 무관)
            GMetrics.db_query_count.fetch_add(1, std::memory_order_relaxed);

            SQLLEN nameLen = SQL_NTS;
            dbConn->BindParam(1, SQL_C_WCHAR, SQL_WVARCHAR, (SQLULEN)wname_str.length(),
                (SQLPOINTER)wname_str.c_str(), &nameLen);

            if (dbConn->Execute())
            {
                dbConn->BindCol(1, SQL_C_LONG, sizeof(int64), &playerId, nullptr);
                success = dbConn->Fetch();

                // [METRICS] Exec 결과
                GMetrics.db_exec_ok.fetch_add(1, std::memory_order_relaxed);
                // [METRICS] Fetch 결과(단순화: 성공/노데이터 구분)
                if (success)
                    GMetrics.db_fetch_ok.fetch_add(1, std::memory_order_relaxed);
                else
                    GMetrics.db_fetch_no_data.fetch_add(1, std::memory_order_relaxed);
            }
            else
            {
                GMetrics.db_exec_fail.fetch_add(1, std::memory_order_relaxed);
            }
        }
        else
        {
            GMetrics.db_prepare_fail.fetch_add(1, std::memory_order_relaxed);
        }
    }
    catch (...) { success = false; }

    dbConn->Unbind();
    GDBConnectionPool->Push(dbConn);
    // [METRICS] 풀 push
    GMetrics.conn_pool_push_total.fetch_add(1, std::memory_order_relaxed);

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
        // players_gauge 증감은 ChatSession/Manager에서 처리 예정
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
    // [METRICS] 앱 패킷 IN
    GMetrics.app_packets_in.fetch_add(1, std::memory_order_relaxed);

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
    // [METRICS] 앱 패킷 IN
    GMetrics.app_packets_in.fetch_add(1, std::memory_order_relaxed);

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
    // [METRICS] 룸 퇴장
    GMetrics.room_leave.fetch_add(1, std::memory_order_relaxed);

    player->_room = nullptr;

    if (room->GetPlayers().empty())
    {
        GRoomManager.RemoveRoom(room->GetId());
        std::cout << "[RoomRemoved] Room " << room->GetId() << " deleted (empty)." << std::endl;
        // rooms_gauge/peak는 RoomManager에서 계측 예정
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
    // [METRICS] 앱 패킷 IN
    GMetrics.app_packets_in.fetch_add(1, std::memory_order_relaxed);

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
        // [METRICS] 룸 입장 성공
        GMetrics.room_enter_ok.fetch_add(1, std::memory_order_relaxed);

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
        // [METRICS] 룸 입장 실패
        GMetrics.room_enter_fail.fetch_add(1, std::memory_order_relaxed);

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
    // [METRICS] 앱 패킷 IN
    GMetrics.app_packets_in.fetch_add(1, std::memory_order_relaxed);

    ChatSessionRef chatSession = std::static_pointer_cast<ChatSession>(session);
    if (!chatSession || !chatSession->GetPlayer())
        return false;

    PlayerRef player = chatSession->GetPlayer();
    RoomRef room = player->_room;

    if (!room)
        return true;

    // [METRICS] 브로드캐스트 서버 내부 지연 측정 시작
    auto t0 = std::chrono::steady_clock::now();

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

    // [METRICS] 브로드캐스트 서버 내부 지연 측정 종료/기록
    auto t1 = std::chrono::steady_clock::now();
    auto us = std::chrono::duration_cast<std::chrono::microseconds>(t1 - t0).count();
    uint32_t us32 = (us <= 0) ? 0u : (us > INT32_MAX ? static_cast<uint32_t>(INT32_MAX) : static_cast<uint32_t>(us));
    GMetrics.ObserveBroadcastLatencyUS(us32);

    // === 로그 추가 ===
    room->LogChat(player->playerId, player->name, pkt.message());
    room->LogChatToDB(player->playerId, player->name, pkt.message());

    return true;
}

bool Handle_C_ADMIN_COMMAND_REQ(PacketSessionRef& session, Protocol::C_ADMIN_COMMAND_REQ& pkt)
{
    // [METRICS] 앱 패킷 IN
    GMetrics.app_packets_in.fetch_add(1, std::memory_order_relaxed);
    return false;
}
