#pragma once
#include "DBConnection.h"
#include <queue>
#include <condition_variable>

/*-------------------
	DBConnectionPool
--------------------*/

class DBConnectionPool
{
public:
	DBConnectionPool() = default;
	~DBConnectionPool();

	bool Connect(int32 connectionCount, const WCHAR* connectionString);
	void Clear();

	DBConnection* Pop();                // 커넥션 하나 빌림 (없으면 대기)
	void Push(DBConnection* connection); // 커넥션 반환

private:
	SQLHENV _environment = SQL_NULL_HANDLE;
	std::queue<DBConnection*> _connections;

	std::mutex _mutex;
	std::condition_variable _cv;
	bool _shutdown = false;
};
