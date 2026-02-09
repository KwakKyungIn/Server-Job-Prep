#include "pch.h"
#include "ThreadManager.h"
#include "CoreTLS.h"
#include "CoreGlobal.h"
#include "GlobalQueue.h"
#include "JobQueue.h"
#include "MetricsSystem.h"

#include <chrono>

namespace
{
    struct WorkerMetrics
    {
        std::shared_ptr<Counter> idleSecondsCounter;
        std::shared_ptr<Counter> execSecondsCounter;
        std::shared_ptr<Gauge> activeGauge;
    };

    WorkerMetrics& GetWorkerMetrics()
    {
        static WorkerMetrics metrics{
            MetricsSystem::Instance().RegisterCounter(
                "worker_idle_seconds_total",
                "Accumulated idle time for global queue workers in seconds.",
                { "type" }),
            MetricsSystem::Instance().RegisterCounter(
                "worker_exec_seconds_total",
                "Accumulated execution time for global queue workers in seconds.",
                { "type" }),
            MetricsSystem::Instance().RegisterGauge(
                "worker_active",
                "Number of currently active global queue workers.",
                { "type" }),
        };

        return metrics;
    }

    void AddIdleSeconds(double idleSeconds)
    {
        if (idleSeconds <= 0.0)
            return;

        GetWorkerMetrics().idleSecondsCounter->Inc(idleSeconds, { { "type", "logic" } });
    }

    void AddExecSeconds(double execSeconds)
    {
        if (execSeconds <= 0.0)
            return;

        GetWorkerMetrics().execSecondsCounter->Inc(execSeconds, { { "type", "logic" } });
    }

    class ActiveWorkerGuard
    {
    public:
        ActiveWorkerGuard()
        {
            GetWorkerMetrics().activeGauge->Add(1.0, { { "type", "logic" } });
        }

        ~ActiveWorkerGuard()
        {
            GetWorkerMetrics().activeGauge->Add(-1.0, { { "type", "logic" } });
        }
    };
}

ThreadManager::ThreadManager()
{
    InitTLS();
}

ThreadManager::~ThreadManager()
{
    Join();
}

void ThreadManager::Launch(function<void()> callback)
{
    LockGuard guard(_lock);
    _threads.push_back(thread([=]()
    {
        InitTLS();
        callback();
        DestroyTLS();
    }));
}

void ThreadManager::Join()
{
    for (thread& t : _threads)
    {
        if (t.joinable())
            t.join();
    }

    _threads.clear();
}

void ThreadManager::InitTLS()
{
    static Atomic<uint32> SThreadId = 1;
    LThreadId = SThreadId.fetch_add(1);
}

void ThreadManager::DestroyTLS()
{
}

void ThreadManager::DoGlobalQueueWork()
{
    while (GIsRunning.load())
    {
        if (GGlobalQueue == nullptr)
        {
            const auto idleStart = std::chrono::steady_clock::now();
            this_thread::sleep_for(std::chrono::milliseconds(10));
            const auto idleEnd = std::chrono::steady_clock::now();
            AddIdleSeconds(std::chrono::duration<double>(idleEnd - idleStart).count());
            continue;
        }

        shared_ptr<JobQueue> jobQueue = GGlobalQueue->Pop();

        if (jobQueue == nullptr)
        {
            const auto idleStart = std::chrono::steady_clock::now();
            this_thread::sleep_for(std::chrono::milliseconds(10));
            const auto idleEnd = std::chrono::steady_clock::now();
            AddIdleSeconds(std::chrono::duration<double>(idleEnd - idleStart).count());
            continue;
        }

        ActiveWorkerGuard activeGuard;
        const auto execStart = std::chrono::steady_clock::now();
        jobQueue->Execute();
        const auto execEnd = std::chrono::steady_clock::now();
        AddExecSeconds(std::chrono::duration<double>(execEnd - execStart).count());
    }
}
