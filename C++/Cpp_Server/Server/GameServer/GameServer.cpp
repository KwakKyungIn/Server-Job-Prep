#include "pch.h"
#include <iostream>
#include "CorePch.h"
#include <atomic> // atomic은 멀티스레드 환경에서 안전하게 변수를 공유하기 위해 사용합니다.
#include <thread> //윈도우즈를 가져오면 리눅스 환경에서는 안될수도있으니까
#include <mutex>	 // mutex는 스레드 간의 동기화를 위해 사용합니다.
#include <windows.h> // Windows API를 사용하기 위해 포함합니다.

mutex m;
queue<int32> q;
HANDLE handle;

void Producer()
{
	while (true)
	{
		{
			unique_lock<mutex> lock(m); // mutex를 잠급니다.
			q.push(100); // 큐에 데이터를 추가합니다.
		}

		::SetEvent(handle); // 이벤트를 설정합니다. (이벤트를 신호 상태로 변경합니다.)
		this_thread::sleep_for(std::chrono::milliseconds(1000)); // 100ms 대기합니다.
	}
}

void Consumer()
{
	while (true)
	{
		{
			::WaitForSingleObject(handle, INFINITE); // 이벤트가 신호 상태가 될 때까지 대기합니다.

			unique_lock<mutex> lock(m); // mutex를 잠급니다.
			if(q.empty()==false) // 큐가 비어있지 않으면
			{
				int32 data = q.front(); // 큐의 앞에서 데이터를 가져옵니다.
				q.pop(); // 큐에서 데이터를 제거합니다.
				cout << "Consumed: " << data << endl; // 소비된 데이터를 출력합니다.
			}
		}
	}
}

int main()
{
	//커널 오브젝트
	//usage count
	//signal, non-signal << bool
	//auto manual
	handle = ::CreateEvent(NULL/*보안 속성*/, FALSE/*bManualReset*/, FALSE/*bInitialState*/, NULL/*name*/); // 이벤트를 생성합니다. 


	thread t1(Producer); // 생산자 스레드를 생성합니다.
	thread t2(Consumer); // 소비자 스레드를 생성합니다.
	t1.join(); // 생산자 스레드가 종료될 때까지 기다립니다.
	t2.join(); // 소비자 스레드가 종료될 때까지 기다립니다.

	::CloseHandle(handle); // 이벤트 핸들을 닫습니다.

}
