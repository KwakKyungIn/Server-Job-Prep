#include "pch.h"
#include "DBAgentPacketHandler.h"
#include "JobQueue.h" // [NEW]
#include "Job.h"      // [NEW]

// GameServer와의 연결을 관리하는 세션
class GameSession : public PacketSession
{
public:
	GameSession()
	{
		// 우선순위 큐: LOAD 계열 요청은 high 큐로 분리해 SAVE 버스트의 영향을 줄인다.
		_highJobQueue = MakeShared<JobQueue>();
		_jobQueue = MakeShared<JobQueue>();
	}
	virtual ~GameSession() {}

	virtual void OnConnected() override;
	virtual void OnDisconnected() override;
	virtual void OnRecvPacket(BYTE* buffer, int32 len) override;
	virtual void OnSend(int32 len) override;

	// [NEW] 패킷 핸들러가 일감을 던질 구멍
	void PushJob(shared_ptr<Job> job)
	{
		_jobQueue->Push(job);
	}

	void PushHighJob(shared_ptr<Job> job)
	{
		_highJobQueue->Push(job);
	}

public:
	// 우선순위 높은 요청(LOAD 계열)용 큐
	shared_ptr<JobQueue> _highJobQueue;
	// 일반 요청(SAVE 계열 등)용 큐
	shared_ptr<JobQueue> _jobQueue;
};
