#pragma once
#include "Types.h"

/*--------------------------
*         RW SpinLock
------------------------------*/
class Lock
{
	enum : uint32
	{
		ACQUIRE_TIMEOUT_TICK = 10000, // 1초
		MAX_SPIN_COUNT = 5000, // 최대 스핀 횟수,
		WRITE_THREAD_MASK = 0xFFFF'0000, // 쓰기 스레드 마스크,
		READ_COUNT_MASK = 0x0000'FFFF, // 읽기 스레드 마스크
		EMPTY_FLAG = 0x0000'0000, // 비어있는 상태
	};

public:
	void WriteLock(const char* name);
	void WriteUnlock(const char* name);
	void ReadLock(const char* name);
	void ReadUnlock(const char* name);

private:
	Atomic<uint32> _lockFlag = EMPTY_FLAG; // Lock 상태를 저장하는 변수
	uint16 _writeCount = 0; // 쓰기 스레드의 개수
};


/*--------------------------
*         LockGuard
------------------------------*/

class ReadLockGuard
{
public:
	ReadLockGuard(Lock& lock, const char* name) : _lock(lock), _name(name) {_lock.ReadLock(name);}
	~ReadLockGuard() { _lock.ReadUnlock(_name); }

private:
	Lock& _lock;
	const char* _name;
};

class WriteLockGuard
{
public:
	WriteLockGuard(Lock& lock, const char* name) : _lock(lock), _name(name){ _lock.WriteLock(name); }
	~WriteLockGuard() { _lock.WriteUnlock(_name); }

private:
	Lock& _lock;
	const char* _name;
};
