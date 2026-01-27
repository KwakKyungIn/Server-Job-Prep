#pragma once
#include <memory>
#include "JobQueue.h"
#include "PartyManagerCore.h"

// 파티 시스템의 동시성 제어를 담당하는 액터
// PartyManagerCore는 순수 로직만 있고, 실제 락 없이 안전하게 돌리기 위해
// 모든 요청을 이 Actor의 JobQueue에 태워서 처리함
class PartyActor
{
public:
    // 전역에서 접근 가능한 싱글톤
    static PartyActor& Instance()
    {
        static PartyActor inst;
        return inst;
    }

    // 외부에서 일감 던지는 함수
    template<typename Fn>
    void Push(Fn&& fn)
    {
        _queue->Push(ObjectPool<Job>::MakeShared(std::forward<Fn>(fn)));
    }

    // 로직 코어 접근자
    PartyManagerCore& Core() { return _core; }
    const PartyManagerCore& Core() const { return _core; }

    // 파티 상태 변경 + 인스턴스 던전 처리가 묶여있는 작업들
    // 여러 Actor를 오가야 해서 별도 함수로 뺐음
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