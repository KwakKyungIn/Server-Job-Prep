#pragma once
#include "JobQueue.h"
#include "Job.h"
#include "Session.h"
#include <string>

class ClientSession : public PacketSession
{
public:
	ClientSession()
	{
		_jobQueue = MakeShared<JobQueue>();
	}
	virtual ~ClientSession() {}

	virtual void OnConnected() override;
	virtual void OnDisconnected() override;
	virtual void OnRecvPacket(BYTE* buffer, int32 len) override;
	virtual void OnSend(int32 len) override;

	void PushJob(shared_ptr<Job> job)
	{
		_jobQueue->Push(job);
	}

public:
	void SetLoginName(const std::string& name) { _loginName = name; }
	const std::string& GetLoginName() const { return _loginName; }
	void ClearLoginName() { _loginName.clear(); }

public:
	shared_ptr<JobQueue> _jobQueue;
	std::string _loginName;
};
