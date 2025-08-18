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
	void WriteLock();
	void WriteUnlock();
	void ReadLock();
	void ReadUnlock();

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
	ReadLockGuard(Lock& lock) : _lock(lock) {_lock.ReadLock();}
	~ReadLockGuard() { _lock.ReadUnlock(); }

private:
	Lock& _lock;

};

class WriteLockGuard
{
public:
	WriteLockGuard(Lock& lock) : _lock(lock) { _lock.WriteLock(); }
	~WriteLockGuard() { _lock.WriteUnlock(); }

private:
	Lock& _lock;

};
