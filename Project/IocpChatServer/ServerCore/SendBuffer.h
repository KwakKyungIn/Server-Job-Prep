#pragma once

class SendBufferChunk;

/*----------------
    SendBuffer
-----------------*/

class SendBuffer
{
public:
    SendBuffer(SendBufferChunkRef owner, BYTE* buffer, uint32 allocSize);
    ~SendBuffer();

    BYTE* Buffer() const { return _buffer; }
    uint32  AllocSize() const { return _allocSize; }
    uint32  WriteSize() const { return _writeSize; }
    void    Close(uint32 writeSize);

    // ★ Getter 추가
    SendBufferChunkRef GetOwner() const { return _owner; }

private:
    BYTE* _buffer = nullptr;
    uint32              _allocSize = 0;
    uint32              _writeSize = 0;   // 항상 0으로 초기화
    SendBufferChunkRef  _owner;
};

/*--------------------
    SendBufferChunk
--------------------*/

class SendBufferChunk : public enable_shared_from_this<SendBufferChunk>
{
    enum { SEND_BUFFER_CHUNK_SIZE = 6000 };

public:
    SendBufferChunk();
    ~SendBufferChunk();

    void            Reset();
    SendBufferRef   Open(uint32 allocSize);
    void            Close(uint32 writeSize);

    bool            IsOpen() const { return _open; }
    BYTE* Buffer() { return &_buffer[_usedSize]; }
    uint32          FreeSize() const { return static_cast<uint32>(_buffer.size()) - _usedSize; }

private:
    Array<BYTE, SEND_BUFFER_CHUNK_SIZE> _buffer = {};
    bool        _open = false;
    uint32      _usedSize = 0;
};

/*---------------------
    SendBufferManager
----------------------*/

class SendBufferManager
{
public:
    SendBufferRef   Open(uint32 size);
    static void         PushGlobal(SendBufferChunk* buffer);
private:
    SendBufferChunkRef  Pop();
    void                Push(SendBufferChunkRef buffer);
    
    

private:
    USE_LOCK;
    Vector<SendBufferChunkRef> _sendBufferChunks;
};
