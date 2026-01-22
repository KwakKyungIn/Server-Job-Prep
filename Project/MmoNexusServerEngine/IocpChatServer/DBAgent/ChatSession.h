#pragma once
#include "Session.h"
#include "JobQueue.h" // [NEW]
#include "Job.h"  

class ChatSession : public PacketSession
{
public:
	ChatSession()
	{
		// [NEW] 엔진 장착
		_jobQueue = MakeShared<JobQueue>();
	}
	virtual ~ChatSession() {}

	virtual void OnConnected() override;
	virtual void OnDisconnected() override;
	virtual void OnRecvPacket(BYTE* buffer, int32 len) override;
	virtual void OnSend(int32 len) override;

	void PushJob(shared_ptr<Job> job)
	{
		_jobQueue->Push(job);
	}

public:
	// [NEW] 이 세션(GameServer)에서 오는 요청을 처리할 큐
	shared_ptr<JobQueue> _jobQueue;
};