#pragma once
#include "Creature.h" // [Inheritance] 부모 클래스 포함
#include "Protocol.pb.h"

class PlayerSession;

// [Inheritance] Creature 상속 (이제 shared_from_this 기능도 Creature가 물려줌)
class Player : public Creature
{
public:
	Player();
	virtual ~Player();

	// [Override] 전투 시스템 필수 함수 재정의
	virtual void			OnDamaged(std::shared_ptr<Creature> attacker, int32 damage) override;
	virtual void			OnDead(std::shared_ptr<Creature> attacker) override;

	// [Init] 로그인 직후 기본 정보 세팅
	void					Init(const Protocol::PlayerInfo& info);

	// [Update] 주기적 실행
	void					Update();
	void					RefreshStats();

	// [NEW] 채널/맵 메타 정보
	void SetChannelId(int32 channelId) { _channelId = channelId; }
	int32 GetChannelId() const { return _channelId; }

	void SetMapId(int32 mapId) { _mapId = mapId; }
	int32 GetMapId() const { return _mapId; }


public:
	void SetInstanceId(int64 instanceId) { _instanceId = instanceId; }
	int64 GetInstanceId() const { return _instanceId; }

	void SetReturnLocation(int32 mapId, int64 instanceId, const Protocol::PositionInfo& pos)
	{
		_returnMapId = mapId;
		_returnInstanceId = instanceId;
		_returnPos.CopyFrom(pos);
	}

	int32 GetReturnMapId() const { return _returnMapId; }
	int64 GetReturnInstanceId() const { return _returnInstanceId; }
	const Protocol::PositionInfo& GetReturnPos() const { return _returnPos; }

public:
	// [Network Link] Player만의 고유 기능 (세션 연결)
	void					SetSession(std::shared_ptr<PlayerSession> session) { _session = session; }
	std::shared_ptr<PlayerSession> GetSession() { return _session.lock(); }

	// [Topology] & [Spatial]
	// SetRoom, GetRoom, SetZoneIndex, GetZoneIndex는 
	// 부모 클래스(Creature)에 이미 구현되어 있으므로 삭제함. (자동 상속)

public:
	// [Data Access]
	uint64					GetPlayerId() const { return _playerInfo.playerid(); } // DB ID
	std::string				GetName() const { return _playerInfo.name(); }

	Protocol::PlayerInfo* GetPlayerInfo() { return &_playerInfo; }

	// GetPosInfo(), GetStatInfo()는 부모(Creature) 것을 그대로 사용.

	// 편의상 남겨둠 (내부 포인터 사용)
	void					SetPosInfo(const Protocol::PositionInfo& posInfo)
	{
		if (_posInfo)
			*_posInfo = posInfo;
	}

public:
	// ===== AOI v2 (Room thread ONLY) =====
	std::unordered_set<uint64>& VisiblePlayers_ActorOnly() { return _visiblePlayers; }
	std::unordered_set<uint64>& VisibleMonsters_ActorOnly() { return _visibleMonsters; }

	//  [NEW] Projectile visible set
	std::unordered_set<uint64>& VisibleProjectiles_ActorOnly() { return _visibleProjectiles; }
	const std::unordered_set<uint64>& VisibleProjectiles_ActorOnly() const { return _visibleProjectiles; }

	const Protocol::PositionInfo& LastAoiPos_ActorOnly() const { return _lastAoiPos; }
	void SetLastAoiPos_ActorOnly(const Protocol::PositionInfo& p) { _lastAoiPos = p; }

	uint64 LastAoiTickMs_ActorOnly() const { return _lastAoiTickMs; }
	void SetLastAoiTickMs_ActorOnly(uint64 v) { _lastAoiTickMs = v; }

	uint32 SnapshotSeq_ActorOnly() const { return _snapshotSeq; }
	uint32 NextSnapshotSeq_ActorOnly() { return ++_snapshotSeq; }

public:
	// [Inventory]
	void SetItems(const google::protobuf::RepeatedPtrField<Protocol::ItemInfo>& items)
	{
		_items.clear();
		for (const auto& item : items)
		{
			_items.push_back(item);
		}
	}

	std::vector<Protocol::ItemInfo>& GetItems() { return _items; }

	// [Trade] (Room thread ONLY)
	uint64 ActiveTradeId_ActorOnly() const { return _activeTradeId; }
	void SetActiveTradeId_ActorOnly(uint64 v) { _activeTradeId = v; }

	// ===== Move Validation (Room thread ONLY) =====
	bool HasMoveStamp_ActorOnly() const { return _hasMoveStamp; }

	uint32 LastMoveSeq_ActorOnly() const { return _lastMoveSeq; }
	uint32 LastClientTimeMs_ActorOnly() const { return _lastClientTimeMs; }

	const Protocol::PositionInfo& LastAcceptedPos_ActorOnly() const { return _lastAcceptedPos; }
	uint64 LastAcceptedServerMs_ActorOnly() const { return _lastAcceptedServerMs; }

	void SetMoveStamp_ActorOnly(uint32 seq, uint32 timeMs,
		const Protocol::PositionInfo& acceptedPos,
		uint64 serverMs)
	{
		_hasMoveStamp = true;
		_lastMoveSeq = seq;
		_lastClientTimeMs = timeMs;
		_lastAcceptedPos = acceptedPos;
		_lastAcceptedServerMs = serverMs;
	}

	void ResetMoveStamp_ActorOnly()
	{
		_hasMoveStamp = false;
		_lastMoveSeq = 0;
		_lastClientTimeMs = 0;
		_lastAcceptedServerMs = 0;
		_lastAcceptedPos.Clear();
	}




protected:
	// [Core Data] Player 고유 데이터
	Protocol::PlayerInfo	_playerInfo;
	std::vector<Protocol::ItemInfo> _items;
	uint64 _activeTradeId = 0;

	// [References]
	std::weak_ptr<PlayerSession> _session;

	int32 _channelId = 1;
	int32 _mapId = 1;
	int64 _instanceId = 0; // 0 = world
	int32 _returnMapId = 1;              // 기본 복귀 맵
	int64 _returnInstanceId = 0;         // 보통 0
	Protocol::PositionInfo _returnPos;   // 복귀 좌표

	// ===== Move Validation (Room thread ONLY) =====
	bool _hasMoveStamp = false;
	uint32 _lastMoveSeq = 0;
	uint32 _lastClientTimeMs = 0;
	uint64 _lastAcceptedServerMs = 0;     // debug
	Protocol::PositionInfo _lastAcceptedPos; // debug/anchor


	// _room, _zoneIndex는 Creature로 이사 갔으니 삭제.

private:
	// visible set (ids)
	std::unordered_set<uint64> _visiblePlayers;
	std::unordered_set<uint64> _visibleMonsters; // monster objectId
	std::unordered_set<uint64> _visibleProjectiles;
	// lazy update state
	Protocol::PositionInfo _lastAoiPos;
	uint64 _lastAoiTickMs = 0;

	// snapshot seq
	uint32 _snapshotSeq = 0;
};