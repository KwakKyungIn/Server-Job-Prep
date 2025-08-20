#pragma once
class RefCountable
{

public:
	RefCountable() : _refCount(1) {}
	virtual ~RefCountable() {}

	int32 GetRefCount() { return _refCount; }

	int32 AddRef() {return ++_refCount;}

	int32 ReleaseRef()
	{
		int32 refCount = --_refCount;
		if (refCount == 0)
		{
			delete this;
			return 0;
		}

		return refCount;
	}

	

protected:
	int32 _refCount;

};

