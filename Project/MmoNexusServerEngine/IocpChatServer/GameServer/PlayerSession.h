#pragma once
#include "Session.h"
#include "JobQueue.h"
#include "Job.h"
#include "Protocol.pb.h"
#include "RoomActor.h"
#include <atomic>
#include <mutex>

class PlayerSession : public PacketSession
{
public:
    PlayerSession()
    {
        // 비동기 작업 처리를 위한 JobQueue 생성
        // 이걸 써야 락 안 걸고도 스레드 안전하게 작업 가능
        _jobQueue = MakeShared<JobQueue>();
    }
    virtual ~PlayerSession() {};

    virtual void OnConnected() override;
    virtual void OnDisconnected() override;
    virtual void OnRecvPacket(BYTE* buffer, int32 len) override;
    virtual void OnSend(int32 len) override;
    virtual void Ping() override;

    // 외부에서 이 세션한테 일 시킬 때 쓰는 함수
    void PushJob(shared_ptr<Job> job) { _jobQueue->Push(job); }

public:
    // ============================================================
    // [Player Identity] 
    // Session은 Player 객체를 직접 참조하지 않고 ID만 들고 있음
    // 스마트 포인터 순환 참조 끊으려고 이렇게 설계함
    // ============================================================
    uint64 GetPlayerId_AnyThread() const { return _playerId.load(std::memory_order_acquire); }

    // ID 세팅은 웬만하면 Actor 스레드 내부에서만 하자
    void SetPlayerId_ActorOnly(uint64 pid) { _playerId.store(pid, std::memory_order_release); }
    void ClearPlayerId_ActorOnly() { _playerId.store(0, std::memory_order_release); }

public:
    // ============================================================
    // [MAP CHANGE STATE] (맵 이동 상태 머신)
    // 맵 이동 중에 패킷이 오거나 또 이동 요청이 오면 꼬이니까 상태를 둠
    // NONE -> WAITING_ACK (클라 응답 대기) -> SWITCHING (서버 처리 중) -> NONE
    // ============================================================
    enum : int32
    {
        MAP_CHANGE_NONE = 0,
        MAP_CHANGE_WAITING_ACK = 1,
        MAP_CHANGE_SWITCHING = 2,
    };

    bool IsMapChanging() const
    {
        return _mapChangeState.load(std::memory_order_acquire) != MAP_CHANGE_NONE;
    }
    bool IsWaitingMapAck() const
    {
        return _mapChangeState.load(std::memory_order_acquire) == MAP_CHANGE_WAITING_ACK;
    }
    bool IsSwitchingMap() const
    {
        return _mapChangeState.load(std::memory_order_acquire) == MAP_CHANGE_SWITCHING;
    }

    // 이동 시작 시도 (락 사용)
    bool TryBeginMapChange(uint64 token, int32 targetChannelId, int32 targetMapId, int64 targetInstanceId, const Protocol::PositionInfo& spawn);
    // 이동 확정 시도
    bool TryConsumeMapChangeAck(uint64 token, int32& outTargetChannelId, int32& outTargetMapId, int64& outTargetInstanceId, Protocol::PositionInfo& outSpawn);

    void EndMapChange();
    void CancelMapChange();
    uint64 GetMapChangeToken() const;


    // ============================================================
    // [PENDING ENTER CONTEXT] (DB 응답 라우팅 키)
    // 비동기 DB 작업 후 결과가 왔을 때, 어느 방으로 보내야 할지 기억해두는 변수들
    // ============================================================
public:
    // Actor 스레드(Post 안)에서 세팅하는 게 원칙
    void SetPendingEnter_ActorOnly(int32 channelId, int32 mapId, int64 instanceId)
    {
        _pendingEnterChannelId.store(channelId, std::memory_order_release);
        _pendingEnterMapId.store(mapId, std::memory_order_release);
        _pendingEnterInstanceId.store(instanceId, std::memory_order_release);
        _pendingEnterActive.store(true, std::memory_order_release);
    }

    bool HasPendingEnter_AnyThread() const
    {
        return _pendingEnterActive.load(std::memory_order_acquire);
    }

    int32 GetPendingChannelId_AnyThread() const
    {
        if (_pendingEnterActive.load(std::memory_order_acquire) == false)
            return 0;
        return _pendingEnterChannelId.load(std::memory_order_acquire);
    }

    // 입장 처리 끝나면 클리어
    void ClearPendingEnter_ActorOnly()
    {
        _pendingEnterActive.store(false, std::memory_order_release);
        _pendingEnterChannelId.store(0, std::memory_order_release);
        _pendingEnterMapId.store(0, std::memory_order_release);
        _pendingEnterInstanceId.store(0, std::memory_order_release);
    }

private:
    std::atomic<bool>  _pendingEnterActive{ false };
    std::atomic<int32> _pendingEnterChannelId{ 0 };
    std::atomic<int32> _pendingEnterMapId{ 0 };
    std::atomic<int64> _pendingEnterInstanceId{ 0 };


public:
    // ============================================================
    // [Actor Post API]
    // 람다 함수를 잡큐에 던져주는 헬퍼 함수. 이거 덕분에 코딩하기 편함
    // ============================================================
    template<typename F>
    void Post(F&& fn)
    {
        auto self = static_pointer_cast<PlayerSession>(shared_from_this());
        // 람다 캡처로 self를 잡아서 생명주기 유지
        _jobQueue->Push(ObjectPool<Job>::MakeShared([self, fn = std::forward<F>(fn)]() mutable
            {
                fn(self);
            }));
    }

    // 현재 내가 들어가 있는 Room에다가 일감을 던지는 함수
    // "나 지금 이 방에 있는데 이거 처리해줘"
    template<typename F>
    void PostRoom(F&& fn)
    {
        Post([fn = std::forward<F>(fn)](std::shared_ptr<PlayerSession> self) mutable
            {
                auto room = self->_currentRoom.lock();
                if (!room) return;
                fn(self, room);
            });
    }

public:
    // Room Actor가 session->Post(...)로만 호출해야 함
    void SetCurrentRoom(RoomActorRef room) { _currentRoom = room; }

    // 방에서 나갈 때 호출
    void ClearCurrentRoom(RoomActorRef room)
    {
        if (!room)
        {
            _currentRoom.reset();
            return;
        }

        // 다른 방으로 이미 바뀌었으면 냅두고, 그 방이 맞으면 지움
        auto cur = _currentRoom.lock();
        if (cur && cur.get() == room.get())
            _currentRoom.reset();
    }

    // 디버깅이나 검증용으로만 사용 (ActorOnly)
    RoomActorRef GetCurrentRoom_ActorOnly() const { return _currentRoom.lock(); }

private:
    void ResetMapChangeState_Locked();

private:
    shared_ptr<JobQueue> _jobQueue;

    std::atomic<uint64> _playerId{ 0 };

    // 현재 있는 방 (weak_ptr로 잡아서 순환참조 방지)
    std::weak_ptr<RoomActor> _currentRoom;

private:
    mutable std::mutex _mapChangeLock;
    std::atomic<int32> _mapChangeState{ MAP_CHANGE_NONE };

    // 맵 변경 중 임시 저장 데이터
    uint64 _mapChangeToken = 0;
    int32 _pendingTargetMapId = 0;
    Protocol::PositionInfo _pendingSpawn;
    int64 _pendingTargetInstanceId = 0;
    int32 _pendingTargetChannelId = 0;

};
using PlayerSessionRef = std::shared_ptr<PlayerSession>;