#pragma once
#include <map>

class PlayerSession;
using PlayerSessionRef = std::shared_ptr<PlayerSession>;

class GameSessionManager
{
public:
	// [Singleton Pattern]
	static GameSessionManager* GSessionManager;

	void Add(PlayerSessionRef session);
	void Remove(PlayerSessionRef session);
	void Broadcast(SendBufferRef sendBuffer);

	// ID로 특정 유저 찾기 (귓속말 등에 사용)
	PlayerSessionRef Find(uint64 id);

private:
	// RW SpinLock 사용 (대부분 Read, 가끔 Write)
	USE_LOCK;

	// SessionId(또는 PlayerId) -> Session 매핑
	std::map<uint64, PlayerSessionRef> _sessions;
};

extern GameSessionManager* GSessionManager;