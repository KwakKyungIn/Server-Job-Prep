#pragma once

/*-------------------
	BaseAllocator
-------------------*/

class BaseAllocator
{
public:
	static void*	Alloc(int32 size);
	static void		Release(void* ptr);
};

/*-----------------------
StompAllocator----> 메모리 오염 잡기 위해!
-----------------------*/

class StompAllocator
{
	enum { PAGE_SIZE = 0x1000 };
public:
	static void*	Alloc(int32 size);
	static void		Release(void* ptr);

};

/*-----------------------
STL Allocator----> 메모리 오염 잡기 위해!
-----------------------*/
template <typename T>
class StlAllocator
{
public:
	using value_type = T;

	StlAllocator() {}

	template <typename Other>
	StlAllocator(const StlAllocator<Other>&) {}

	T* allocate(size_t count)
	{
		const int32 size = static_cast<int32>(sizeof(T) * count);
		return static_cast<T*>(Xalloc(size));
	}

	void deallocate(T* ptr, size_t count)
	{
		Xrelease(ptr);
	}


};
