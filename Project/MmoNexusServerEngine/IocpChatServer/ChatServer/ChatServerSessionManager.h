#pragma once
#include "pch.h"
#include <unordered_map>
#include <mutex>

class GameSession;

class ChatServerSessionManager
{
public:
	static void Add(const std::shared_ptr<GameSession>& session);
	static void Remove(uint64 sessionId);

	// GameServer들로 브로드캐스트
	static void Broadcast(const SendBufferRef& sendBuffer);

private:
	static std::mutex _lock;
	static std::unordered_map<uint64, std::weak_ptr<GameSession>> _sessions;
};
