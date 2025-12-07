#pragma once
#include "pch.h"
#include "Session.h"
#include "JobQueue.h" // [NEW]
#include "Job.h"      // [NEW]

class GameSession : public PacketSession
{
public:
	GameSession()
	{
		// [NEW] 엔진 장착
		_jobQueue = MakeShared<JobQueue>();
	}
	virtual ~GameSession() {}

	virtual void OnConnected() override;
	virtual void OnDisconnected() override;
	virtual void OnRecvPacket(BYTE* buffer, int32 len) override;
	virtual void OnSend(int32 len) override;

	// [NEW] 일감 투입구
	void PushJob(shared_ptr<Job> job)
	{
		_jobQueue->Push(job);
	}

public:
	// [NEW] 이 세션 전용 큐
	shared_ptr<JobQueue> _jobQueue;
};