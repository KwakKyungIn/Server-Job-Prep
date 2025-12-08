#pragma once
#include "JobQueue.h"
#include "Protocol.pb.h"

class GameMap;
class Player;
class Monster;

using GameMapRef = std::shared_ptr<GameMap>;
using PlayerRef = std::shared_ptr<Player>;
using MonsterRef = std::shared_ptr<Monster>;

// [Spatial Partitioning]
// 맵을 격자(Grid)로 쪼갠 하나의 구역
struct Zone
{
	Set<PlayerRef> players; // 이 구역에 위치한 플레이어 목록
	Set<MonsterRef> monsters;
};

class GameRoom : public enable_shared_from_this<GameRoom>
{
public:
	GameRoom();
	virtual ~GameRoom();

	// zoneSize: 격자 한 칸의 크기 (기본 50 추천)
	void Init(int32 mapId, int32 sizeX, int32 sizeY, int32 zoneSize = 50);
	void Update();

	// [Interface] 외부에서 Job을 넣는 함수
	template<typename F, typename... Args>
	void PushJob(F func, Args&&... args)
	{
		_jobQueue->Push(MakeShared<Job>(shared_from_this(), func, std::forward<Args>(args)...));
	}

public:
	// [Content Logic] JobQueue 안에서 순차 실행됨 (Lock 불필요)
	void Enter(PlayerSessionRef session);
	void Leave(PlayerSessionRef session);
	void HandleMove(PlayerSessionRef session, Protocol::C_MOVE pkt);

	void EnterMonster(MonsterRef monster);
	void LeaveMonster(uint64 objectId);

	PlayerRef FindNearestPlayer(Protocol::PositionInfo* pos, float range);

	GameMapRef GetMap() { return _map; }


	// Zone 기반 전송 (나를 제외한 해당 Zone 유저들에게 전송)
	void  BroadcastToZone(SendBufferRef sendBuffer, int32 zoneIndex, uint64 exceptId = 0);

	// 전체 전송 (공지사항, 전체 채팅 등 특수 목적)
	void  Broadcast(SendBufferRef sendBuffer, uint64 exceptId = 0);

private:
	// [AOI Helpers]
	int32 GetZoneIndex(const Protocol::PositionInfo& posInfo);
	void  GetNearbyZones(int32 zoneIndex, Vector<Zone*>& outZones);

	void  GetNearbyZoneIndices(int32 zoneIndex, Vector<int32>& outIndices);

private:
	shared_ptr<GameMap> _map;
	shared_ptr<JobQueue> _jobQueue;

	// 전체 플레이어 Lookup 용 (ID -> 객체)
	Map<uint64, PlayerRef> _players;
	Map<uint64, MonsterRef> _monsters;

	// [Spatial Management]
	Vector<Zone> _zones;    // 1차원 배열로 관리하는 2D 격자
	int32 _gridSizeX = 0;   // X축 Zone 개수
	int32 _gridSizeY = 0;   // Y축 Zone 개수
	int32 _zoneCellSize = 0; // Zone 한 칸의 길이
};