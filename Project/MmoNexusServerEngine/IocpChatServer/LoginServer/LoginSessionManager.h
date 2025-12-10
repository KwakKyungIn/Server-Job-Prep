#pragma once
#include <map>
#include "ClientSession.h"

class LoginSessionManager
{
public:
	static LoginSessionManager* GSessionManager; // 편의상 이름 통일

	void Add(shared_ptr<ClientSession> session);
	void Remove(shared_ptr<ClientSession> session);
	shared_ptr<ClientSession> Find(uint64 id);

private:
	USE_LOCK;
	std::map<uint64, shared_ptr<ClientSession>> _sessions;
};

extern LoginSessionManager* GSessionManager;