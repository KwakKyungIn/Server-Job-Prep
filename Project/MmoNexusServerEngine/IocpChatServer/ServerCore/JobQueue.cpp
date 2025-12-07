#include "pch.h"
#include "JobQueue.h"
#include "GlobalQueue.h"

void JobQueue::Push(shared_ptr<Job> job)
{
    const bool prev = _posted.exchange(true);

    {
        WRITE_LOCK; // _lock을 건다
        _jobs.push(job);
    }

    // 이전에 아무도 처리를 안 하고 있었다면, 이제 내가 GlobalQueue에 등록하러 간다.
    if (prev == false)
    {
        GGlobalQueue->Push(shared_from_this());
    }
}

void JobQueue::Execute()
{
    // 큐에 있는 일감을 한 번에 싹 처리할지, 몇 개만 할지 결정.
    // 여기선 일단 다 처리하는 구조로 간다.

    while (true)
    {
        vector<shared_ptr<Job>> jobs;
        {
            WRITE_LOCK;
            while (_jobs.empty() == false)
            {
                jobs.push_back(_jobs.front());
                _jobs.pop();
            }
        }

        // 일감이 없으면 종료 루틴
        if (jobs.empty())
        {
            // 처리 끝났다고 깃발 내림
            _posted.store(false);

            // 그 사이에 누가 또 넣었을 수도 있으니까 다시 확인
            {
                WRITE_LOCK;
                if (_jobs.empty())
                    break; // 진짜 없음. 퇴근.

                // 누가 또 넣었네? 다시 깃발 들고 일하러 감
                const bool prev = _posted.exchange(true);
                if (prev == false)
                    continue; // 내가 다시 처리
                else
                    break; // 이미 다른 애가 GlobalQueue에 넣었을 것임
            }
        }

        // 잡 실행 (Lock 밖에서 실행해야 함! 중요!)
        for (shared_ptr<Job>& job : jobs)
        {
            job->Execute();
        }
    }
}