#include "pch.h"
#include "AutoCommitService.h"

#include "RedisManager.h"
#include "RedisKeys.h"
#include "PersistenceService.h"

#include <chrono>
#include <vector>

namespace Persistence
{
    AutoCommitService& AutoCommitService::I()
    {
        static AutoCommitService s;
        return s;
    }

    void AutoCommitService::Init(RedisManager* redis, SendCoreFn sendCore, SendInvFn sendInv)
    {
        _redis = redis;
        _sendCore = std::move(sendCore);
        _sendInv = std::move(sendInv);
    }

    void AutoCommitService::Start()
    {
        if (_running.exchange(true))
            return;

        _worker = std::thread([this]()
            {
                WorkerLoop();
            });
    }

    void AutoCommitService::Stop()
    {
        if (!_running.exchange(false))
            return;

        _cv.notify_all();
        if (_worker.joinable())
            _worker.join();
    }

    void AutoCommitService::RequestFlushNow(uint64 pid)
    {
        {
            std::lock_guard<std::mutex> lock(_mx);
            _flushNow.insert(pid);
        }
        _cv.notify_all();
    }

    void AutoCommitService::OnCommitFinished(uint64 pid)
    {
        std::lock_guard<std::mutex> lock(_mx);
        _inflight.erase(pid);
    }

    bool AutoCommitService::ParsePid(const std::string& s, uint64& outPid)
    {
        try { outPid = static_cast<uint64>(std::stoull(s)); return true; }
        catch (...) { return false; }
    }

    void AutoCommitService::TickOnce()
    {
        TickCommit_Internal();
    }

    void AutoCommitService::WorkerLoop()
    {
        using namespace std::chrono;

        const auto interval = seconds(120);
        auto nextTick = steady_clock::now() + interval;

        while (_running.load())
        {
            std::unique_lock<std::mutex> lock(_mx);
            _cv.wait_until(lock, nextTick, [this]() {
                return !_running.load() || !_flushNow.empty();
                });

            if (!_running.load())
                break;

            lock.unlock();

            TickCommit_Internal();

            // 다음 tick 갱신
            nextTick = steady_clock::now() + interval;
        }
    }

    void AutoCommitService::TickCommit_Internal()
    {
        if (!_redis) return;

        // 1) targets 수집: dirty set + flushNow
        std::unordered_set<uint64> targets;

        // dirty:player
        {
            std::vector<std::string> pids;
            if (_redis->SMembers(KeyDirtyPlayer(), pids))
            {
                for (auto& s : pids)
                {
                    uint64 pid = 0;
                    if (ParsePid(s, pid)) targets.insert(pid);
                }
            }
        }

        // dirty:inv
        {
            std::vector<std::string> pids;
            if (_redis->SMembers(KeyDirtyInv(), pids))
            {
                for (auto& s : pids)
                {
                    uint64 pid = 0;
                    if (ParsePid(s, pid)) targets.insert(pid);
                }
            }
        }

        // flushNow (즉시 저장 요청)
        {
            std::lock_guard<std::mutex> lock(_mx);
            for (uint64 pid : _flushNow)
                targets.insert(pid);
            _flushNow.clear();
        }

        // 2) pid별 snapshot -> send
        for (uint64 pid : targets)
        {
            // in-flight 가드
            {
                std::lock_guard<std::mutex> lock(_mx);
                if (_inflight.count(pid))
                    continue;
                _inflight.insert(pid);
            }

            Protocol::S2S_REQ_SAVE_PLAYER_CORE coreReq;
            Protocol::S2S_REQ_SAVE_INVENTORY invReq;

            const bool coreOk = PersistenceService::I().BuildSnapshot_PlayerCore(pid, coreReq);
            const bool invOk = PersistenceService::I().BuildSnapshot_Inventory(pid, invReq);

            bool sentAny = false;

            if (coreOk && _sendCore)
            {
                _sendCore(coreReq);
                sentAny = true;
            }

            if (invOk && _sendInv)
            {
                _sendInv(invReq);
                sentAny = true;
            }

            // 아무 것도 못 보냈으면 inflight 해제(재시도 가능)
            if (!sentAny)
            {
                std::lock_guard<std::mutex> lock(_mx);
                _inflight.erase(pid);
            }
        }
    }
}
