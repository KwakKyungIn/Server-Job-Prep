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
		// [NEW] 엔진 장착
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

public:
	// [NEW] 이 세션(GameServer)에서 오는 요청을 처리할 큐
	shared_ptr<JobQueue> _jobQueue;
};