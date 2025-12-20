#pragma once
#include "Session.h"
#include "JobQueue.h"
#include "Job.h"
#include "Protocol.pb.h"
#include "Player.h" // [중요] Player 클래스 정의를 알아야 위임 가능

#include <atomic>
#include <mutex>

class GameRoom;

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

    // 외부가 잡 넣는 건 허용(Actor mailbox)
    void PushJob(shared_ptr<Job> job) { _jobQueue->Push(job); }

public:
    // [Logic Object Link]
    // ※ 원칙: SetPlayer도 가능하면 Post 안에서만 호출해라(네가 지켜주면 됨)
    void SetPlayer(shared_ptr<Player> player) { _player = player; }

public:
    // ============================================================
    // [MAP CHANGE STATE] (그대로)
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

    bool TryBeginMapChange(uint64 token, int32 targetMapId, const Protocol::PositionInfo& spawn);
    bool TryConsumeMapChangeAck(uint64 token, int32& outTargetMapId, Protocol::PositionInfo& outSpawn);

    void EndMapChange();
    void CancelMapChange();

    uint64 GetMapChangeToken() const;

public:
    // ============================================================
    // [Actor Post API] (외부는 이것만 써라)
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

    template<typename F>
    void PostPlayer(F&& fn)
    {
        Post([fn = std::forward<F>(fn)](PlayerSessionRef self) mutable
            {
                auto player = self->_player;
                if (!player) return;
                fn(self, player);
            });
    }

private:
    // ============================================================
    // [Helper / Wrapper]  <-- 이제 외부 접근 금지
    // ============================================================
    shared_ptr<Player> GetPlayer() { return _player; }

    Protocol::PlayerInfo* GetPlayerInfo()
    {
        if (_player) return _player->GetPlayerInfo();
        return nullptr;
    }

    uint64 GetPlayerId()
    {
        if (_player) return _player->GetPlayerId();
        return 0;
    }

    shared_ptr<GameRoom> GetRoom()
    {
        if (_player) return _player->GetRoom();
        return nullptr;
    }

private:
    void ResetMapChangeState_Locked();

private:
    // 이제 _jobQueue 외부 노출 금지
    shared_ptr<JobQueue> _jobQueue;

protected:
    shared_ptr<Player> _player;

private:
    mutable std::mutex _mapChangeLock;
    std::atomic<int32> _mapChangeState{ MAP_CHANGE_NONE };

    uint64 _mapChangeToken = 0;
    int32 _pendingTargetMapId = 0;
    Protocol::PositionInfo _pendingSpawn;
};
