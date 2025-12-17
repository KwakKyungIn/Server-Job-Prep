#pragma once
#include "JobQueue.h"
#include "Protocol.pb.h"
#include "SpatialGrid.h"
#include "BattleSystem.h"

class GameMap;
class Player;
class Monster;
class Creature;

using GameMapRef = std::shared_ptr<GameMap>;
using PlayerRef = std::shared_ptr<Player>;
using MonsterRef = std::shared_ptr<Monster>;

class GameRoom : public std::enable_shared_from_this<GameRoom>
{
public:
    GameRoom();
    virtual ~GameRoom();

    void Init(int32 channelId, int32 mapId, int32 sizeX, int32 sizeY, int32 zoneSize = 50);
    void Update();

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
    // [Content Logic]
    void Enter(PlayerSessionRef session);
    void Leave(PlayerSessionRef session);
    void HandleMove(PlayerSessionRef session, Protocol::C_MOVE pkt);

    void EnterMonster(MonsterRef monster);
    void LeaveMonster(uint64 objectId);

    bool EnterRegister(PlayerSessionRef session); // 등록만
    void SendEnterSpawns(PlayerSessionRef session); // 스폰만
    void EnterMapChange(PlayerSessionRef session); // 맵이동


    PlayerRef  FindNearestPlayer(Protocol::PositionInfo* pos, float range);
    GameMapRef GetMap() { return _map; }

    void BroadcastToZone(SendBufferRef sendBuffer, int32 zoneIndex, uint64 exceptId = 0);
    void Broadcast(SendBufferRef sendBuffer, uint64 exceptId = 0);

    // 스킬 판정
    void HandleSkill(std::shared_ptr<Creature> attacker, int32 skillId);
    void OnMonsterMoved(MonsterRef monster);

    //(포션)아이템
    void HandleUseItem(PlayerSessionRef session, Protocol::C_USE_ITEM pkt);

    // 보상 + 몬스터 제거(Despawn)까지 룸에서 직렬 처리
    void HandleMonsterDead(std::shared_ptr<Creature> attacker, MonsterRef monster);


private:
    GameMapRef              _map;
    std::shared_ptr<JobQueue> _jobQueue;

    Map<uint64, PlayerRef>  _players;
    Map<uint64, MonsterRef> _monsters;

    SpatialGrid             _grid;      // AOI 책임

    int32 _channelId = 1;
    int32 _mapId = 1;


    std::unique_ptr<BattleSystem> _battle; // 전투 로직 엔진

};
