#pragma once
#include "Types.h"
#include <string>
#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <memory>

struct DbLogJob
{
	DbLogJob(uint64 pId, const std::string& pName, const std::string& msg, int64 ts)
		: playerId(pId), playerName(pName), message(msg), timestamp(ts)
	{
	}

	uint64 playerId;
	std::string playerName;
	std::string message;
	int64 timestamp;
};

class DbLogger
{
public:
	DbLogger();
	~DbLogger();

	// GIGACHAD FIX: 쓰레드 개수를 정할 수 있게 파라미터 추가
	void Start(int32 threadCount = 2);
	void Stop();

	void Push(uint64 playerId, const std::string& playerName, const std::string& message);

private:
	void WorkerThread();
	void ExecuteBatch(std::vector<DbLogJob*>& jobs);

private:
	std::atomic<bool> _running = false;

	// GIGACHAD FIX: 1개가 아니라 여러 개의 쓰레드를 관리
	std::vector<std::thread> _workers;
	std::mutex _mutex;
	std::condition_variable _cv;
	std::queue<DbLogJob*> _queue;
};

extern DbLogger* GDbLogger;