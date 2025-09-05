#pragma once
#include <vector>
#include <deque>
#include <thread>
#include <future>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <atomic>

#include "Metrics.h"// 테스트용

class JobQueue {
public:
    using Job = std::function<void()>;

    JobQueue(size_t maxQueueSize = 0) // maxQueueSize==0 : unbounded
        : stop_(false), maxQueueSize_(maxQueueSize) {
    }

    ~JobQueue() {
        Stop();
    }

    void Start(size_t threadCount = std::thread::hardware_concurrency()) {
        stop_ = false;
        for (size_t i = 0; i < threadCount; ++i) {
            workers_.emplace_back([this]() {
                WorkerLoop();
                });
        }
    }

    void Stop() {
        {
            std::unique_lock<std::mutex> lk(mtx_);
            stop_ = true;
            cv_.notify_all();
        }
        for (auto& t : workers_) if (t.joinable()) t.join();
        workers_.clear();
    }

    // Enqueue returns a future for the task result.
    template<typename F, typename... Args>
    auto Enqueue(F&& f, Args&&... args)
        -> std::future<typename std::result_of<F(Args...)>::type>
    {
        using Ret = typename std::result_of<F(Args...)>::type;
        auto taskPtr = std::make_shared<std::packaged_task<Ret()>>(std::bind(std::forward<F>(f), std::forward<Args>(args)...));
        std::future<Ret> res = taskPtr->get_future();

        {
            std::unique_lock<std::mutex> lk(mtx_);
            if (maxQueueSize_ > 0) {
                // blocking or fail policy: here we block until space (simple)
                cvFull_.wait(lk, [this]() { return stop_ || tasks_.size() < maxQueueSize_; });
                if (stop_) throw std::runtime_error("JobQueue stopped");
            }
            tasks_.emplace_back([taskPtr]() { (*taskPtr)(); });
        }
        cv_.notify_one();
        return res;
    }

    // Non-blocking push (returns false if full)
    bool TryEnqueue(Job job) {
        std::unique_lock<std::mutex> lk(mtx_);
        if (stop_) return false;
        if (maxQueueSize_ > 0 && tasks_.size() >= maxQueueSize_) return false;
        tasks_.push_back(std::move(job));
        cv_.notify_one();

        //테스트용
        GMetrics.jobs_enqueued.fetch_add(1, std::memory_order_relaxed);
        uint32_t sz = static_cast<uint32_t>(tasks_.size());
        uint32_t prev = GMetrics.jobqueue_peak.load();
        while (sz > prev && !GMetrics.jobqueue_peak.compare_exchange_weak(prev, sz)) {}


        return true;
    }

    // <-- 수정: Size()를 const로 변경하고 내부 뮤텍스는 mutable로 선언 -->
    size_t Size() const {
        std::unique_lock<std::mutex> lk(mtx_); // mtx_ is mutable so we can lock in const method
        return tasks_.size();
    }

private:
    void WorkerLoop() {
        while (true) {
			//테스트용
            GMetrics.jobs_executed.fetch_add(1, std::memory_order_relaxed);

            Job job;
            {
                std::unique_lock<std::mutex> lk(mtx_);
                cv_.wait(lk, [this]() { return stop_ || !tasks_.empty(); });
                if (stop_ && tasks_.empty()) return;
                job = std::move(tasks_.front()); tasks_.pop_front();
                if (maxQueueSize_ > 0) cvFull_.notify_one();
            }
            try {
                job();
            }
            catch (const std::exception& e) {
                std::cerr << "[JobQueue Error] " << e.what() << std::endl;
            }
            catch (...) {
                // TODO: async log
            }
        }
    }

    std::vector<std::thread> workers_;
    std::deque<Job> tasks_;
    mutable std::mutex mtx_;               // <-- 수정: mutable로 변경
    std::condition_variable cv_;
    std::condition_variable cvFull_;
    std::atomic<bool> stop_;
    size_t maxQueueSize_;
};
