#include "pch.h"
#include "DBConnectionPool.h"
#include <iostream>
#include <chrono>        // [추가]
#include "Metrics.h"     // [METRICS]

/*-------------------
    DBConnectionPool
--------------------*/

DBConnectionPool::~DBConnectionPool()
{
    Clear();
}

bool DBConnectionPool::Connect(int32 connectionCount, const WCHAR* connectionString)
{
    std::unique_lock<std::mutex> lock(_mutex);

    if (::SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &_environment) != SQL_SUCCESS)
    {
        std::wcout << L"SQLAllocHandle(SQL_HANDLE_ENV) failed." << std::endl;
        return false;
    }

    if (::SQLSetEnvAttr(_environment, SQL_ATTR_ODBC_VERSION,
        reinterpret_cast<SQLPOINTER>(SQL_OV_ODBC3), 0) != SQL_SUCCESS)
    {
        std::wcout << L"SQLSetEnvAttr failed." << std::endl;
        return false;
    }

    for (int32 i = 0; i < connectionCount; i++)
    {
        DBConnection* connection = xnew<DBConnection>();
        if (connection->Connect(_environment, connectionString) == false)
        {
            std::wcout << L"DBConnection::Connect failed." << std::endl;
            xdelete(connection);
            return false;
        }

        _connections.push(connection);
    }

    // [METRICS] 풀 사이즈 게이지 업데이트
    GMetrics.conn_pool_size.store(static_cast<uint32_t>(_connections.size()), std::memory_order_relaxed);

    return true;
}

void DBConnectionPool::Clear()
{
    std::unique_lock<std::mutex> lock(_mutex);
    _shutdown = true;
    _cv.notify_all(); // Pop 대기 중인 스레드 깨우기

    while (!_connections.empty())
    {
        DBConnection* conn = _connections.front();
        _connections.pop();
        xdelete(conn);
    }

    if (_environment != SQL_NULL_HANDLE)
    {
        ::SQLFreeHandle(SQL_HANDLE_ENV, _environment);
        _environment = SQL_NULL_HANDLE;
    }

    // [METRICS] 풀 사이즈 0으로
    GMetrics.conn_pool_size.store(0, std::memory_order_relaxed);
}

DBConnection* DBConnectionPool::Pop()
{
    auto t0 = std::chrono::steady_clock::now(); // [METRICS] 대기시간 측정 시작

    std::unique_lock<std::mutex> lock(_mutex);
    _cv.wait(lock, [this] { return !_connections.empty() || _shutdown; });

    if (_shutdown)
        return nullptr;

    DBConnection* connection = _connections.front();
    _connections.pop();

    // [METRICS] pop 카운터/대기시간/사이즈
    auto waitMs = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - t0).count();
    GMetrics.conn_pool_pop_total.fetch_add(1, std::memory_order_relaxed);
    GMetrics.conn_pool_acquire_wait_ms_total.fetch_add(static_cast<uint64_t>(waitMs), std::memory_order_relaxed);
    GMetrics.conn_pool_size.store(static_cast<uint32_t>(_connections.size()), std::memory_order_relaxed);

    return connection;
}

void DBConnectionPool::Push(DBConnection* connection)
{
    {
        std::unique_lock<std::mutex> lock(_mutex);
        _connections.push(connection);
        // [METRICS] push 카운터/사이즈
        GMetrics.conn_pool_push_total.fetch_add(1, std::memory_order_relaxed);
        GMetrics.conn_pool_size.store(static_cast<uint32_t>(_connections.size()), std::memory_order_relaxed);
    }
    _cv.notify_one();
}
