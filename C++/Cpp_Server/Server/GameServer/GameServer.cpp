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
		while (_locked)
		{

		}

		_locked = true; // 스핀락을 획득합니다.
	}
	void unlock()
	{
		_locked = false; // 스핀락을 해제합니다.
	}


private:
	bool _locked = false;
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
