#pragma once
#include "Session.h"
#include "JobQueue.h"
#include "Job.h"

class LoginSession : public PacketSession
{
public:
	LoginSession()
	{
		_jobQueue = MakeShared<JobQueue>();
	}
	virtual ~LoginSession() {}

	virtual void OnConnected() override;
	virtual void OnDisconnected() override;
	virtual void OnRecvPacket(BYTE* buffer, int32 len) override;
	virtual void OnSend(int32 len) override;

	void PushJob(shared_ptr<Job> job) { _jobQueue->Push(job); }

public:
	shared_ptr<JobQueue> _jobQueue;
};