#include "pch.h"
#include "GlobalQueue.h"
#include "JobQueue.h"
#include "MetricsSystem.h"

#include <cstdint>

namespace
{
    struct GlobalQueueMetrics
    {
        std::shared_ptr<Counter> pushCounter;
        std::shared_ptr<Gauge> depthGauge;
        std::shared_ptr<Counter> popCounter;
        std::shared_ptr<Counter> stealAttemptCounter;
        std::shared_ptr<Counter> stealSuccessCounter;
    };

    GlobalQueueMetrics& GetGlobalQueueMetrics()
    {
        static GlobalQueueMetrics metrics{
            MetricsSystem::Instance().RegisterCounter(
                "globalqueue_push_total",
                "Total pushes into GlobalQueue.",
                { "queue" }),
            MetricsSystem::Instance().RegisterGauge(
                "globalqueue_depth",
                "Current queue depth per GlobalQueue shard.",
                { "queue", "shard" }),
            MetricsSystem::Instance().RegisterCounter(
                "globalqueue_pop_total",
                "Total GlobalQueue pop outcomes.",
                { "reason" }),
            MetricsSystem::Instance().RegisterCounter(
                "globalqueue_steal_attempt_total",
                "Total GlobalQueue steal attempts."),
            MetricsSystem::Instance().RegisterCounter(
                "globalqueue_steal_success_total",
                "Total GlobalQueue steal successes."),
        };

        return metrics;
    }

    void RecordGlobalQueuePush()
    {
        GetGlobalQueueMetrics().pushCounter->Inc(1.0, { { "queue", "global" } });
    }

    void RecordGlobalQueuePop(bool hit)
    {
        GetGlobalQueueMetrics().popCounter->Inc(1.0, { { "reason", hit ? "hit" : "miss" } });
    }

    void RecordGlobalQueueStealAttempt()
    {
        GetGlobalQueueMetrics().stealAttemptCounter->Inc();
    }

    void RecordGlobalQueueStealSuccess()
    {
        GetGlobalQueueMetrics().stealSuccessCounter->Inc();
    }

    void ObserveGlobalQueueShardDepth(int32 shardIndex, int64 depth)
    {
        GetGlobalQueueMetrics().depthGauge->Set(
            static_cast<double>(depth),
            { { "queue", "global" }, { "shard", std::to_string(shardIndex) } });
    }
}

class LockQueue
{
public:
    explicit LockQueue(int32 shardIndex)
        : _shardIndex(shardIndex)
    {
    }

    void Push(const shared_ptr<JobQueue>& jobQueue)
    {
        WRITE_LOCK;
        _items.push(jobQueue);
        ObserveGlobalQueueShardDepth(_shardIndex, static_cast<int64>(_items.size()));
    }

    shared_ptr<JobQueue> Pop()
    {
        WRITE_LOCK;
        if (_items.empty())
            return nullptr;

        shared_ptr<JobQueue> ret = _items.front();
        _items.pop();
        ObserveGlobalQueueShardDepth(_shardIndex, static_cast<int64>(_items.size()));
        return ret;
    }

    bool Empty()
    {
        READ_LOCK;
        return _items.empty();
    }

private:
    int32 _shardIndex = 0;
    USE_LOCK;
    queue<shared_ptr<JobQueue>> _items;
};

GlobalQueue::GlobalQueue()
{
    for (int32 i = 0; i < 32; i++)
    {
        _jobQueues.push_back(new LockQueue(i));
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
    if (jobQueue == nullptr)
        return;

    const size_t shardCount = _jobQueues.size();
    const int32 index = static_cast<int32>((reinterpret_cast<uintptr_t>(jobQueue.get()) >> 4) % shardCount);

    RecordGlobalQueuePush();
    _jobQueues[index]->Push(jobQueue);
}

shared_ptr<JobQueue> GlobalQueue::Pop()
{
    const int32 index = static_cast<int32>(LThreadId % _jobQueues.size());

    shared_ptr<JobQueue> jobQueue = _jobQueues[index]->Pop();
    if (jobQueue)
    {
        RecordGlobalQueuePop(true);
        return jobQueue;
    }

    for (int32 i = 0; i < static_cast<int32>(_jobQueues.size()); i++)
    {
        if (i == index)
            continue;

        if (_jobQueues[i]->Empty())
            continue;

        RecordGlobalQueueStealAttempt();
        jobQueue = _jobQueues[i]->Pop();
        if (jobQueue)
        {
            RecordGlobalQueueStealSuccess();
            RecordGlobalQueuePop(true);
            return jobQueue;
        }
    }

    RecordGlobalQueuePop(false);
    return nullptr;
}
