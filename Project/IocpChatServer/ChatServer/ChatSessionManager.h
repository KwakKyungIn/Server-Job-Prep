#pragma once

class ChatSession;

using ChatSessionRef = shared_ptr<ChatSession>;

class ChatSessionManager
{
public:
	void Add(ChatSessionRef session);
	void Remove(ChatSessionRef session);
	void Broadcast(SendBufferRef sendBuffer);

private:
	USE_LOCK;
	Set<ChatSessionRef> _sessions;
};

extern ChatSessionManager GSessionManager;
