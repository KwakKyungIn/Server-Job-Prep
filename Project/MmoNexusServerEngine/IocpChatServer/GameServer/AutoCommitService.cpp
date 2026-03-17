#include "pch.h"
#include "AutoCommitService.h"

#include "ExperimentUtils.h"
#include "GameMetrics.h"
#include "RedisManager.h"
#include "RedisKeys.h"
#include "PersistenceService.h"

#include <algorithm>
#include <chrono>
#include <thread>
#include <vector>

namespace Persistence
{
    namespace
    {
        // Flush 버스트를 줄이기 위해 한 번에 처리할 타겟 수를 제한한다.
        constexpr size_t kAutoCommitTargetsPerBatch = 12;
        constexpr int32 kAutoCommitInterBatchSleepMs = 8;
        // SendBuffer chunk(6KB) 하드 제한보다 여유 있게 인벤토리 저장 패킷을 분할한다.
        constexpr int32 kInventoryChunkMaxBytes = 3500;

        int32 SendInventoryChunked(
            const AutoCommitService::SendInvFn& sendInv,
            const Protocol::S2S_REQ_SAVE_INVENTORY& srcReq)
        {
            if (sendInv == nullptr)
                return 0;

            Protocol::S2S_REQ_SAVE_INVENTORY chunkReq;
            chunkReq.set_playerid(srcReq.playerid());

            int32 sentCount = 0;
            bool hasPayload = false;

            // 삭제 목록은 첫 청크에만 넣는다.
            for (int i = 0; i < srcReq.deleteditemuids_size(); ++i)
            {
                chunkReq.add_deleteditemuids(srcReq.deleteditemuids(i));
                hasPayload = true;
            }

            auto FlushChunk = [&]() -> bool
            {
                if (hasPayload == false)
                    return true;

                sendInv(chunkReq);
                ++sentCount;

                chunkReq.clear_deleteditemuids();
                chunkReq.clear_items();
                hasPayload = false;
                return true;
            };

            for (int i = 0; i < srcReq.items_size(); ++i)
            {
                const Protocol::ItemInfo& item = srcReq.items(i);
                *chunkReq.add_items() = item;
                hasPayload = true;

                if (chunkReq.ByteSizeLong() > kInventoryChunkMaxBytes)
                {
                    chunkReq.mutable_items()->RemoveLast();

                    if (FlushChunk() == false)
                        return sentCount;

                    *chunkReq.add_items() = item;
                    hasPayload = true;

                    // 단일 아이템이 너무 커서 기준을 넘어도 그대로 전송(무한 루프 방지)
                    if (chunkReq.ByteSizeLong() > kInventoryChunkMaxBytes)
                    {
                        sendInv(chunkReq);
                        ++sentCount;
                        chunkReq.clear_deleteditemuids();
                        chunkReq.clear_items();
                        hasPayload = false;
                    }
                }
            }

            FlushChunk();
            return sentCount;
        }
    }

    AutoCommitService& AutoCommitService::I()
    {
        static AutoCommitService s;
        return s;
    }

    // 외부에서 의존성 주입받는 초기화 함수
    // 패킷 보내는 함수들을 람다나 함수 포인터로 받아서 결합도를 낮춤
    void AutoCommitService::Init(RedisManager* redis, SendCoreFn sendCore, SendInvFn sendInv)
    {
        Init(redis, std::move(sendCore), std::move(sendInv), SendQsFn());
    }

    void AutoCommitService::Init(RedisManager* redis, SendCoreFn sendCore, SendInvFn sendInv, SendQsFn sendQs)
    {
        _redis = redis;
        _sendCore = std::move(sendCore);
        _sendInv = std::move(sendInv);
        _sendQs = std::move(sendQs);
        GameMetrics::OnAutoCommitInflight(0);
        GameMetrics::OnAutoCommitTargets(0);
    }

    void AutoCommitService::Start()
    {
        // 이미 실행 중이면 중복 실행 방지
        if (_running.exchange(true))
            return;

        // 백그라운드 워커 스레드 생성
        _worker = std::thread([this]()
            {
                WorkerLoop();
            });
    }

    void AutoCommitService::Stop()
    {
        // 실행 중이 아니면 리턴
        if (!_running.exchange(false))
            return;

        // 자고 있는 스레드 깨워서 종료시킴
        _cv.notify_all();
        if (_worker.joinable())
            _worker.join();
    }

    // 로그아웃 시 즉시 저장 요청
    // 뮤텍스로 보호된 셋에 PID 넣고 스레드 깨움
    void AutoCommitService::RequestFlushNow(uint64 pid)
    {
        {
            std::lock_guard<std::mutex> lock(_mx);
            _flushNow.insert(pid);
        }
        _cv.notify_all();
    }

    bool AutoCommitService::SendQuickSlotImmediate(uint64 pid)
    {
        if (pid == 0 || _sendQs == nullptr)
            return false;

        Protocol::S2S_REQ_SAVE_QUICKSLOT qsReq;
        if (PersistenceService::I().BuildSnapshot_QuickSlot(pid, qsReq) == false)
            return false;

        _sendQs(qsReq);
        return true;
    }

    // 저장 패킷에 대한 응답이 왔을 때 호출됨
    // 해당 PID에 대한 대기 카운트를 줄여서 다음 저장을 가능하게 함
    void AutoCommitService::OnCommitFinished(uint64 pid)
    {
        std::lock_guard<std::mutex> lock(_mx);
        auto it = _inflightCount.find(pid);
        if (it == _inflightCount.end())
            return;

        it->second -= 1;
        // 카운트가 0 이하가 되면 맵에서 제거
        if (it->second <= 0)
            _inflightCount.erase(it);

        GameMetrics::OnAutoCommitInflight(_inflightCount.size());
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

    // 스레드 메인 루프
    // 주기적으로 깨어나거나(현재 설정 2분) 즉시 저장 요청이 있을 때 동작함
    void AutoCommitService::WorkerLoop()
    {
        using namespace std::chrono;

        const auto interval = seconds((std::max)(1, ExperimentUtils::GetAutoCommitIntervalSec()));
        auto nextTick = steady_clock::now() + interval;

        while (_running.load())
        {
            std::unique_lock<std::mutex> lock(_mx);
            // 다음 틱까지 대기하되, 종료 신호나 Flush 요청이 오면 바로 깨어남
            _cv.wait_until(lock, nextTick, [this]() {
                return !_running.load() || !_flushNow.empty();
                });

            if (!_running.load())
                break;

            lock.unlock();

            // 실제 커밋 로직 수행
            TickCommit_Internal();

            // 다음 실행 시간 갱신
            nextTick = steady_clock::now() + interval;
        }
    }


    // 저장 로직의 핵심 부분
    // Redis에 기록된 변경 사항(Dirty Set)을 읽어와서 실제 DB 저장을 요청함
    void AutoCommitService::TickCommit_Internal()
    {
        if (!_redis)
            return;

        const std::uint64_t tickStartUs = static_cast<std::uint64_t>(
            std::chrono::duration_cast<std::chrono::microseconds>(
                std::chrono::steady_clock::now().time_since_epoch()).count());

        HashSet<uint64> targets;
        HashSet<uint64> playerDirtyTargets;
        HashSet<uint64> invDirtyTargets;
        HashSet<uint64> quickDirtyTargets;
        HashSet<uint64> flushTargets;
        std::vector<std::string> pids;

        // Redis Set에서 변경된 플레이어 목록 가져오기 (플레이어 정보)
        pids.clear();
        if (_redis->SMembers(KeyDirtyPlayer(), pids))
        {
            for (auto& s : pids)
            {
                uint64 pid = 0;
                if (ParsePid(s, pid) == false)
                    continue;

                playerDirtyTargets.insert(pid);
                targets.insert(pid);
            }
        }
        GameMetrics::OnDirtySetSize(GameMetrics::DirtySetDomain::Player, pids.size());

        // Redis Set에서 변경된 인벤토리 목록 가져오기
        pids.clear();
        if (_redis->SMembers(KeyDirtyInv(), pids))
        {
            for (auto& s : pids)
            {
                uint64 pid = 0;
                if (ParsePid(s, pid) == false)
                    continue;

                invDirtyTargets.insert(pid);
                targets.insert(pid);
            }
        }
        GameMetrics::OnDirtySetSize(GameMetrics::DirtySetDomain::Inventory, pids.size());

        // Redis Set에서 변경된 퀵슬롯 목록 가져오기
        pids.clear();
        if (_redis->SMembers(KeyDirtyQuick(), pids))
        {
            for (auto& s : pids)
            {
                uint64 pid = 0;
                if (ParsePid(s, pid) == false)
                    continue;

                quickDirtyTargets.insert(pid);
                if (ExperimentUtils::IsPersistenceWriteback())
                    targets.insert(pid);
            }
        }
        GameMetrics::OnDirtySetSize(GameMetrics::DirtySetDomain::QuickSlot, pids.size());

        // 즉시 저장 요청된 목록들도 타겟에 추가
        {
            std::lock_guard<std::mutex> lock(_mx);
            for (uint64 pid : _flushNow)
            {
                flushTargets.insert(pid);
                targets.insert(pid);
            }
            _flushNow.clear();
        }

        GameMetrics::OnAutoCommitTargets(targets.size());
        {
            std::lock_guard<std::mutex> lock(_mx);
            GameMetrics::OnAutoCommitInflight(_inflightCount.size());
        }

        std::vector<uint64> targetList;
        targetList.reserve(targets.size());
        for (uint64 pid : targets)
            targetList.push_back(pid);

        size_t processedInBatch = 0;
        for (size_t i = 0; i < targetList.size(); ++i)
        {
            const uint64 pid = targetList[i];

            // 이미 저장 요청이 진행 중인 플레이어는 건너뜀 (중복 저장 방지)
            {
                std::lock_guard<std::mutex> lock(_mx);
                if (_inflightCount.count(pid))
                {
                    GameMetrics::OnAutoCommitSkip(GameMetrics::AutoCommitSkipReason::Inflight);
                    continue;
                }
            }

            Protocol::S2S_REQ_SAVE_PLAYER_CORE coreReq;
            Protocol::S2S_REQ_SAVE_INVENTORY invReq;
            Protocol::S2S_REQ_SAVE_QUICKSLOT qsReq;
            const bool shouldHandleCore = (playerDirtyTargets.count(pid) > 0) || (flushTargets.count(pid) > 0);
            const bool shouldHandleInv = (invDirtyTargets.count(pid) > 0) || (flushTargets.count(pid) > 0);
            const bool shouldHandleQs = ExperimentUtils::IsPersistenceWriteback()
                && ((quickDirtyTargets.count(pid) > 0) || (flushTargets.count(pid) > 0));

            // 현재 메모리 상태를 기반으로 스냅샷 생성
            const bool coreOk = shouldHandleCore && PersistenceService::I().BuildSnapshot_PlayerCore(pid, coreReq);
            const bool invOk = shouldHandleInv && PersistenceService::I().BuildSnapshot_Inventory(pid, invReq);
            const bool qsOk = shouldHandleQs && PersistenceService::I().BuildSnapshot_QuickSlot(pid, qsReq);

            int32 sentCount = 0;

            // 스냅샷 생성에 성공했으면 DB 서버로 전송
            if (coreOk && _sendCore)
            {
                _sendCore(coreReq);
                sentCount++;
                GameMetrics::OnAutoCommitSent(GameMetrics::AutoCommitDomain::Core);
            }
            else if (shouldHandleCore && !coreOk)
            {
                GameMetrics::OnAutoCommitSkip(GameMetrics::AutoCommitSkipReason::RedisMissing);
            }

            if (invOk && _sendInv)
            {
                const int32 invSentCount = SendInventoryChunked(_sendInv, invReq);
                if (invSentCount > 0)
                {
                    sentCount += invSentCount;
                    GameMetrics::OnAutoCommitSent(GameMetrics::AutoCommitDomain::Inventory, static_cast<std::size_t>(invSentCount));
                }
                else
                {
                    GameMetrics::OnAutoCommitSkip(GameMetrics::AutoCommitSkipReason::EmptySnapshot);
                }
            }
            else if (shouldHandleInv && !invOk)
            {
                GameMetrics::OnAutoCommitSkip(GameMetrics::AutoCommitSkipReason::RedisMissing);
            }

            if (qsOk && _sendQs)
            {
                _sendQs(qsReq);
                sentCount++;
                GameMetrics::OnAutoCommitSent(GameMetrics::AutoCommitDomain::QuickSlot);
            }
            else if (shouldHandleQs && !qsOk)
            {
                GameMetrics::OnAutoCommitSkip(GameMetrics::AutoCommitSkipReason::RedisMissing);
            }

            if (sentCount <= 0)
                continue;

            // 전송한 패킷 수만큼 inflight 카운트 증가시켜둠
            {
                std::lock_guard<std::mutex> lock(_mx);
                _inflightCount[pid] = sentCount;
                GameMetrics::OnAutoCommitInflight(_inflightCount.size());
            }

            ++processedInBatch;
            if (processedInBatch >= kAutoCommitTargetsPerBatch && (i + 1) < targetList.size())
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(kAutoCommitInterBatchSleepMs));
                processedInBatch = 0;
            }
        }

        const std::uint64_t tickEndUs = static_cast<std::uint64_t>(
            std::chrono::duration_cast<std::chrono::microseconds>(
                std::chrono::steady_clock::now().time_since_epoch()).count());
        if (tickEndUs >= tickStartUs)
        {
            const double elapsedSeconds = static_cast<double>(tickEndUs - tickStartUs) / 1000000.0;
            GameMetrics::OnAutoCommitTick(elapsedSeconds);
        }
    }
}
