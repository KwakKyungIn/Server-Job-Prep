#include "pch.h"
#include "DbLogger.h"
// #include "pch.h" // Duplicate include
#include "DBConnectionPool.h"
#include "DBConnection.h"
#include "Metrics.h"
#include <chrono>
#include <sstream> // For string building
#include <algorithm> // For std::min

DbLogger* GDbLogger = nullptr;

DbLogger::DbLogger()
{
}

DbLogger::~DbLogger()
{
    Stop();

    while (false == _queue.empty())
    {
        DbLogJob* job = _queue.front();
        _queue.pop();
        delete job;
    }
}

// GIGACHAD FIX: 'threadCount'만큼 워커 쓰레드를 생성
void DbLogger::Start(int32 threadCount)
{
    if (threadCount <= 0)
        threadCount = 1;

    _running = true;

    _workers.reserve(threadCount);
    for (int32 i = 0; i < threadCount; ++i)
    {
        _workers.emplace_back(&DbLogger::WorkerThread, this);
    }
}

// GIGACHAD FIX: 모든 워커 쓰레드를 깨우고 Join
void DbLogger::Stop()
{
    _running = false;
    _cv.notify_all(); // 자고 있는 모든 쓰레드를 깨운다 (중요!)

    for (std::thread& worker : _workers)
    {
        if (worker.joinable())
            worker.join();
    }
    _workers.clear();
}

void DbLogger::Push(uint64 playerId, const std::string& playerName, const std::string& message)
{
    int64 timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();

    DbLogJob* job = new DbLogJob(playerId, playerName, message, timestamp);

    {
        std::lock_guard<std::mutex> lock(_mutex);
        _queue.push(job);
    }

    _cv.notify_one(); // 자고 있는 쓰레드 중 '하나'만 깨운다 (효율적)
}

void DbLogger::WorkerThread()
{
    // GigaChad Note: BATCH_SIZE is now the *maximum* number of jobs
    // we pull from the queue. The *real* batching happens in ExecuteBatch.
    const int32 BATCH_SIZE = 1000;
    const auto BATCH_TIMEOUT = std::chrono::milliseconds(1000);

    std::vector<DbLogJob*> batch;
    batch.reserve(BATCH_SIZE);

    while (_running)
    {
        {
            std::unique_lock<std::mutex> lock(_mutex);
            _cv.wait_for(lock, BATCH_TIMEOUT, [this]() {
                return !_queue.empty() || !_running;
                });

            if (!_running && _queue.empty())
                break;

            // Pull *up to* BATCH_SIZE jobs from the queue.
            while (batch.size() < BATCH_SIZE && !_queue.empty())
            {
                batch.push_back(_queue.front());
                _queue.pop();
            }
        } // GigaChad Note: Mutex(lock) is released here!
          // The slow DB work happens *outside* the lock. This is why multi-threading works.

        if (!batch.empty())
        {
            ExecuteBatch(batch); // This function now does the *real* bulk insert

            for (DbLogJob* job : batch)
                delete job; // Free the memory

            batch.clear(); // Clear the vector for the next loop
        }
    }
}

// ... ExecuteBatch ...
// [ GIGACHAD FIX: THE *REAL* BATCH EXECUTION (SOLUTION 1) ]
// (이 함수는 수정할 필요 없음. 이미 멀티쓰레딩에서 잘 동작함)
void DbLogger::ExecuteBatch(std::vector<DbLogJob*>& jobs)
{
    if (jobs.empty())
        return;

    DBConnection* dbConn = GDBConnectionPool->Pop();
    if (!dbConn)
    {
        GMetrics.conn_acquire_fail.fetch_add(jobs.size(), std::memory_order_relaxed);
        GMetrics.chat_log_db_fail.fetch_add(jobs.size(), std::memory_order_relaxed);
        return;
    }

    GMetrics.conn_pool_pop_total.fetch_add(1, std::memory_order_relaxed);

    const int32 PARAMS_PER_JOB = 4;
    const int32 MAX_JOBS_PER_CHUNK = 500;

    int32 BATCH_EXEC_OK = 0;
    int32 BATCH_EXEC_FAIL = 0;
    bool allChunksSucceeded = true;

    // 1. Turn off Auto-Commit (Start Transaction)
    if (!dbConn->SetAutoCommit(false))
    {
        GMetrics.chat_log_db_fail.fetch_add(jobs.size(), std::memory_order_relaxed);
        GDBConnectionPool->Push(dbConn);
        return;
    }

    int32 jobsProcessed = 0;
    int32 totalJobs = static_cast<int32>(jobs.size());

    while (jobsProcessed < totalJobs)
    {
        int32 jobsInThisChunk = std::min(totalJobs - jobsProcessed, MAX_JOBS_PER_CHUNK);
        if (jobsInThisChunk <= 0)
            break;

        // 2. Build the giant query string: "INSERT ... VALUES (...), (...), ..."
        std::wstringstream wQueryStream;
        wQueryStream << L"INSERT INTO ChatLogs (PlayerId, PlayerName, Message, Timestamp) VALUES ";
        for (int i = 0; i < jobsInThisChunk; ++i)
        {
            wQueryStream << L"(?, ?, ?, ?)";
            if (i < jobsInThisChunk - 1)
                wQueryStream << L", ";
        }

        std::wstring query = wQueryStream.str();

        // 3. Prepare the query *once* for this chunk
        if (!dbConn->Prepare(query.c_str()))
        {
            GMetrics.db_prepare_fail.fetch_add(1, std::memory_order_relaxed);
            BATCH_EXEC_FAIL += jobsInThisChunk;
            allChunksSucceeded = false;
            break; // Stop processing chunks if prepare fails
        }

        std::vector<std::wstring> wPlayerNames; wPlayerNames.reserve(jobsInThisChunk);
        std::vector<std::wstring> wMessages; wMessages.reserve(jobsInThisChunk);
        for (int i = 0; i < jobsInThisChunk; ++i)
        {
            DbLogJob* job = jobs[jobsProcessed + i];
            wPlayerNames.push_back(std::wstring(job->playerName.begin(), job->playerName.end()));
            wMessages.push_back(std::wstring(job->message.begin(), job->message.end()));
        }

        // 4. Bind all 2000 (500*4) parameters for this chunk
        int32 paramIndex = 1;
        bool bindFailed = false;
        for (int i = 0; i < jobsInThisChunk; ++i)
        {
            DbLogJob* job = jobs[jobsProcessed + i];
            SQLLEN idLen = 0, nameLen = SQL_NTS, msgLen = SQL_NTS, tsLen = 0;

            if (!dbConn->BindParam(paramIndex++, SQL_C_SBIGINT, SQL_BIGINT, 0, (SQLPOINTER) & (job->playerId), &idLen) ||
                !dbConn->BindParam(paramIndex++, SQL_C_WCHAR, SQL_WVARCHAR, (SQLULEN)wPlayerNames[i].size(), (SQLPOINTER)wPlayerNames[i].c_str(), &nameLen) ||
                !dbConn->BindParam(paramIndex++, SQL_C_WCHAR, SQL_WVARCHAR, (SQLULEN)wMessages[i].size(), (SQLPOINTER)wMessages[i].c_str(), &msgLen) ||
                !dbConn->BindParam(paramIndex++, SQL_C_SBIGINT, SQL_BIGINT, 0, (SQLPOINTER) & (job->timestamp), &tsLen))
            {
                bindFailed = true;
                break;
            }
        }

        if (bindFailed)
        {
            BATCH_EXEC_FAIL += jobsInThisChunk;
            allChunksSucceeded = false;
            dbConn->Unbind();
            break;
        }

        // 5. Execute the query *ONCE* for this 500-job chunk
        if (dbConn->Execute())
        {
            BATCH_EXEC_OK += jobsInThisChunk;
        }
        else
        {
            BATCH_EXEC_FAIL += jobsInThisChunk;
            allChunksSucceeded = false;
        }

        dbConn->Unbind();
        jobsProcessed += jobsInThisChunk;
    } // End of while loop (chunks)

    // 6. Commit or Rollback the *entire* transaction
    if (allChunksSucceeded)
        dbConn->Commit();
    else
        dbConn->Rollback();

    // 7. Restore Auto-Commit
    dbConn->SetAutoCommit(true);

    // 8. Update Metrics (now batched)
    GMetrics.db_query_count.fetch_add(jobs.size(), std::memory_order_relaxed);
    GMetrics.db_exec_ok.fetch_add(BATCH_EXEC_OK, std::memory_order_relaxed);
    GMetrics.db_exec_fail.fetch_add(BATCH_EXEC_FAIL, std::memory_order_relaxed);
    GMetrics.chat_log_db_ok.fetch_add(BATCH_EXEC_OK, std::memory_order_relaxed);
    GMetrics.chat_log_db_fail.fetch_add(BATCH_EXEC_FAIL, std::memory_order_relaxed);
    GMetrics.db_unbind_calls.fetch_add((jobsProcessed + MAX_JOBS_PER_CHUNK - 1) / MAX_JOBS_PER_CHUNK); // 1 per chunk

    GDBConnectionPool->Push(dbConn);
    GMetrics.conn_pool_push_total.fetch_add(1, std::memory_order_relaxed);
}