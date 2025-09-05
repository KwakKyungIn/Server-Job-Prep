#include "pch.h"
#include "Room.h"
#include "Player.h"
#include "ChatSession.h"
#include "DBConnectionPool.h"
#include <vector>
#include <chrono>

#include "Metrics.h" // [METRICS]

// 생성자
Room::Room() : _jobQueue(0)
{
    _jobQueue.Start(1); // per-room single thread

    // roomId는 아직 없는 상태 → SetId()에서 로그 파일 오픈
}

// 소멸자
Room::~Room()
{
    _jobQueue.Stop();
    if (_logFile.is_open())
        _logFile.close();
}

void Room::SetId(int32 roomId) {
    _roomId = roomId;
    if (_logFile.is_open()) _logFile.close();
    std::string filename = "Room_" + std::to_string(_roomId) + "_chat.log";
    _logFile.open(filename, std::ios::out | std::ios::app);
    if (!_logFile.is_open()) {
        std::cerr << "Failed to open log file for room " << _roomId << std::endl;
        // [METRICS]
        GMetrics.room_log_open_fail.fetch_add(1, std::memory_order_relaxed);
    }
    else {
        // [METRICS] (선택) 방 로그파일 정상 오픈 카운터가 있으면 올려도 됨
        // GMetrics.room_log_open_ok.fetch_add(1, std::memory_order_relaxed);
    }
}

bool Room::Enter(PlayerRef player)
{
    WRITE_LOCK;
    if (_players.size() >= 100) {
        // [METRICS]
        GMetrics.room_enter_fail.fetch_add(1, std::memory_order_relaxed);
        return false;
    }

    _players[player->playerId] = player;
    player->_room = shared_from_this();

    // [METRICS]
    GMetrics.room_enter_ok.fetch_add(1, std::memory_order_relaxed);
    return true;
}

void Room::Leave(PlayerRef player)
{
    WRITE_LOCK;
    _players.erase(player->playerId);
    player->_room = nullptr;

    // [METRICS]
    GMetrics.room_leave.fetch_add(1, std::memory_order_relaxed);
}

void Room::Broadcast(SendBufferRef sendBuffer)
{
    std::vector<PacketSessionRef> targets;
    {
        READ_LOCK;
        for (const auto& kv : _players)
            if (kv.second && kv.second->ownerSession)
                targets.push_back(kv.second->ownerSession);
    }

    if (targets.empty()) return;

    // [METRICS] 실제 전달 수(수신자 수 단위)
    GMetrics.broadcast_deliveries.fetch_add(targets.size(), std::memory_order_relaxed);

    if (!_jobQueue.TryEnqueue([targets, sendBuffer]() {
        for (auto& sess : targets)
            sess->Send(sendBuffer);
        })) {
        for (auto& sess : targets)
            sess->Send(sendBuffer);
    }
}

void Room::BroadcastWithoutSelf(SendBufferRef sendBuffer, uint64 selfId)
{
    struct Target { PacketSessionRef sess; uint64 id; };
    std::vector<Target> targets;
    {
        READ_LOCK;
        for (const auto& kv : _players)
            if (kv.second && kv.second->ownerSession)
                targets.push_back({ kv.second->ownerSession, kv.first });
    }

    if (targets.empty()) return;

    if (!_jobQueue.TryEnqueue([targets, sendBuffer, selfId]() {
        for (const auto& t : targets) {
            if (t.id == selfId) continue;
            t.sess->Send(sendBuffer);
        }
        })) {
        for (const auto& t : targets) {
            if (t.id == selfId) continue;
            t.sess->Send(sendBuffer);
        }
    }
}

void Room::LogChat(uint64 playerId, const std::string& playerName, const std::string& message)
{
    if (!_logFile.is_open()) return;

    auto now = std::chrono::system_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();

    _jobQueue.TryEnqueue([this, ms, playerId, playerName, message]() {
        if (_logFile.is_open()) {
            _logFile << ms << "," << playerId << "," << playerName << "," << message << std::endl;
        }
        });

    // [METRICS]
    GMetrics.chat_log_file_lines.fetch_add(1, std::memory_order_relaxed);
}

void Room::LogChatToDB(uint64 playerId, const std::string& playerName, const std::string& message)
{
    _jobQueue.TryEnqueue([playerId, playerName, message]() {
        DBConnection* dbConn = GDBConnectionPool->Pop();
        if (!dbConn) {
            // [METRICS]
            GMetrics.conn_acquire_fail.fetch_add(1, std::memory_order_relaxed);
            return;
        }

        try {
            std::wstring query = L"INSERT INTO ChatLogs (PlayerId, PlayerName, Message, Timestamp) VALUES (?, ?, ?, ?)";
            if (dbConn->Prepare(query.c_str())) {
                auto wname = std::wstring(playerName.begin(), playerName.end());
                auto wmsg = std::wstring(message.begin(), message.end());

                SQLLEN idLen = 0, nameLen = SQL_NTS, msgLen = SQL_NTS, tsLen = 0;
                int64 ts = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now().time_since_epoch()).count();

                dbConn->BindParam(1, SQL_C_SBIGINT, SQL_BIGINT, 0, (SQLPOINTER)&playerId, &idLen);
                dbConn->BindParam(2, SQL_C_WCHAR, SQL_WVARCHAR, (SQLULEN)wname.size(), (SQLPOINTER)wname.c_str(), &nameLen);
                dbConn->BindParam(3, SQL_C_WCHAR, SQL_WVARCHAR, (SQLULEN)wmsg.size(), (SQLPOINTER)wmsg.c_str(), &msgLen);
                dbConn->BindParam(4, SQL_C_SBIGINT, SQL_BIGINT, 0, (SQLPOINTER)&ts, &tsLen);

                if (dbConn->Execute()) {
                    // [METRICS]
                    GMetrics.chat_log_db_ok.fetch_add(1, std::memory_order_relaxed);
                }
                else {
                    GMetrics.chat_log_db_fail.fetch_add(1, std::memory_order_relaxed);
                }
            }
            else {
                // Prepare 실패
                GMetrics.chat_log_db_fail.fetch_add(1, std::memory_order_relaxed);
            }
        }
        catch (...) {
            // [METRICS]
            GMetrics.chat_log_db_fail.fetch_add(1, std::memory_order_relaxed);
        }

        dbConn->Unbind();
        GDBConnectionPool->Push(dbConn);
        });
}
