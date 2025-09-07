#include "pch.h"
#include "MemoryPool.h"
#include "Metrics.h" // [METRICS] 추가

/*-----------------
    MemoryPool
------------------*/

MemoryPool::MemoryPool(int32 allocSize) : _allocSize(allocSize)
{
    ::InitializeSListHead(&_header);
}

MemoryPool::~MemoryPool()
{
    while (MemoryHeader* memory = static_cast<MemoryHeader*>(::InterlockedPopEntrySList(&_header)))
        ::_aligned_free(memory);
}

void MemoryPool::Push(MemoryHeader* ptr)
{
    ptr->allocSize = 0;

    ::InterlockedPushEntrySList(&_header, static_cast<PSLIST_ENTRY>(ptr));

    // [METRICS] 풀 반납: inuse--, free_total++
    GMetrics.mpool_inuse_gauge.fetch_sub(1, std::memory_order_relaxed);
    GMetrics.mpool_free_total.fetch_add(1, std::memory_order_relaxed);

    _useCount.fetch_sub(1);
    _reserveCount.fetch_add(1);
}

MemoryHeader* MemoryPool::Pop()
{
    MemoryHeader* memory = static_cast<MemoryHeader*>(::InterlockedPopEntrySList(&_header));

    // 없으면 새로 만들다
    if (memory == nullptr)
    {
        memory = reinterpret_cast<MemoryHeader*>(::_aligned_malloc(_allocSize, SLIST_ALIGNMENT));
        // [METRICS] 풀 신규 할당: alloc_total++
        GMetrics.mpool_alloc_total.fetch_add(1, std::memory_order_relaxed);
    }
    else
    {
        ASSERT_CRASH(memory->allocSize == 0);
        _reserveCount.fetch_sub(1);
    }

    _useCount.fetch_add(1);

    // [METRICS] 풀에서 꺼냄: inuse++ 및 1초 피크 갱신
    uint32_t inuse = GMetrics.mpool_inuse_gauge.fetch_add(1, std::memory_order_relaxed) + 1;
    uint32_t prev = GMetrics.mpool_inuse_peak.load(std::memory_order_relaxed);
    while (inuse > prev && !GMetrics.mpool_inuse_peak.compare_exchange_weak(prev, inuse, std::memory_order_relaxed)) {}

    return memory;
}
