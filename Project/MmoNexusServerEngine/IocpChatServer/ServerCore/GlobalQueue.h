#pragma once

/*-------------------
    GlobalQueue
--------------------*/

class JobQueue; // 전방 선언

class GlobalQueue
{
public:
    GlobalQueue();
    ~GlobalQueue();

    void Push(shared_ptr<JobQueue> jobQueue);
    shared_ptr<JobQueue> Pop();

private:
    USE_LOCK;
    // [Lock Sharding]
    // 하나의 큰 큐 대신, 여러 개의 작은 큐로 쪼개서 경합을 분산시킴
    // 일반적으로 CPU 코어 수 이상으로 설정
    vector<class LockQueue*> _jobQueues;
};

extern GlobalQueue* GGlobalQueue;