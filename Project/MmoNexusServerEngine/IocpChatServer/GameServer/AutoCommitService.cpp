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

    void AutoCommitService::Init(RedisManager* redis, SendCoreFn sendCore, SendInvFn sendInv, SendQsFn sendQs)
    {
        _redis = redis;
        _sendCore = std::move(sendCore);
        _sendInv = std::move(sendInv);
        _sendQs = std::move(sendQs);
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
        auto it = _inflightCount.find(pid);
        if (it == _inflightCount.end())
            return;

        it->second -= 1;
        if (it->second <= 0)
            _inflightCount.erase(it);
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

        HashSet<uint64> targets;

        // dirty:player
        { std::vector<std::string> pids; if (_redis->SMembers(KeyDirtyPlayer(), pids)) for (auto& s : pids) { uint64 pid = 0; if (ParsePid(s, pid)) targets.insert(pid); } }

        // dirty:inv
        { std::vector<std::string> pids; if (_redis->SMembers(KeyDirtyInv(), pids)) for (auto& s : pids) { uint64 pid = 0; if (ParsePid(s, pid)) targets.insert(pid); } }

        // dirty:qs  [NEW]
        { std::vector<std::string> pids; if (_redis->SMembers(KeyDirtyQuick(), pids)) for (auto& s : pids) { uint64 pid = 0; if (ParsePid(s, pid)) targets.insert(pid); } }

        // flushNow
        {
            std::lock_guard<std::mutex> lock(_mx);
            for (uint64 pid : _flushNow) targets.insert(pid);
            _flushNow.clear();
        }

        for (uint64 pid : targets)
        {
            // inflight 가드
            {
                std::lock_guard<std::mutex> lock(_mx);
                if (_inflightCount.count(pid))
                    continue;
            }

            Protocol::S2S_REQ_SAVE_PLAYER_CORE coreReq;
            Protocol::S2S_REQ_SAVE_INVENTORY invReq;
            Protocol::S2S_REQ_SAVE_QUICKSLOT qsReq;

            const bool coreOk = PersistenceService::I().BuildSnapshot_PlayerCore(pid, coreReq);
            const bool invOk = PersistenceService::I().BuildSnapshot_Inventory(pid, invReq);
            const bool qsOk = PersistenceService::I().BuildSnapshot_QuickSlot(pid, qsReq);

            int32 sentCount = 0;

            if (coreOk && _sendCore) { _sendCore(coreReq); sentCount++; }
            if (invOk && _sendInv) { _sendInv(invReq);  sentCount++; }
            if (qsOk && _sendQs) { _sendQs(qsReq);    sentCount++; }

            if (sentCount <= 0)
                continue;

            {
                std::lock_guard<std::mutex> lock(_mx);
                _inflightCount[pid] = sentCount;
            }
        }
    }
}
