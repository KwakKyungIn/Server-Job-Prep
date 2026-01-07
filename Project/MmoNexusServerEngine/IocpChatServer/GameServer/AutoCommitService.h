#pragma once
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <unordered_set>
#include <functional>
#include "Protocol_S2S.pb.h"

class RedisManager;

namespace Persistence
{
    class AutoCommitService
    {
    public:
        using SendCoreFn = std::function<void(const Protocol::S2S_REQ_SAVE_PLAYER_CORE&)>;
        using SendInvFn = std::function<void(const Protocol::S2S_REQ_SAVE_INVENTORY&)>;

        static AutoCommitService& I();

        // B가 MakeSendBuffer/PacketId 붙인 다음에,
        // 여기 send 콜백만 연결하면 AutoCommitService는 완성된다.
        void Init(RedisManager* redis, SendCoreFn sendCore, SendInvFn sendInv);

        void Start();
        void Stop();

        // 로그아웃/Disconnect 즉시 저장 트리거
        void RequestFlushNow(uint64 pid);

        // 응답(RES_SAVE_*) 성공/실패 처리 끝났을 때 inflight 해제용
        void OnCommitFinished(uint64 pid);

        // (테스트/디버그) 스레드 없이 1회 커밋 시도
        void TickOnce();

    private:
        AutoCommitService() = default;

        void WorkerLoop();
        void TickCommit_Internal();

        bool ParsePid(const std::string& s, uint64& outPid);

    private:
        RedisManager* _redis = nullptr;
        SendCoreFn _sendCore;
        SendInvFn  _sendInv;

        std::thread _worker;
        std::atomic<bool> _running{ false };

        std::mutex _mx;
        std::condition_variable _cv;

        std::unordered_set<uint64> _flushNow;
        std::unordered_set<uint64> _inflight;
    };
}
