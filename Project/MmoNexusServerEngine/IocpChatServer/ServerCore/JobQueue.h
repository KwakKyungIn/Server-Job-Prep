#pragma once
#include "Job.h"
#include "Lock.h"

class JobQueue : public enable_shared_from_this<JobQueue>
{
public:
    void Push(shared_ptr<Job> job);
    void Execute();

private:
    USE_LOCK;
    queue<shared_ptr<Job>> _jobs;
    Atomic<bool> _posted = false;
    Atomic<int64> _depth = 0;
};
