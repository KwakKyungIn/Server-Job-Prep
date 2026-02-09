#include "pch.h"
#include "ThreadManager.h"
#include "CoreTLS.h"
#include "CoreGlobal.h"
#include "GlobalQueue.h" // [NEW] 필수
#include "JobQueue.h"


/*--------------------------------
        ThreadManager
    쓰레드 생성/관리 클래스
    - TLS 초기화
    - 쓰레드 시작/종료 관리
---------------------------------*/

ThreadManager::ThreadManager()
{
    // 메인 쓰레드도 TLS 초기화
    InitTLS();
}

ThreadManager::~ThreadManager()
{
    // 모든 쓰레드가 끝날 때까지 대기
    Join();
}

void ThreadManager::Launch(function<void()> callback)
{
    // 새로운 쓰레드 생성 시 락 걸고 리스트에 추가
    LockGuard guard(_lock);
    _threads.push_back(thread([=]()
        {
            // 새 쓰레드에서 TLS 초기화
            InitTLS();

            // 실행할 콜백 함수
            callback();

            // TLS 정리 (현재는 비어 있음)
            DestroyTLS();
        }));
}

void ThreadManager::Join()
{
    // 생성된 모든 쓰레드를 join
    for (thread& t : _threads)
    {
        if (t.joinable())
        {
            t.join(); // 쓰레드 종료 대기
        }
    }
    _threads.clear(); // 벡터 초기화
}

void ThreadManager::InitTLS()
{
    // 고유한 스레드 ID를 부여
    static Atomic<uint32> SThreadId = 1;
    LThreadId = SThreadId.fetch_add(1);
}

void ThreadManager::DestroyTLS()
{
    // 현재는 정리할 리소스 없음 (확장 가능)
}

// [NEW] 워커 스레드가 실행할 메인 루프
void ThreadManager::DoGlobalQueueWork()
{
    while (GIsRunning.load()) // 혹은 while (GIsRunning)
    {
        // 1. GlobalQueue에서 일감이 있는 JobQueue를 꺼내온다
        // GGlobalQueue가 nullptr면 크래시 나니까 방어 코드 (초반 초기화 이슈 방지)
        if (GGlobalQueue == nullptr)
        {
            this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }

        shared_ptr<JobQueue> jobQueue = GGlobalQueue->Pop();

        // 2. 일감이 없으면? 
        if (jobQueue == nullptr)
        {
            // 잠깐 쉬었다가 다시 확인 (CPU 과점유 방지)
            this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }

        // 3. 일감이 있으면 실행한다.
        // Execute 내부에서 큐에 쌓인 Job들을 처리함
        jobQueue->Execute();
    }
}