#pragma once
#include <atomic>
#include <cstdint>

// 게임 내 아이템의 고유 ID(UID)를 발급하는 클래스
// 멀티스레드 환경에서 동시에 여러 명이 아이템을 먹을 수 있으므로 Thread-Safe 해야 함
class GameItemUidGen
{
public:
    // 서버 시작 시 DB에서 마지막으로 발급된 UID + 1 값을 가져와서 초기화
    // 이걸 안 하면 서버 재부팅 할 때마다 아이템 ID가 겹치는 대참사 발생함
    static void Init(uint64_t nextUid)
    {
        _next.store(nextUid, std::memory_order_release);
    }

    // 런타임에 아이템 생성될 때 호출
    // atomic fetch_add를 써서 락 없이 고속으로 유니크 ID 발급
    static uint64_t Alloc()
    {
        return _next.fetch_add(1, std::memory_order_relaxed);
    }

    // 현재 발급된 번호 확인용 (디버깅이나 모니터링)
    static uint64_t Peek()
    {
        return _next.load(std::memory_order_acquire);
    }

private:
    // 동시성 보장을 위해 atomic 사용
    static std::atomic<uint64_t> _next;
};