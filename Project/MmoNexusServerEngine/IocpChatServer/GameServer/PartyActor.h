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

    template<typename Fn>
    void Push(Fn&& fn)
    {
        _queue->Push(ObjectPool<Job>::MakeShared(std::forward<Fn>(fn)));
    }

    PartyManagerCore& Core() { return _core; }
    const PartyManagerCore& Core() const { return _core; }

    //  6번 연결 지점: 여기로 모아라
    void LeaveAndHandleInstance(uint64 playerId);
    void DisbandAndHandleInstance(uint64 leaderId);
    void KickAndHandleInstance(uint64 leaderId, uint64 targetId);

private:
    PartyActor()
    {
        _queue = MakeShared<JobQueue>();
    }

private:
    std::shared_ptr<JobQueue> _queue;
    PartyManagerCore _core;
};
