#pragma once
#include "JobQueue.h"
#include "Protocol.pb.h"

class GameMap;

// [Refactoring Note] 나중에 Types.h 등으로 이동 권장
using GameMapRef = std::shared_ptr<GameMap>;

class GameRoom : public enable_shared_from_this<GameRoom>
{
public:
	GameRoom();
	virtual ~GameRoom();

	void Init(int32 mapId, int32 sizeX, int32 sizeY);
	void Update();

	// [Interface] 외부에서 Job을 던지는 통로
	template<typename F, typename... Args>
	void PushJob(F func, Args&&... args)
	{
		_jobQueue->Push(MakeShared<Job>(shared_from_this(), func, std::forward<Args>(args)...));
	}

public:
	// [Content Logic] JobQueue에 의해 순차 실행됨 (No Lock needed)
	void Enter(PlayerSessionRef session);
	void Leave(PlayerSessionRef session);
	void HandleMove(PlayerSessionRef session, Protocol::C_MOVE pkt);

	// [Getter]
	GameMapRef GetMap() { return _map; }

private:
	void Broadcast(SendBufferRef sendBuffer, uint64 exceptId = 0);

private:
	shared_ptr<GameMap> _map;
	shared_ptr<JobQueue> _jobQueue;
	Map<uint64, PlayerSessionRef> _sessions;
};