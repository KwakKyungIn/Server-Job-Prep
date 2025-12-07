#include "pch.h"
#include "GameSessionManager.h"
#include "PlayerSession.h" // 실제 구현부에서는 include 필요

// 전역 인스턴스 초기화
GameSessionManager* GameSessionManager::GSessionManager = new GameSessionManager();

void GameSessionManager::Add(PlayerSessionRef session)
{
	WRITE_LOCK; // 쓰기 락
	_sessions.insert({ session->GetSessionId(), session });
}

void GameSessionManager::Remove(PlayerSessionRef session)
{
	WRITE_LOCK; // 쓰기 락
	_sessions.erase(session->GetSessionId());
}

void GameSessionManager::Broadcast(SendBufferRef sendBuffer)
{
	READ_LOCK; // 읽기 락 (동시 접속자들에게 뿌리기만 하므로)
	for (auto& item : _sessions)
	{
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