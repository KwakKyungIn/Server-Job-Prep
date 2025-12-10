#include "pch.h"
#include "LoginSessionManager.h"

LoginSessionManager* LoginSessionManager::GSessionManager = new LoginSessionManager();
LoginSessionManager* GSessionManager = nullptr;

void LoginSessionManager::Add(shared_ptr<ClientSession> session)
{
	WRITE_LOCK;
	_sessions.insert({ session->GetSessionId(), session });
}

void LoginSessionManager::Remove(shared_ptr<ClientSession> session)
{
	WRITE_LOCK;
	_sessions.erase(session->GetSessionId());
}

shared_ptr<ClientSession> LoginSessionManager::Find(uint64 id)
{
	READ_LOCK;
	auto it = _sessions.find(id);
	if (it == _sessions.end())
		return nullptr;
	return it->second;
}