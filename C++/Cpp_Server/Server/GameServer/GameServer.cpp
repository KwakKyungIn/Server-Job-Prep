#include "pch.h"
#include <iostream>
#include "CorePch.h"
#include <atomic> // atomic은 멀티스레드 환경에서 안전하게 변수를 공유하기 위해 사용합니다.

#include <thread> //윈도우즈를 가져오면 리눅스 환경에서는 안될수도있으니까
#include <mutex>	 // mutex는 스레드 간의 동기화를 위해 사용합니다.
mutex m; // 벡터에 접근할 때 사용할 뮤텍스입니다.
int32 sum = 0;

class SpinLock
{
public:
	void lock()
	{
		//cas(Compare and Swap) 알고리즘을 사용하여 스핀락을 구현합니다.
		bool expected = false; // 스핀락이 잠겨있지 않은 상태를 나타냅니다.
		bool desired = true; // 스핀락을 잠글 상태를 나타냅니다.
		// compare_exchange_strong은 _locked가 expected와 같으면 desired로 변경하고 true를 반환합니다.
		// 그렇지 않으면 _locked의 현재 값을 expected에 저장하고 false를 반환합니다.

	
		while (_locked.compare_exchange_strong(expected, desired)==false)
		{
			expected = false; // expected를 false로 초기화합니다.
		}

	}
	void unlock()
	{
		_locked.store(false); // 스핀락을 해제합니다.
	}


private:
	atomic<bool> _locked = false;
};

SpinLock spinLock; // 스핀락 객체 생성

void Add()
{
	for(int32 i = 0; i < 1000000; ++i)
	{
		lock_guard<SpinLock> guard(spinLock); 
		sum++;
	}
}

void Sub()
{
	for (int32 i = 0; i < 1000000; ++i)
	{
		lock_guard<SpinLock> guard(spinLock);
		sum--;
	}
}

int main()
{
	thread t1(Add);
	thread t2(Sub);
	
	t1.join(); // t1 스레드가 종료될 때까지 기다립니다.
	t2.join(); // t2 스레드가 종료될 때까지 기다립니다.

	cout << "Final sum: " << sum << endl; // 최종 합계를 출력합니다.
}
