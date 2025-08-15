#include "pch.h"
#include <iostream>
#include "CorePch.h"
#include <atomic> // atomic은 멀티스레드 환경에서 안전하게 변수를 공유하기 위해 사용합니다.

#include <thread> //윈도우즈를 가져오면 리눅스 환경에서는 안될수도있으니까
#include <mutex>	 // mutex는 스레드 간의 동기화를 위해 사용합니다.
vector<int32> v;
mutex m; // 벡터에 접근할 때 사용할 뮤텍스입니다.

template<typename T>
class LockGuard
{
	public:
		LockGuard(T& m)
		{
			_mtx = m; // 생성자에서 뮤텍스를 잠급니다.
			_mtx-> lock(); // 뮤텍스를 잠급니다.
		}
		
	~LockGuard() 
	{ 
		_mtx->unlock();// 소멸자에서 뮤텍스를 해제합니다.

	} 

	private:
	T& _mtx; // 뮤텍스 참조
};

void Push()
{
	for (int32 i = 0; i < 10000; ++i)
	{
		LockGuard<mutex> lockguard(m); // LockGuard를 사용하여 뮤텍스를 잠급니다.
		v.push_back(i);

	}
}


int main()
{
	v.reserve(20000); // 벡터의 크기를 미리 예약합니다. 이로 인해 동적 할당이 줄어들어 성능이 향상됩니다.
	thread t1(Push);
	thread t2(Push);
	t1.join(); // t1 스레드가 종료될 때까지 기다립니다.
	t2.join(); // t2 스레드가 종료될 때까지 기다립니다.
	cout << "Size of vector: " << v.size() << endl;

}
