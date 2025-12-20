#pragma once
#include <memory>
#include "JobQueue.h"
#include "PartyManagerCore.h"

class PartyActor
{
public:
    static PartyActor& Instance()
    {
        static PartyActor inst;
        return inst;
    }

    // PartyActor 전용 큐에 작업 등록
    template<typename Fn>
    void Push(Fn&& fn)
    {
        _queue->Push(ObjectPool<Job>::MakeShared(std::forward<Fn>(fn)));
    }

    // PartyActor thread에서만 Core 접근하도록 “핸들” 제공
    PartyManagerCore& Core() { return _core; }
    const PartyManagerCore& Core() const { return _core; }

private:
    PartyActor()
    {
        _queue = std::make_shared<JobQueue>();
    }

private:
    std::shared_ptr<JobQueue> _queue;
    PartyManagerCore _core; // 락 없음. PartyActor thread only.
};
