#pragma once
#include "Job.h"
#include "Lock.h"

/*----------------
    JobQueue
-----------------*/

class JobQueue : public enable_shared_from_this<JobQueue>
{
public:
    void Push(shared_ptr<Job> job);
    void Execute();

private:
    // 네가 가진 Lock 사용
    USE_LOCK;
    queue<shared_ptr<Job>> _jobs;

    // 현재 이 큐가 GlobalQueue에 들어가서 처리 대기중인지?
    atomic<bool> _posted = false;
};