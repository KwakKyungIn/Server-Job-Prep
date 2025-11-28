#include "pch.h"
#include "GameSessionManager.h"

// 전역 인스턴스 생성
GameSessionManager* GameSessionManager::GSessionManager = new GameSessionManager();

void GameSessionManager::Add(PlayerSessionRef session)
{
	WRITE_LOCK; // 쓰기 락 (동시성 보장)

	// Map에 ID를 키값으로 저장
	// session->GetSessionId()는 이제 Base Session 클래스에서 상속받은 걸 쓴다.
	_sessions.insert({ session->GetSessionId(), session });
}

void GameSessionManager::Remove(PlayerSessionRef session)
{
	WRITE_LOCK; // 쓰기 락

	_sessions.erase(session->GetSessionId());
}

void GameSessionManager::Broadcast(SendBufferRef sendBuffer)
{
	READ_LOCK; // 읽기 락 (Broadcast는 읽기만 하므로 READ_LOCK이 성능상 유리)

	for (auto& item : _sessions)
	{
		// item.second가 PlayerSessionRef
		item.second->Send(sendBuffer);
	}
}

PlayerSessionRef GameSessionManager::Find(uint64 id)
{
	READ_LOCK; // 읽기 락

	auto it = _sessions.find(id);
	if (it == _sessions.end())
		return nullptr;

	return it->second;
}