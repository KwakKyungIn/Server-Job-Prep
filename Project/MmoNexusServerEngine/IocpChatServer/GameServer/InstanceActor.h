#pragma once
#include <memory>
#include "JobQueue.h"
#include "Job.h"
#include "InstanceManagerCore.h"
#include "GameRoom.h" 
#include "RoomManager.h"
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

    void TickTimeout()
    {
        const uint64 now = ::GetTickCount64();
        Vector<InstanceManagerCore::InstanceInfo> expired;
        _core.CollectExpired(now, expired);

        for (auto& inst : expired)
        {
            InstanceManagerCore::InstanceInfo closed;
            if (_core.CloseForParty(inst.partyId, closed))
            {
                // room이 존재하면 closing 마킹 (purge 가능)
                if (GRoomManager)
                {
                    auto room = GRoomManager->FindRoom(closed.channelId, closed.mapId, closed.instanceId);
                    if (room) room->MarkClosing(true);
                }
            }
        }
    }




private:
    InstanceActor()
    {
        _queue = MakeShared<JobQueue>();
    }

private:
    std::shared_ptr<JobQueue> _queue;
    InstanceManagerCore _core;
};
