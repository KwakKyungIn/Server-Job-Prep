#pragma once
#include "JobQueue.h"
#include "Protocol.pb.h"
#include "SpatialGrid.h"
#include "BattleSystem.h"
#include "RoomActor.h" 

class GameMap;
class Player;
class Monster;
class Creature;

class PlayerSession;
using PlayerSessionRef = std::shared_ptr<PlayerSession>;


using GameMapRef = std::shared_ptr<GameMap>;
using PlayerRef = std::shared_ptr<Player>;
using MonsterRef = std::shared_ptr<Monster>;

class GameRoom : public RoomActor, public std::enable_shared_from_this<GameRoom>
{
public:
    GameRoom();
    virtual ~GameRoom();

    void Init(int32 channelId, int32 mapId, int32 sizeX, int32 sizeY, int32 zoneSize = 50);
    void Update();

public:
    // =========================================================
    // [RoomActor Interface]
    // =========================================================
    virtual RoomKind GetKind() const override { return RoomKind::Game; }

    // 공통 Actor 실행 API (Session이 Room에 라우팅할 때 사용)
    virtual void Push(std::function<void()> fn) override
    {
        // GameRoom은 JobQueue로 직렬 실행
        _jobQueue->Push(MakeShared<Job>([fn = std::move(fn)]() mutable { fn(); }));
    }



public:
    // =========================================================
    // [Job System]
    // =========================================================

    // 1. [Lambda] 인자가 1개인 경우
    template<typename F>
    void PushJob(F&& job)
    {
        _jobQueue->Push(MakeShared<Job>(std::forward<F>(job)));
    }

    // 2. [Member Function] 인자가 2개 이상인 경우
    template<typename F, typename A, typename... Args>
    void PushJob(F func, A&& arg, Args&&... args)
    {
        _jobQueue->Push(MakeShared<Job>(shared_from_this(), func,
            std::forward<A>(arg), std::forward<Args>(args)...));
    }


public:
    // =========================================================
    // [Instance / Lifetime]
    // =========================================================
    void SetInstanceId(int64 instanceId) { _instanceId = instanceId; }
    int64 GetInstanceId() const { return _instanceId; }
    bool IsInstanceRoom() const { return _instanceId != 0; }

    void MarkClosing(bool value = true) { _closing.store(value, std::memory_order_release); }
    bool IsClosing() const { return _closing.load(std::memory_order_acquire); }

    int32 GetPlayerCountApprox() const { return _playerCount.load(std::memory_order_acquire); }

    // RoomManager purge 판단용 (다른 스레드에서 읽어도 안전)
    bool ShouldPurge(uint64 nowMs) const;


public:
    // [Content Logic]
    void Enter(PlayerSessionRef session, PlayerRef player);
    void Leave(PlayerSessionRef session, PlayerRef player);
    void HandleMove(PlayerSessionRef session, PlayerRef player,Protocol::C_MOVE pkt);

    void EnterMonster(MonsterRef monster);
    void LeaveMonster(uint64 objectId);

    bool EnterRegister(PlayerSessionRef session, PlayerRef player); // 등록만
    void SendEnterSpawns(PlayerSessionRef session, PlayerRef player); // 스폰만
    void EnterMapChange(PlayerSessionRef session, PlayerRef player); // 맵이동


    PlayerRef  FindNearestPlayer(Protocol::PositionInfo* pos, float range);
    GameMapRef GetMap() { return _map; }

    void BroadcastToZone(SendBufferRef sendBuffer, int32 zoneIndex, uint64 exceptId = 0);
    void Broadcast(SendBufferRef sendBuffer, uint64 exceptId = 0);

    // 스킬 판정
    void HandleSkill(std::shared_ptr<Creature> attacker, int32 skillId);
    void OnMonsterMoved(MonsterRef monster);

    //(포션)아이템
    void HandleUseItem(PlayerSessionRef session, PlayerRef player,Protocol::C_USE_ITEM pkt);

    // 보상 + 몬스터 제거(Despawn)까지 룸에서 직렬 처리
    void HandleMonsterDead(std::shared_ptr<Creature> attacker, MonsterRef monster);

    void BroadcastChat(const Protocol::S_CHAT_NTF& ntf);

    void LeaveById(PlayerSessionRef session, uint64 playerId);

    // Room thread(잡큐)에서만 호출
    PlayerRef FindPlayer_ActorOnly(uint64 playerId) const;
    // [ById] Session은 PlayerRef 금지. Room thread에서 Find 후 처리한다.
    void HandleMoveById(PlayerSessionRef session, uint64 playerId, Protocol::C_MOVE pkt);
    void HandleUseItemById(PlayerSessionRef session, uint64 playerId, Protocol::C_USE_ITEM pkt);
    void HandleEquipItemById(PlayerSessionRef session, uint64 playerId, Protocol::C_EQUIP_ITEM pkt);

    // ===== [ById Router] (Room thread에서 Find 후 처리) =====
    void HandleSkillById(PlayerSessionRef session, uint64 playerId, int32 skillId);
    void HandleChatById(PlayerSessionRef session, uint64 playerId, const std::string& msg);

    // ===== [MapChange] ACK 이후 실제 전이(OldRoom thread에서 수행) =====
    void TransferMapChangeById(PlayerSessionRef session,
        uint64 playerId,
        int32 targetMapId,
        int64 targetInstanceId,
        const Protocol::PositionInfo& spawn);

    void SaveReturnLocation_ActorOnly(uint64 playerId);

private:
    GameMapRef              _map;
    std::shared_ptr<JobQueue> _jobQueue;

    Map<uint64, PlayerRef>  _players;
    Map<uint64, MonsterRef> _monsters;

    SpatialGrid             _grid;      // AOI 책임

    int32 _channelId = 1;
    int32 _mapId = 1;

    // =========================================================
     //  Instance Lifetime State (RoomManager가 읽는 값들은 atomic)
   // =========================================================
    int64 _instanceId = 0; // 0 = world/field, >0 = instance
    std::atomic<int32> _playerCount{ 0 };
    std::atomic<bool>  _closing{ false };
    std::atomic<uint64> _emptySinceMs{ 0 }; // playerCount==0이 된 시점(지연 purge 용)


    std::unique_ptr<BattleSystem> _battle; // 전투 로직 엔진

};
