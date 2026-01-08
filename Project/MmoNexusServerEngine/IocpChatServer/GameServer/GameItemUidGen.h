#pragma once
#include <atomic>
#include <cstdint>

class GameItemUidGen
{
public:
    // 서버 시작 시 1회 호출: DB에서 읽은 nextUid로 시드
    static void Init(uint64_t nextUid)
    {
        _next.store(nextUid, std::memory_order_release);
    }

    // 런타임 발급
    static uint64_t Alloc()
    {
        return _next.fetch_add(1, std::memory_order_relaxed);
    }

    // 디버그/로그용
    static uint64_t Peek()
    {
        return _next.load(std::memory_order_acquire);
    }

private:
    static std::atomic<uint64_t> _next;
};
