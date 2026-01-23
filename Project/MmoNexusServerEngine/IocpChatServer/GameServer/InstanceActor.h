#pragma once
#include <memory>
#include "JobQueue.h"
#include "Job.h"
#include "InstanceManagerCore.h"
#include "GameRoom.h" 
#include "RoomManager.h"

// 인스턴스 던전 관리를 위한 액터 클래스
// 싱글톤으로 만들어서 어디서든 접근 가능하게 했고, JobQueue를 통해 비동기로 처리함
// 멀티스레드 환경에서 인스턴스 생성/삭제 요청이 꼬이지 않게 순차 처리하는 게 핵심
class InstanceActor
{
public:
    static InstanceActor& Instance()
    {
        static InstanceActor inst;
        return inst;
    }

    // 외부에서 람다식으로 일감 던져주는 함수
    // 락을 거는 대신 Job을 큐에 밀어넣는 방식이라 성능 저하가 적음
    template<typename Fn>
    void Push(Fn&& fn)
    {
        _queue->Push(ObjectPool<Job>::MakeShared(std::forward<Fn>(fn)));
    }

    InstanceManagerCore& Core() { return _core; }

    // 주기적으로 호출돼서 만료된 던전을 정리하는 함수
    // 메인 루프나 별도 타이머 스레드에서 호출해줘야 함
    void TickTimeout()
    {
        const uint64 now = ::GetTickCount64();
        Vector<InstanceManagerCore::InstanceInfo> expired;

        // 시간 다 된 인스턴스들 수집
        _core.CollectExpired(now, expired);

        for (auto& inst : expired)
        {
            InstanceManagerCore::InstanceInfo closed;
            // 관리 코어에서 논리적으로 삭제 성공하면
            if (_core.CloseForParty(inst.partyId, closed))
            {
                // 실제 메모리에 떠있는 GameRoom도 닫힘 상태로 변경
                // 이렇게 마킹해두면 나중에 GameServer 메인 루프에서 안전하게 Purge 함
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
    InstanceManagerCore _core; // 실제 데이터 관리는 여기서 함
};