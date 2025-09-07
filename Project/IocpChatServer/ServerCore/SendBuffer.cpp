#include "pch.h"
#include "SendBuffer.h"
#include "Metrics.h" // [METRICS] 추가

/*----------------
    SendBuffer
-----------------*/

SendBuffer::SendBuffer(SendBufferChunkRef owner, BYTE* buffer, uint32 allocSize)
    : _owner(owner), _buffer(buffer), _allocSize(allocSize)
{
}

SendBuffer::~SendBuffer()
{
}

void SendBuffer::Close(uint32 writeSize)
{
    ASSERT_CRASH(_allocSize >= writeSize);
    _writeSize = writeSize;
    _owner->Close(writeSize);
}

/*--------------------
    SendBufferChunk
--------------------*/

SendBufferChunk::SendBufferChunk()
{
}

SendBufferChunk::~SendBufferChunk()
{
}

void SendBufferChunk::Reset()
{
    _open = false;
    _usedSize = 0;
}

SendBufferRef SendBufferChunk::Open(uint32 allocSize)
{
    ASSERT_CRASH(allocSize <= SEND_BUFFER_CHUNK_SIZE);
    ASSERT_CRASH(_open == false);

    if (allocSize > FreeSize())
        return nullptr;

    _open = true;
    return ObjectPool<SendBuffer>::MakeShared(shared_from_this(), Buffer(), allocSize);
}

void SendBufferChunk::Close(uint32 writeSize)
{
    ASSERT_CRASH(_open == true);
    _open = false;
    _usedSize += writeSize;
}

/*---------------------
    SendBufferManager
----------------------*/

SendBufferRef SendBufferManager::Open(uint32 size)
{
    if (LSendBufferChunk == nullptr)
    {
        LSendBufferChunk = Pop(); // WRITE_LOCK
        LSendBufferChunk->Reset();
    }

    ASSERT_CRASH(LSendBufferChunk->IsOpen() == false);

    // 다 썼으면 버리고 새거로 교체
    if (LSendBufferChunk->FreeSize() < size)
    {
        LSendBufferChunk = Pop(); // WRITE_LOCK
        LSendBufferChunk->Reset();
    }

    return LSendBufferChunk->Open(size);
}

SendBufferChunkRef SendBufferManager::Pop()
{
    {
        WRITE_LOCK;
        if (_sendBufferChunks.empty() == false)
        {
            SendBufferChunkRef sendBufferChunk = _sendBufferChunks.back();
            _sendBufferChunks.pop_back();

            // [METRICS] 글로벌 풀에서 Pop + inuse 증가/피크 갱신
            GMetrics.sendbuf_global_pop.fetch_add(1, std::memory_order_relaxed);
            uint32_t inuse = GMetrics.sendbuf_inuse_gauge.fetch_add(1, std::memory_order_relaxed) + 1;
            uint32_t prev = GMetrics.sendbuf_inuse_peak.load(std::memory_order_relaxed);
            while (inuse > prev && !GMetrics.sendbuf_inuse_peak.compare_exchange_weak(prev, inuse, std::memory_order_relaxed)) {}

            return sendBufferChunk;
        }
    }

    // 풀에 없으면 새로 생성해서 반환 (초기 부팅 단계라 Pop 카운트에도 포함)
    SendBufferChunkRef ref = SendBufferChunkRef(xnew<SendBufferChunk>(), PushGlobal);

    // [METRICS] 생성 경로도 '획득'으로 간주하여 Pop + inuse 증가/피크
    GMetrics.sendbuf_global_pop.fetch_add(1, std::memory_order_relaxed);
    {
        uint32_t inuse = GMetrics.sendbuf_inuse_gauge.fetch_add(1, std::memory_order_relaxed) + 1;
        uint32_t prev = GMetrics.sendbuf_inuse_peak.load(std::memory_order_relaxed);
        while (inuse > prev && !GMetrics.sendbuf_inuse_peak.compare_exchange_weak(prev, inuse, std::memory_order_relaxed)) {}
    }

    return ref;
}

void SendBufferManager::Push(SendBufferChunkRef buffer)
{
    WRITE_LOCK;
    _sendBufferChunks.push_back(buffer);
}

void SendBufferManager::PushGlobal(SendBufferChunk* buffer)
{
   
    // [METRICS] 글로벌 풀로 Push + inuse 감소
    GMetrics.sendbuf_global_push.fetch_add(1, std::memory_order_relaxed);
    GMetrics.sendbuf_inuse_gauge.fetch_sub(1, std::memory_order_relaxed);

    GSendBufferManager->Push(SendBufferChunkRef(buffer, PushGlobal));
}
