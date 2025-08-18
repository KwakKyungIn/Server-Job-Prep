#include "pch.h"
#include <iostream>
#include "CorePch.h"
#include <atomic> // atomic은 멀티스레드 환경에서 안전하게 변수를 공유하기 위해 사용합니다.
#include <thread> //윈도우즈를 가져오면 리눅스 환경에서는 안될수도있으니까
#include <mutex>	 // mutex는 스레드 간의 동기화를 위해 사용합니다.
#include <windows.h> // Windows API를 사용하기 위해 포함합니다.
#include <future>
#include "ConcurrentQueue.h"
#include "ConcurrentStack.h"
LockQueue<int32> q;
LockStack<int32> s;

void Push()
{
	while(true) // 무한 루프를 돌면서
	{
		int32 value = rand() % 100; // 0부터 99까지의 랜덤한 값을 생성합니다.
		q.Push(value); // 스택에 값을 푸시합니다.
		this_thread::sleep_for(chrono::milliseconds(10)); // 100ms 대기합니다.
	}
}

void Pop()
{
		while(true) // 무한 루프를 돌면서
	{
		
			int32 data = 0;
			if (q.TryPop(OUT data))
				cout << data << endl;// 큐에서 값을 꺼내려고 시도합니다.
		
		}
}



int main()
{


	thread t1(Push); 
	thread t2(Pop);
	thread t3(Pop); 




	t1.join(); // 생산자 스레드가 종료될 때까지 기다립니다.
	t2.join(); // 소비자 스레드가 종료될 때까지 기다립니다.


}
