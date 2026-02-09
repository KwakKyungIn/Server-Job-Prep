#include "pch.h"
#include "DBConnectionPool.h"
#include "DBAgentMetrics.h"
#include <iostream>
#include <chrono>

DBConnectionPool::~DBConnectionPool()
{
    // 객체 해제 시 풀 정리
    Clear();
}

bool DBConnectionPool::Connect(int32 connectionCount, const WCHAR* connectionString)
{
    std::unique_lock<std::mutex> lock(_mutex);
    _shutdown = false;
    _poolSize = 0;
    _inUse = 0;

    // ODBC 환경 핸들 생성
    if (::SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &_environment) != SQL_SUCCESS)
    {
        std::wcout << L"SQLAllocHandle(SQL_HANDLE_ENV) failed." << std::endl;
        return false;
    }

    // ODBC 버전 3.0으로 설정
    if (::SQLSetEnvAttr(_environment, SQL_ATTR_ODBC_VERSION,
        reinterpret_cast<SQLPOINTER>(SQL_OV_ODBC3), 0) != SQL_SUCCESS)
    {
        std::wcout << L"SQLSetEnvAttr failed." << std::endl;
        return false;
    }

    // 지정된 개수만큼 DB 연결 생성 후 풀에 저장
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

    _poolSize = connectionCount;
    const int32 poolSize = _poolSize;
    const int32 inUse = _inUse;
    lock.unlock();

    DBAgentMetrics::ObservePoolState(poolSize, inUse);

    return true;
}

void DBConnectionPool::Clear()
{
    std::unique_lock<std::mutex> lock(_mutex);
    _shutdown = true;

    // 대기 중인 Pop 스레드 깨우기
    _cv.notify_all();

    // 풀에 남은 연결 모두 삭제
    while (!_connections.empty())
    {
        DBConnection* conn = _connections.front();
        _connections.pop();
        xdelete(conn);
    }

    // 환경 핸들 해제
    if (_environment != SQL_NULL_HANDLE)
    {
        ::SQLFreeHandle(SQL_HANDLE_ENV, _environment);
        _environment = SQL_NULL_HANDLE;
    }

    _poolSize = 0;
    _inUse = 0;
    lock.unlock();

    DBAgentMetrics::ObservePoolState(0, 0);
}

DBConnection* DBConnectionPool::Pop()
{
    const auto waitStart = std::chrono::steady_clock::now();

    std::unique_lock<std::mutex> lock(_mutex);

    // 연결이 생기거나 종료될 때까지 대기
    _cv.wait(lock, [this] { return !_connections.empty() || _shutdown; });

    if (_shutdown)
        return nullptr;

    // 사용 가능한 연결 하나 꺼내기
    DBConnection* connection = _connections.front();
    _connections.pop();
    ++_inUse;

    const int32 poolSize = _poolSize;
    const int32 inUse = _inUse;
    lock.unlock();

    const double waitSeconds = std::chrono::duration<double>(
        std::chrono::steady_clock::now() - waitStart).count();
    DBAgentMetrics::ObservePoolWait(waitSeconds);
    DBAgentMetrics::ObservePoolState(poolSize, inUse);

    return connection;
}

void DBConnectionPool::Push(DBConnection* connection)
{
    if (connection == nullptr)
        return;

    int32 poolSize = 0;
    int32 inUse = 0;

    {
        std::unique_lock<std::mutex> lock(_mutex);

        // 사용한 연결을 다시 풀에 반납
        _connections.push(connection);
        if (_inUse > 0)
            --_inUse;

        poolSize = _poolSize;
        inUse = _inUse;

    }

    DBAgentMetrics::ObservePoolState(poolSize, inUse);

    // 대기 중인 Pop 스레드 깨우기
    _cv.notify_one();
}
