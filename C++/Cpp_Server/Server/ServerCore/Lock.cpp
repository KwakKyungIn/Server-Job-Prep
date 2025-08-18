#include "pch.h"
#include "Lock.h"
#include "CoreTLS.h"

void Lock::WriteLock()
{
	const uint32 lockThreadId = (_lockFlag.load() & WRITE_THREAD_MASK) >> 16;
	if(LThreadId == lockThreadId)
	{
		// 이미 쓰기 잠금을 획득한 스레드가 다시 쓰기 잠금을 시도하는 경우
		_writeCount++;
		return; // 성공적으로 쓰기 잠금 획득
	}

	const int64 beginTick = ::GetTickCount64();

	//아무도 소유및 공유하고 있지 않을 때, 경합해서 소유권을 얻는다.
	const uint32 desired = ((LThreadId << 16) & WRITE_THREAD_MASK);
	while (true)
	{
		for(uint32 spinCount=0; spinCount < MAX_SPIN_COUNT; ++spinCount)
		{
			uint32 expected = EMPTY_FLAG;
	
			if (_lockFlag.compare_exchange_strong(OUT expected, desired))
			{
				_writeCount++;
				return; // 성공적으로 쓰기 잠금 획득
			}

		}

		if(::GetTickCount64() - beginTick > ACQUIRE_TIMEOUT_TICK)
		{
			CRASH("LOCK_TIMEOUT");
		}


		this_thread::yield(); // 스레드 양보
	}
}

void Lock::WriteUnlock()
{
	//ReadLock 다 풀기전에는 WriteUnlock을 호출할 수 없다.
	if((_lockFlag.load() & READ_COUNT_MASK)!=0)
	{
		CRASH("INVALID_UNLOCK_ORDER");
	}

	const int32 lockCount = --_writeCount;
	if(lockCount == 0)
		_lockFlag.store(EMPTY_FLAG); // 쓰기 잠금 해제
}

void Lock::ReadLock()
{

	const uint32 lockThreadId = (_lockFlag.load() & WRITE_THREAD_MASK) >> 16;
	if (LThreadId == lockThreadId)
	{
		_lockFlag.fetch_add(1); // 이미 쓰기 잠금을 획득한 스레드가 읽기 잠금을 시도하는 경우
		return; // 성공적으로 쓰기 잠금 획득
	}

	const int64 beginTick = ::GetTickCount64();
	while (true)
	{
		for (uint32 spinCount = 0; spinCount < MAX_SPIN_COUNT; ++spinCount)
		{
			uint32 expected = (_lockFlag.load() & READ_COUNT_MASK); 
			if (_lockFlag.compare_exchange_strong(expected, expected + 1))
				return;
		}

		if (::GetTickCount64() - beginTick > ACQUIRE_TIMEOUT_TICK)
		{
			CRASH("LOCK_TIMEOUT");
		}


		this_thread::yield(); // 스레드 양보
	}
}

void Lock::ReadUnlock()
{
	if ((_lockFlag.fetch_sub(1) && READ_COUNT_MASK) == 0)
		CRASH("MULTIPLE_UNLOCK");
}
