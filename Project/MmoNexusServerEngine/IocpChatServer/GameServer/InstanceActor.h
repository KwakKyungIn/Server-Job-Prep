#pragma once
#include <memory>
#include "JobQueue.h"
#include "Job.h"
#include "InstanceManagerCore.h"

class InstanceActor
{
public:
    static InstanceActor& Instance()
    {
        static InstanceActor inst;
        return inst;
    }

    template<typename Fn>
    void Push(Fn&& fn)
    {
        _queue->Push(ObjectPool<Job>::MakeShared(std::forward<Fn>(fn)));
    }

    InstanceManagerCore& Core() { return _core; }

private:
    InstanceActor()
    {
        _queue = std::make_shared<JobQueue>();
    }

private:
    std::shared_ptr<JobQueue> _queue;
    InstanceManagerCore _core;
};
