#include "pch.h"
#include <iostream>
#include "CorePch.h"

#include <thread> //윈도우즈를 가져오면 리눅스 환경에서는 안될수도있으니까

void HelloThread()
{
	cout << "Hello Thread" << endl;
}

int main()
{
	std::thread t;
	auto id1 = t.get_id(); //쓰레드마다 id를 줌
	t = std::thread(HelloThread);


	int32 count = t.hardware_concurrency();
	auto id2 = t.get_id(); //쓰레드마다 id를 줌


	t.detach(); //스레드가 끝나면 자동으로 자원을 해제한다.
	if (t.joinable())
		t.join();//스레드가 join()을 호출할 수 있는지 여부를 반환한다.


	cout << "Hello World"<< count << endl;
}
