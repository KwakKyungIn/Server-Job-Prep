#pragma once
#include "JobQueue.h"
#include "Job.h"
#include "Session.h"

class DBAgentSession : public PacketSession
{
public:
	DBAgentSession()
	{
		_jobQueue = MakeShared<JobQueue>();
	}
	virtual ~DBAgentSession() {}

	virtual void OnConnected() override;
	virtual void OnDisconnected() override;
	virtual void OnRecvPacket(BYTE* buffer, int32 len) override;
	virtual void OnSend(int32 len) override;

	void PushJob(shared_ptr<Job> job)
	{
		_jobQueue->Push(job);
	}

public:
	shared_ptr<JobQueue> _jobQueue;
};