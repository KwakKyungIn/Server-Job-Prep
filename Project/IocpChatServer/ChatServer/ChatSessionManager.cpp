#include "pch.h"
#include "ChatSessionManager.h"
#include "ChatSession.h"

ChatSessionManager GSessionManager;

void ChatSessionManager::Add(ChatSessionRef session)
{
	WRITE_LOCK;
	_sessions.insert(session);
}

void ChatSessionManager::Remove(ChatSessionRef session)
{
	WRITE_LOCK;
	_sessions.erase(session);
}

void ChatSessionManager::Broadcast(SendBufferRef sendBuffer)
{
	WRITE_LOCK;
	for (ChatSessionRef session : _sessions)
	{
		session->Send(sendBuffer);
	}
}