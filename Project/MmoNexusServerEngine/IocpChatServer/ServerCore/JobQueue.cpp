#include "pch.h"
#include "JobQueue.h"
#include "GlobalQueue.h"
#include "Metrics.h"
#include "MetricsSystem.h"

#include <chrono>

namespace
{
    uint64 NowSteadyMicroseconds()
    {
        return static_cast<uint64>(std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count());
    }

    struct JobQueueMetrics
    {
        std::shared_ptr<Gauge> depthGauge;
        std::shared_ptr<Histogram> batchSizeHistogram;
        std::shared_ptr<Histogram> waitHistogram;
        std::shared_ptr<Histogram> execHistogram;
    };

    JobQueueMetrics& GetJobQueueMetrics()
    {
        static const std::vector<double> kBatchSizeBuckets = {
            1.0,
            2.0,
            4.0,
            8.0,
            16.0,
            32.0,
            64.0,
            128.0,
        };

        static JobQueueMetrics metrics{
            MetricsSystem::Instance().RegisterGauge(
                "jobqueue_depth",
                "Current depth of JobQueue.",
                { "queue" }),
            MetricsSystem::Instance().RegisterHistogram(
                "jobqueue_batch_size",
                "Batch size executed by JobQueue::Execute.",
                kBatchSizeBuckets,
                { "queue" }),
            MetricsSystem::Instance().RegisterHistogram(
                "jobqueue_wait_seconds",
                "Wait time from enqueue to execution start in JobQueue.",
                MetricsHistogramBuckets::JobQueueWaitSeconds(),
                { "queue" }),
            MetricsSystem::Instance().RegisterHistogram(
                "jobqueue_exec_seconds",
                "Execution time per Job in JobQueue.",
                MetricsHistogramBuckets::JobQueueWaitSeconds(),
                { "queue" }),
        };

        return metrics;
    }

    void ObserveJobQueueDepth(int64 depth)
    {
        GetJobQueueMetrics().depthGauge->Set(static_cast<double>(depth), { { "queue", "job" } });
    }

    void ObserveJobQueueBatchSize(size_t batchSize)
    {
        GetJobQueueMetrics().batchSizeHistogram->Observe(static_cast<double>(batchSize), { { "queue", "job" } });
    }

    void ObserveJobQueueWaitSeconds(double waitSeconds)
    {
        GetJobQueueMetrics().waitHistogram->Observe(waitSeconds, { { "queue", "job" } });
    }

    void ObserveJobQueueExecSeconds(double execSeconds)
    {
        GetJobQueueMetrics().execHistogram->Observe(execSeconds, { { "queue", "job" } });
    }
}

void JobQueue::Push(shared_ptr<Job> job)
{
    if (job == nullptr)
        return;

    job->SetEnqueueTimestampUs(NowSteadyMicroseconds());

    const bool prev = _posted.exchange(true);
    int64 depthAfterPush = 0;

    {
        WRITE_LOCK;
        _jobs.push(job);
        depthAfterPush = ++_depth;
    }

    ObserveJobQueueDepth(depthAfterPush);

    if (prev == false)
    {
        GGlobalQueue->Push(shared_from_this());
    }
}

void JobQueue::Execute()
{
    while (true)
    {
        vector<shared_ptr<Job>> jobs;

        {
            WRITE_LOCK;
            while (_jobs.empty() == false)
            {
                jobs.push_back(_jobs.front());
                _jobs.pop();
                --_depth;
            }
        }

        ObserveJobQueueDepth(_depth.load());

        if (jobs.empty() == false)
            ObserveJobQueueBatchSize(jobs.size());

        if (jobs.empty())
        {
            _posted.store(false);

            {
                WRITE_LOCK;
                if (_jobs.empty())
                    break;

                const bool prev = _posted.exchange(true);
                if (prev == false)
                    continue;
                else
                    break;
            }
        }

        for (shared_ptr<Job>& job : jobs)
        {
            if (job == nullptr)
                continue;

            const uint64 execStartUs = NowSteadyMicroseconds();
            const uint64 enqueueUs = job->GetEnqueueTimestampUs();
            if (enqueueUs != 0 && execStartUs >= enqueueUs)
            {
                const double waitSeconds = static_cast<double>(execStartUs - enqueueUs) / 1000000.0;
                ObserveJobQueueWaitSeconds(waitSeconds);
            }

            job->Execute();

            const uint64 execEndUs = NowSteadyMicroseconds();
            if (execEndUs >= execStartUs)
            {
                const double execSeconds = static_cast<double>(execEndUs - execStartUs) / 1000000.0;
                ObserveJobQueueExecSeconds(execSeconds);
            }
        }
    }
}
