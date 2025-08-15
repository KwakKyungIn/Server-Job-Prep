#include "pch.h"
#include <iostream>
#include "CorePch.h"
#include <atomic> // atomic은 멀티스레드 환경에서 안전하게 변수를 공유하기 위해 사용합니다.

#include <thread> //윈도우즈를 가져오면 리눅스 환경에서는 안될수도있으니까

atomic<int32> sum = 0;
void Add()
{
	for (int32 i = 0; i < 1000000; ++i)
	{
		sum += i;
	}
}
void Sub()
{
	for (int32 i = 0; i < 1000000; ++i)
	{
		sum -= i;
	}
}

int main()
{
	Add();
	Sub();
	cout << "Sum: " << sum << endl;

	std::thread t1(Add);
	std::thread t2(Sub);
	t1.join();
	t2.join();
	cout << "Final Sum: " << sum << std::endl;
}
