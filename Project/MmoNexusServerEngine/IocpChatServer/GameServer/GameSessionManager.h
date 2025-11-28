#pragma once
#include "PlayerSession.h"
#include <map>

class GameSessionManager
{
public:
	// [Singleton]
	static GameSessionManager* GSessionManager;

	void Add(PlayerSessionRef session);
	void Remove(PlayerSessionRef session);
	void Broadcast(SendBufferRef sendBuffer);

	// [Key Feature] ID로 특정 유저 찾기 (S2S 응답 처리용)
	// Map을 쓰므로 O(logN) 속도로 빠르게 찾는다.
	PlayerSessionRef Find(uint64 id);

private:
	// [Lock] 네가 사용하는 Lock 규격 (pch.h에 정의되어 있다고 가정)
	USE_LOCK;

	// [Storage] 검색(Find)을 위해 Map 사용 (SessionID -> Session)
	std::map<uint64, PlayerSessionRef> _sessions;
};

extern GameSessionManager* GSessionManager;