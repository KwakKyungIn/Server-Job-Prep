#pragma once
#include "JobQueue.h"
#include "Protocol.pb.h"

class GameMap;
class Player;
class Monster;
class Creature;

using GameMapRef = std::shared_ptr<GameMap>;
using PlayerRef = std::shared_ptr<Player>;
using MonsterRef = std::shared_ptr<Monster>;

struct Zone
{
	Set<PlayerRef> players;
	Set<MonsterRef> monsters;
};

class GameRoom : public enable_shared_from_this<GameRoom>
{
public:
	GameRoom();
	virtual ~GameRoom();

	void Init(int32 mapId, int32 sizeX, int32 sizeY, int32 zoneSize = 50);
	void Update();

public:
	// =========================================================
	// [Job System] GIGACHAD FIX (Conflict Resolved)
	// =========================================================

	// 1. [Lambda] 인자가 1개인 경우 (람다 객체 하나만 던질 때)
	// 사용: room->PushJob([=](){ player->UseSkill(1); });
	template<typename F>
	void PushJob(F&& job)
	{
		_jobQueue->Push(MakeShared<Job>(std::forward<F>(job)));
	}

	// 2. [Member Function] 인자가 2개 이상인 경우 (기존 코드 호환)
	// 사용: room->PushJob(&GameRoom::Enter, session);
	// F: 함수 포인터, A: 첫 번째 인자, Args: 나머지 인자들
	template<typename F, typename A, typename... Args>
	void PushJob(F func, A&& arg, Args&&... args)
	{
		// 여기서는 shared_from_this()를 Owner로 자동으로 넣어준다.
		_jobQueue->Push(MakeShared<Job>(shared_from_this(), func, std::forward<A>(arg), std::forward<Args>(args)...));
	}

public:
	// [Content Logic]
	void Enter(PlayerSessionRef session);
	void Leave(PlayerSessionRef session);
	void HandleMove(PlayerSessionRef session, Protocol::C_MOVE pkt);

	void EnterMonster(MonsterRef monster);
	void LeaveMonster(uint64 objectId);

	PlayerRef FindNearestPlayer(Protocol::PositionInfo* pos, float range);
	GameMapRef GetMap() { return _map; }

	void  BroadcastToZone(SendBufferRef sendBuffer, int32 zoneIndex, uint64 exceptId = 0);
	void  Broadcast(SendBufferRef sendBuffer, uint64 exceptId = 0);

	//======스킬 판정====
	void HandleSkill(std::shared_ptr<Creature> attacker, int32 skillId);

private:
	int32 GetZoneIndex(const Protocol::PositionInfo& posInfo);
	void  GetNearbyZones(int32 zoneIndex, Vector<Zone*>& outZones);
	void  GetNearbyZoneIndices(int32 zoneIndex, Vector<int32>& outIndices);

private:
	shared_ptr<GameMap> _map;
	shared_ptr<JobQueue> _jobQueue;

	Map<uint64, PlayerRef> _players;
	Map<uint64, MonsterRef> _monsters;

	Vector<Zone> _zones;
	int32 _gridSizeX = 0;
	int32 _gridSizeY = 0;
	int32 _zoneCellSize = 0;
};