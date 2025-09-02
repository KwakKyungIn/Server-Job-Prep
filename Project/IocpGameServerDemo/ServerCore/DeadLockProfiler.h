#pragma once
#include <vector>
#include <map>
#include <stack>


/*-----------------------------------------
* DeadLockProfiler
----------------------------------------------*/
class DeadLockProfiler
{
public:
	void PushLock(const char* name);
	void PopLock(const char* name);
	void CheckCycle();

private:
	void Dfs(int32 index);

private:
	unordered_map<const char*, int32> _nameToId;
	unordered_map<int32, const char*> _idToName;
	map<int32, set<int32>> _lockHistory;

	Mutex _lock;

private:
	vector<int32>		_discoveredOrder;//노드가 발견된 순서를 저장하는 배열
	int32				_discoveredCount = 0; //노드가 발견된 순서
	vector<bool>		_finished; //노드가 끝났는지 여부를 저장하는 배열
	vector<int32>		_parent; //부모 노드의 인덱스를 저장하는 배열


};

