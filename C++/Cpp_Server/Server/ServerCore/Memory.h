#pragma once
#include "Allocator.h"

template<typename Type, typename... Args>
Type* Xnew(Args&&... args)
{
	Type* memory = static_cast<Type*>(Xalloc(sizeof(Type)));
	new(memory)Type(forward<Args>(args)...); // placement new
	return memory;
}

template<typename Type>
void Xdelete(Type* obj)
{
	obj->~Type();
	Xrelease(obj);
}
