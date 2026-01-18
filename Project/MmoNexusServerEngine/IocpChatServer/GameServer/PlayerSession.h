// PlayerSession.h (REPLACE ALL)
#pragma once
#include "Session.h"
#include "JobQueue.h"
#include "Job.h"
#include "Protocol.pb.h"
#include "RoomActor.h"   //  currentRoom 타입
#include <atomic>
#include <mutex>

class PlayerSession : public PacketSession
{
public:
    PlayerSession()
    {
        _jobQueue = MakeShared<JobQueue>();
    }
    virtual ~PlayerSession() {};

    virtual void OnConnected() override;
    virtual void OnDisconnected() override;
    virtual void OnRecvPacket(BYTE* buffer, int32 len) override;
    virtual void OnSend(int32 len) override;
    virtual void Ping() override;

    void PushJob(shared_ptr<Job> job) { _jobQueue->Push(job); }

public:
    // ============================================================
    // [Player Identity] (Session은 PlayerRef 금지. ID만)
    // ============================================================
    uint64 GetPlayerId_AnyThread() const { return _playerId.load(std::memory_order_acquire); }

    // 웬만하면 Actor thread(Post 안)에서만 호출
    void SetPlayerId_ActorOnly(uint64 pid) { _playerId.store(pid, std::memory_order_release); }
    void ClearPlayerId_ActorOnly() { _playerId.store(0, std::memory_order_release); }

public:
    // ============================================================
    // [MAP CHANGE STATE] (그대로 유지)
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

    bool TryBeginMapChange(uint64 token, int32 targetChannelId, int32 targetMapId, int64 targetInstanceId, const Protocol::PositionInfo& spawn);
    bool TryConsumeMapChangeAck(uint64 token, int32& outTargetChannelId, int32& outTargetMapId, int64& outTargetInstanceId, Protocol::PositionInfo& outSpawn);


    void EndMapChange();
    void CancelMapChange();
    uint64 GetMapChangeToken() const;


    // ============================================================
    // [PENDING ENTER CONTEXT] (DB 응답 라우팅 키)
    // - DB 응답은 gameSessionId로 세션 찾고
    // - pendingChannelId로 "어느 Lobby로 Push할지" 결정한다.
    // ============================================================
public:
    // Actor thread(Post 안)에서 세팅하는 걸 원칙으로 하자.
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
    // [Actor Post API] (외부는 이것만)
    // ============================================================
    template<typename F>
    void Post(F&& fn)
    {
        auto self = static_pointer_cast<PlayerSession>(shared_from_this());
        _jobQueue->Push(MakeShared<Job>([self, fn = std::forward<F>(fn)]() mutable
            {
                fn(self);
            }));
    }

    //  Session 라우팅: "현재 Room"에만 던진다.
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
    //  Room Actor가 session->Post(...)로만 호출
    void SetCurrentRoom(RoomActorRef room) { _currentRoom = room; }

    void ClearCurrentRoom(RoomActorRef room)
    {
        if (!room)
        {
            _currentRoom.reset();
            return;
        }

        auto cur = _currentRoom.lock();
        if (cur && cur.get() == room.get())
            _currentRoom.reset();
    }

    // Actor thread에서만 읽는 용도(디버그/검증)
    RoomActorRef GetCurrentRoom_ActorOnly() const { return _currentRoom.lock(); }

private:
    void ResetMapChangeState_Locked();

private:
    shared_ptr<JobQueue> _jobQueue;

    //  playerId만 유지
    std::atomic<uint64> _playerId{ 0 };

    //  currentRoom 타입 통일
    std::weak_ptr<RoomActor> _currentRoom;

private:
    mutable std::mutex _mapChangeLock;
    std::atomic<int32> _mapChangeState{ MAP_CHANGE_NONE };

    uint64 _mapChangeToken = 0;
    int32 _pendingTargetMapId = 0;
    Protocol::PositionInfo _pendingSpawn;
    int64 _pendingTargetInstanceId = 0;
    int32 _pendingTargetChannelId = 0; // 추가

};
using PlayerSessionRef = std::shared_ptr<PlayerSession>;
