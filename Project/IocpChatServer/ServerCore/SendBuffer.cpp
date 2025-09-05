#include "pch.h"
#include "SendBuffer.h"

/*----------------
    SendBuffer
-----------------*/

SendBuffer::SendBuffer(SendBufferChunkRef owner, BYTE* buffer, uint32 allocSize)
    : _owner(std::move(owner)), _buffer(buffer), _allocSize(allocSize), _writeSize(0)
{
    // 방어적 체크 (개발 중 ASSERT, 릴리즈에서도 안전)
    ASSERT_CRASH(_owner != nullptr);
    ASSERT_CRASH(_buffer != nullptr);
    ASSERT_CRASH(_allocSize > 0);
}

SendBuffer::~SendBuffer()
{
}

void SendBuffer::Close(uint32 writeSize)
{
    // writeSize는 반드시 allocSize 이하
    ASSERT_CRASH(_allocSize >= writeSize);
    _writeSize = writeSize;

    // owner가 반드시 살아있어야 함 (SendBufferRef가 잡고 있음)
    ASSERT_CRASH(_owner != nullptr);
    _owner->Close(writeSize);
}

/*--------------------
    SendBufferChunk
--------------------*/

SendBufferChunk::SendBufferChunk() {}
SendBufferChunk::~SendBufferChunk() {}

extern thread_local SendBufferChunkRef LSendBufferChunk;

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
    return std::make_shared<SendBuffer>(shared_from_this(), Buffer(), allocSize);

    //return ObjectPool<SendBuffer>::MakeShared(shared_from_this(), Buffer(), allocSize);
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

    // 여유 공간 부족하면 새 청크
    if (LSendBufferChunk->FreeSize() < size)
    {
        LSendBufferChunk = Pop(); // WRITE_LOCK
        LSendBufferChunk->Reset();
    }

    // 드물게 Open이 nullptr을 반환할 수도 있으니 방어
    SendBufferRef sb = LSendBufferChunk->Open(size);
    if (sb == nullptr)
    {
        // 새 청크 팝 후 한 번 더 시도
        LSendBufferChunk = Pop();
        LSendBufferChunk->Reset();
        sb = LSendBufferChunk->Open(size);
        ASSERT_CRASH(sb != nullptr); // 여기서도 nullptr이면 치명적
    }
    return sb;
}

SendBufferChunkRef SendBufferManager::Pop()
{
    {
        WRITE_LOCK;
        if (_sendBufferChunks.empty() == false)
        {
            SendBufferChunkRef sendBufferChunk = _sendBufferChunks.back();
            _sendBufferChunks.pop_back();
            return sendBufferChunk;
        }
    }
    return SendBufferChunkRef(xnew<SendBufferChunk>(), PushGlobal);
}

void SendBufferManager::Push(SendBufferChunkRef buffer)
{
    WRITE_LOCK;
    _sendBufferChunks.push_back(buffer);
}

void SendBufferManager::PushGlobal(SendBufferChunk* buffer)
{
    cout << "PushGlobal SENDBUFFERCHUNK" << endl;
    GSendBufferManager->Push(SendBufferChunkRef(buffer, PushGlobal));
}
