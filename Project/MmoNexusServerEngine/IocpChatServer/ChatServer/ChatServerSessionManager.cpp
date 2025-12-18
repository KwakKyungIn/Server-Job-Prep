#include "pch.h"
#include "ChatServerSessionManager.h"
#include "GameSession.h"

std::mutex ChatServerSessionManager::_lock;
std::unordered_map<uint64, std::weak_ptr<GameSession>> ChatServerSessionManager::_sessions;

void ChatServerSessionManager::Add(const std::shared_ptr<GameSession>& session)
{
	if (!session) return;

	std::lock_guard<std::mutex> guard(_lock);
	_sessions[session->GetSessionId()] = session;
}

void ChatServerSessionManager::Remove(uint64 sessionId)
{
	std::lock_guard<std::mutex> guard(_lock);
	_sessions.erase(sessionId);
}

void ChatServerSessionManager::Broadcast(const SendBufferRef& sendBuffer)
{
	if (!sendBuffer) return;

	std::lock_guard<std::mutex> guard(_lock);

	for (auto it = _sessions.begin(); it != _sessions.end(); )
	{
		auto sp = it->second.lock();
		if (!sp)
		{
			it = _sessions.erase(it);
			continue;
		}

		sp->Send(sendBuffer);
		++it;
	}
}
