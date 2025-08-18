#include "pch.h"
#include <iostream>
#include "CorePch.h"
#include <atomic> // atomic은 멀티스레드 환경에서 안전하게 변수를 공유하기 위해 사용합니다.
#include <thread> //윈도우즈를 가져오면 리눅스 환경에서는 안될수도있으니까
#include <mutex>	 // mutex는 스레드 간의 동기화를 위해 사용합니다.
#include <windows.h> // Windows API를 사용하기 위해 포함합니다.
#include <future>
#include "ThreadManager.h"

CoreGlobal Core; // CoreGlobal 객체를 생성합니다. 이 객체는 스레드 매니저를 초기화하고 관리합니다.


void ThreadMain()
{
	while (true)
	{
		cout << "Hello, World! from Thread ID: " << LThreadId << endl;
		this_thread::sleep_for(chrono::seconds(1)); // 1초 동안 대기합니다.

	}

}
int main()
{
	for(int32 i = 0; i < 5; ++i)
	{
		GThreadManager->Launch(ThreadMain); // 스레드를 생성하고 ThreadMain 함수를 실행합니다.
	}
	GThreadManager->Join(); // 모든 스레드가 종료될 때까지 대기합니다.

}
