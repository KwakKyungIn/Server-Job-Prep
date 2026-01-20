#pragma once
#include "JobQueue.h"
#include "Protocol.pb.h"
#include "Protocol_S2S.pb.h"
#include "SpatialGrid.h"
#include "BattleSystem.h"
#include "RoomActor.h" 

class GameMap;
class Player;
class Monster;
class Creature;

class Projectile;



class PlayerSession;
using PlayerSessionRef = std::shared_ptr<PlayerSession>;


using GameMapRef = std::shared_ptr<GameMap>;
using PlayerRef = std::shared_ptr<Player>;
using MonsterRef = std::shared_ptr<Monster>;
using ProjectileRef = std::shared_ptr<Projectile>;

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

    // ���� Actor ���� API (Session�� Room�� ������� �� ���)
    virtual void Push(std::function<void()> fn) override
    {
        // GameRoom�� JobQueue�� ���� ����
        _jobQueue->Push(MakeShared<Job>([fn = std::move(fn)]() mutable { fn(); }));
    }



public:
    // =========================================================
    // [Job System]
    // =========================================================

    // 1. [Lambda] ���ڰ� 1���� ���
    template<typename F>
    void PushJob(F&& job)
    {
        _jobQueue->Push(MakeShared<Job>(std::forward<F>(job)));
    }

    // 2. [Member Function] ���ڰ� 2�� �̻��� ���
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

    // [Trade Phase 2] DBAgent commit response (must run on actor thread)
    void OnTradeCommitResult(Protocol::S2S_RES_TRADE_COMMIT pkt);
    bool IsInstanceRoom() const { return _instanceId != 0; }

    void MarkClosing(bool value = true) { _closing.store(value, std::memory_order_release); }
    bool IsClosing() const { return _closing.load(std::memory_order_acquire); }

    int32 GetPlayerCountApprox() const { return _playerCount.load(std::memory_order_acquire); }

    // RoomManager purge �Ǵܿ� (�ٸ� �����忡�� �о ����)
    bool ShouldPurge(uint64 nowMs) const;


public:
    // [Content Logic]
    void Enter(PlayerSessionRef session, PlayerRef player);
    void Leave(PlayerSessionRef session, PlayerRef player);
    void HandleMove(PlayerSessionRef session, PlayerRef player, Protocol::C_MOVE pkt);

    void EnterMonster(MonsterRef monster);
    void LeaveMonster(uint64 objectId);

    bool EnterRegister(PlayerSessionRef session, PlayerRef player); // ��ϸ�
    void SendEnterSpawns(PlayerSessionRef session, PlayerRef player); // ������
    void EnterMapChange(PlayerSessionRef session, PlayerRef player); // ���̵�


    PlayerRef  FindNearestPlayer(Protocol::PositionInfo* pos, float range);
    GameMapRef GetMap() { return _map; }

    void BroadcastToZone(SendBufferRef sendBuffer, int32 zoneIndex, uint64 exceptId = 0);
    void Broadcast(SendBufferRef sendBuffer, uint64 exceptId = 0);

    // ��ų ����
    void HandleSkill(std::shared_ptr<Creature> attacker, int32 skillId);
    void OnMonsterMoved(MonsterRef monster);

    //(����)������
    void HandleUseItem(PlayerSessionRef session, PlayerRef player, Protocol::C_USE_ITEM pkt);

    // Inventory drag & drop (move / swap / merge)
    void HandleInvDragDrop(PlayerSessionRef session, PlayerRef player, Protocol::C_INV_DRAG_DROP pkt);

    // ���� + ���� ����(Despawn)���� �뿡�� ���� ó��
    void HandleMonsterDead(std::shared_ptr<Creature> attacker, MonsterRef monster);

    void BroadcastChat(const Protocol::S_CHAT_NTF& ntf);

    void LeaveById(PlayerSessionRef session, uint64 playerId);

    // Room thread(��ť)������ ȣ��
    PlayerRef FindPlayer_ActorOnly(uint64 playerId) const;
    // [ById] Session�� PlayerRef ����. Room thread���� Find �� ó���Ѵ�.
    void HandleMoveById(PlayerSessionRef session, uint64 playerId, Protocol::C_MOVE pkt);
    void HandleUseItemById(PlayerSessionRef session, uint64 playerId, Protocol::C_USE_ITEM pkt);
    void HandleEquipItemById(PlayerSessionRef session, uint64 playerId, Protocol::C_EQUIP_ITEM pkt);
    void HandleInvDragDropById(PlayerSessionRef session, uint64 playerId, Protocol::C_INV_DRAG_DROP pkt);

    // ===== [Trade v1] =====
    void HandleTradeReqById(PlayerSessionRef session, uint64 fromPlayerId, uint64 targetPlayerId);
    void HandleTradeInviteRespById(PlayerSessionRef session, uint64 responderId, bool accept);
    void HandleTradeOfferSetById(PlayerSessionRef session, uint64 playerId, Protocol::C_TRADE_OFFER_SET pkt);
    void HandleTradeReadyById(PlayerSessionRef session, uint64 playerId, Protocol::C_TRADE_READY pkt);
    void HandleTradeConfirmById(PlayerSessionRef session, uint64 playerId, Protocol::C_TRADE_CONFIRM pkt);
    void HandleTradeCancelById(PlayerSessionRef session, uint64 playerId, Protocol::C_TRADE_CANCEL pkt);

    // ===== [ById Router] (Room thread���� Find �� ó��) =====
    void HandleSkillById(PlayerSessionRef session, uint64 playerId, int32 skillId);
    void HandleChatById(PlayerSessionRef session, uint64 playerId, const std::string& msg);

    // ===== [MapChange] ACK ���� ���� ����(OldRoom thread���� ����) =====
    void TransferMapChangeById(PlayerSessionRef session,
        uint64 playerId,
        int32 targetChannelId,
        int32 targetMapId,
        int64 targetInstanceId,
        const Protocol::PositionInfo& spawn);

public:
    void EnterProjectile(ProjectileRef p);
    void LeaveProjectile(uint64 projectileId);
    void OnProjectileMoved(ProjectileRef p);
    void UpdateProjectiles(uint64 deltaMs);

    void SaveReturnLocation_ActorOnly(uint64 playerId);

    void HandleSkill(std::shared_ptr<Creature> attacker, int32 skillId, float castYaw, uint32 clientTimeMs);
    void HandleSkillById(PlayerSessionRef session, uint64 playerId, int32 skillId, float castYaw, uint32 clientTimeMs);

    int32 GetChannelId() const { return _channelId; }
    int32 GetMapId() const { return _mapId; }

private:
    GameMapRef              _map;
    std::shared_ptr<JobQueue> _jobQueue;

    Map<uint64, PlayerRef>  _players;
    Map<uint64, MonsterRef> _monsters;

    SpatialGrid             _grid;      // AOI å��

    int32 _channelId = 1;
    int32 _mapId = 1;

    // =========================================================
     //  Instance Lifetime State (RoomManager�� �д� ������ atomic)
   // =========================================================
    int64 _instanceId = 0; // 0 = world/field, >0 = instance
    std::atomic<int32> _playerCount{ 0 };
    std::atomic<bool>  _closing{ false };
    std::atomic<uint64> _emptySinceMs{ 0 }; // playerCount==0�� �� ����(���� purge ��)


    std::unique_ptr<BattleSystem> _battle; // ���� ���� ����
private:
    // ===== AOI v2 Params (�ϴ� ������) =====
    int32 _aoiNeighborRadiusCells = 2; // 5x5
    float _interestRadius = 150.f;
    float _lazyUpdateDist = 10.f;      // 10m
    uint64 _lazyUpdateTickMs = 500;    // 0.5s

    int32 _batchSpawnPlayers = 50;
    int32 _batchSpawnMonsters = 100;
    int32 _batchDespawn = 200;

private:
    void UpdateAOI(PlayerSessionRef session, PlayerRef me, bool forceFullSnapshot);
    bool ShouldUpdateAOI(PlayerRef me, bool zoneChanged) const;

    void CollectCandidates(int32 zoneIndex, Vector<PlayerRef>& outPlayers, Vector<MonsterRef>& outMonsters);

    bool PassDistance2D(const Protocol::PositionInfo& a, const Protocol::PositionInfo& b, float r) const;

    uint32 GetConnectivityId_ActorOnly(const Protocol::PositionInfo& pos) const;


    // ��Ī ����
    void SendSpawnBatchedToMe(PlayerSessionRef session,
        const Vector<PlayerRef>& spawnPlayers,
        const Vector<MonsterRef>& spawnMonsters,
        bool snapshotMode,
        uint32 snapshotId);

    void SendDespawnBatchedToMe(PlayerSessionRef session, const Vector<uint64>& objectIds);

    int32 EffectiveAoiRadiusCells() const;


    // =========================================================
    // Trade v1 (Phase 1: In-memory commit + FlushNow)
    //  - All trade logic runs on GameRoom actor thread
    // =========================================================
    enum class TradeState : uint8
    {
        None = 0,
        Invited = 1,
        Active = 2,
        Locked = 3,
        Committing = 4,
    };

    struct TradeOfferEntry
    {
        uint64 itemUid = 0;
        int32 templateId = 0;
        int32 count = 0;
    };

    struct TradeCommitPlan
    {
        // Final snapshots for DBAgent (atomic SQL transaction)
        std::vector<Protocol::ItemInfo> finalAItems;
        std::vector<uint64> deletedAItemUids;
        std::vector<Protocol::ItemInfo> finalBItems;
        std::vector<uint64> deletedBItemUids;

        // Client notifications (applied after DB success)
        std::vector<Protocol::ItemInfo> notifyChangeA;
        std::vector<uint64> notifyRemoveA;
        std::vector<Protocol::ItemInfo> notifyChangeB;
        std::vector<uint64> notifyRemoveB;
    };

    struct TradeSession
    {
        uint64 tradeId = 0;
        uint64 playerAId = 0;
        uint64 playerBId = 0;

        std::unordered_map<uint64, TradeOfferEntry> offerA; // itemUid -> entry
        std::unordered_map<uint64, TradeOfferEntry> offerB;

        bool readyA = false;
        bool readyB = false;
        bool confirmA = false;
        bool confirmB = false;

        TradeState state = TradeState::None;

        uint64 createdAtMs = 0;
        uint64 lastTouchedMs = 0;

        // Phase 2: pending DB atomic commit (Actor thread only)
        uint64 commitRequestId = 0;
        std::unique_ptr<TradeCommitPlan> commitPlan;
    };

    void UpdateTrades_ActorOnly(uint64 nowMs);
    void CancelTrade_ActorOnly(uint64 tradeId, Protocol::TradeCancelReason reason, Protocol::TradeFailCode failCode = Protocol::TRADE_FAIL_NONE, const std::string& msg = "");
    void SendOfferUpdate_ActorOnly(uint64 tradeId, uint64 whoPlayerId);
    void SendReadyState_ActorOnly(uint64 tradeId);

    bool BuildTradeCommitPlan_ActorOnly(uint64 tradeId, TradeCommitPlan& outPlan, Protocol::TradeFailCode& outFail, std::string& outMsg);
    bool StartTradeCommitPhase2_ActorOnly(uint64 tradeId, Protocol::TradeFailCode& outFail, std::string& outMsg);
    void OnTradeCommitResult_ActorOnly(const Protocol::S2S_RES_TRADE_COMMIT& pkt);

    TradeSession* FindTrade_ActorOnly(uint64 tradeId);
    TradeSession* FindTradeByPlayer_ActorOnly(uint64 playerId);

    static constexpr int32 kTradeMaxInventorySlots = 24;
    static constexpr uint64 kTradeTimeoutMs = 60'000;

    std::unordered_map<uint64, TradeSession> _trades;
    std::unordered_map<uint64, uint64> _tradeByPlayer;

    Map<uint64, ProjectileRef> _projectiles;
    uint64 _lastUpdateMs = 0;

};
