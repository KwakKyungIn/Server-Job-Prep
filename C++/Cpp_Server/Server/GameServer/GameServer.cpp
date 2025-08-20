#include "pch.h"
#include <iostream>
#include "CorePch.h"
#include <atomic>
#include <mutex>
#include <windows.h>
#include <future>
#include "ThreadManager.h"
#include <vector>
#include <map>

#include "RefCounting.h"
#include "Memory.h"
#include "Allocator.h"
class Knight
{
public:
	Knight()
	{
		cout << "Knight()" << endl;
	}

	Knight(int32 hp) : _hp(hp)
	{
		cout << "Knight(hp)" << endl;
	}

	~Knight()
	{
		cout << "~Knight()" << endl;
	}


	int32 _hp = 100;
	int32 _mp = 10;
};



int main()
{
	Vector<Knight> v(100);

}
