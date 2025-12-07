#include "pch.h"
#include "GlobalQueue.h"
#include "JobQueue.h"

// ==================================================
// LockQueue
// - GlobalQueue 내부에서 사용할 개별 락 큐
// - 외부 노출 없이 cpp 내부에서만 사용 (Helper Class)
// ==================================================
class LockQueue
{
public:
    void Push(shared_ptr<JobQueue> jobQueue)
    {
        WRITE_LOCK;
        _items.push(jobQueue);
    }

    shared_ptr<JobQueue> Pop()
    {
        WRITE_LOCK;
        if (_items.empty())
            return nullptr;

        shared_ptr<JobQueue> ret = _items.front();
        _items.pop();
        return ret;
    }

    // 락 없이 빈 여부만 확인 (Work Stealing 시 힌트용)
    // 100% 정확하지 않아도 되므로 락 생략 가능 (최적화)
    bool Empty() { return _items.empty(); }

private:
    USE_LOCK; // 개별 락
    queue<shared_ptr<JobQueue>> _items;
};

// ==================================================
// GlobalQueue
// ==================================================

GlobalQueue::GlobalQueue()
{
    // 락 경합을 줄이기 위해 큐를 여러 개 생성 (Sharding)
    // 예: 4코어 기준 4~8개 정도면 충분하지만, 넉넉하게 32개 생성
    // (메모리 오버헤드는 거의 없음)
    for (int32 i = 0; i < 32; i++)
    {
        _jobQueues.push_back(new LockQueue());
    }
}

GlobalQueue::~GlobalQueue()
{
    for (LockQueue* q : _jobQueues)
        delete q;
    _jobQueues.clear();
}

void GlobalQueue::Push(shared_ptr<JobQueue> jobQueue)
{
    // [Round Robin / Hashing]
    // 32개의 큐 중 하나를 골라서 넣음.
    // 여기서는 JobQueue 객체의 주소값을 해싱하여 인덱스를 결정.
    // (주소값은 랜덤하므로 자연스럽게 분산됨)
    int32 index = (reinterpret_cast<int32>(jobQueue.get()) >> 4) % _jobQueues.size();

    _jobQueues[index]->Push(jobQueue);
}

shared_ptr<JobQueue> GlobalQueue::Pop()
{
    // [Work Stealing Strategy]

    // 1. 내 스레드 ID(TLS)에 매핑된 큐를 먼저 확인 (Cache Locality)
    int32 index = LThreadId % _jobQueues.size();

    // 2. 내 담당 큐 먼저 뒤지기
    shared_ptr<JobQueue> jobQueue = _jobQueues[index]->Pop();
    if (jobQueue) return jobQueue;

    // 3. 내 큐가 비었으면, 다른 큐를 순회하며 훔쳐옴 (Stealing)
    // -> 한 스레드가 놀고 있을 때, 다른 바쁜 큐의 일감을 도와줌
    for (int32 i = 0; i < _jobQueues.size(); i++)
    {
        if (i == index) continue; // 내껀 이미 봤음

        // 락 걸기 전에 비어있는지 살짝 간만 봄 (성능 최적화)
        if (_jobQueues[i]->Empty()) continue;

        jobQueue = _jobQueues[i]->Pop();
        if (jobQueue) return jobQueue;
    }

    return nullptr;
}