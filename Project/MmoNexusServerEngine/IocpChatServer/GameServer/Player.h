#pragma once
#include "Creature.h"
#include "Protocol.pb.h"

class PlayerSession;

// 플레이어 클래스
// 몬스터나 NPC와 공통된 기능은 Creature에 몰아넣고,
// 플레이어만 가진 기능(인벤토리, 세션 연동 등)은 여기서 구현함
// shared_from_this 기능도 부모인 Creature가 물려줘서 편하게 사용 가능
class Player : public Creature
{
public:
	Player();
	virtual ~Player();

	// 전투 관련 핵심 함수들 오버라이딩
	virtual void			OnDamaged(std::shared_ptr<Creature> attacker, int32 damage) override;
	virtual void			OnDead(std::shared_ptr<Creature> attacker) override;

	// 로그인 직후 데이터 세팅하는 함수
	void					Init(const Protocol::PlayerInfo& info);

	// 게임 루프용
	void					Update();
	// 장비 교체나 레벨업 시 스탯 다시 계산
	void					RefreshStats();

	// 채널 및 맵 정보 관리
	// 이 정보들은 방 이동할 때 중요하게 쓰임
	void SetChannelId(int32 channelId) { _channelId = channelId; }
	int32 GetChannelId() const { return _channelId; }

	void SetMapId(int32 mapId) { _mapId = mapId; }
	int32 GetMapId() const { return _mapId; }


public:
	// 인스턴스 던전 ID (0이면 일반 필드)
	void SetInstanceId(int64 instanceId) { _instanceId = instanceId; }
	int64 GetInstanceId() const { return _instanceId; }

	// 귀환 위치 저장
	// 마을로 돌아가거나 부활할 때 이 좌표로 보냄
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
	// 네트워크 세션 연결
	// Player는 로직 객체고, Session은 통신 객체임. 서로 알고 있어야 패킷을 보낼 수 있음
	void					SetSession(std::shared_ptr<PlayerSession> session) { _session = session; }
	std::shared_ptr<PlayerSession> GetSession() { return _session.lock(); }

	// 위치 관련 함수(SetRoom 등)는 부모 클래스에 다 있어서 삭제함

public:
	// 데이터 접근용 게터
	uint64					GetPlayerId() const { return _playerInfo.playerid(); } // DB ID
	std::string				GetName() const { return _playerInfo.name(); }

	Protocol::PlayerInfo* GetPlayerInfo() { return &_playerInfo; }

	// 위치랑 스탯 정보는 부모 클래스 거 쓰면 됨

	// 편의 함수. 내부 포인터 값 변경
	void					SetPosInfo(const Protocol::PositionInfo& posInfo)
	{
		if (_posInfo)
			*_posInfo = posInfo;
	}

public:
	// ===== AOI (Area of Interest) 관리 =====
	// 주의: 아래 함수들은 반드시 Room 스레드 안에서만 호출해야 함 (동기화 문제 방지)
	// _ActorOnly 접미사를 붙여서 실수하지 않게 표시해둠

	// 내 주변에 보이는 플레이어 목록
	HashSet<uint64>& VisiblePlayers_ActorOnly() { return _visiblePlayers; }
	// 내 주변에 보이는 몬스터 목록
	HashSet<uint64>& VisibleMonsters_ActorOnly() { return _visibleMonsters; }

	// 내 주변에 날아다니는 투사체 목록
	HashSet<uint64>& VisibleProjectiles_ActorOnly() { return _visibleProjectiles; }
	const HashSet<uint64>& VisibleProjectiles_ActorOnly() const { return _visibleProjectiles; }

	// 시야 갱신 최적화를 위해 마지막으로 체크한 위치와 시간을 저장
	const Protocol::PositionInfo& LastAoiPos_ActorOnly() const { return _lastAoiPos; }
	void SetLastAoiPos_ActorOnly(const Protocol::PositionInfo& p) { _lastAoiPos = p; }

	uint64 LastAoiTickMs_ActorOnly() const { return _lastAoiTickMs; }
	void SetLastAoiTickMs_ActorOnly(uint64 v) { _lastAoiTickMs = v; }

	// 패킷 순서 보장용 시퀀스
	uint32 SnapshotSeq_ActorOnly() const { return _snapshotSeq; }
	uint32 NextSnapshotSeq_ActorOnly() { return ++_snapshotSeq; }

public:
	// [인벤토리]
	// 아이템 목록 통째로 세팅
	void SetItems(const google::protobuf::RepeatedPtrField<Protocol::ItemInfo>& items)
	{
		_items.clear();
		for (const auto& item : items)
		{
			_items.push_back(item);
		}
	}

	Vector<Protocol::ItemInfo>& GetItems() { return _items; }

	// [거래 시스템]
	// 현재 거래 중인 상대방 ID. 0이면 거래 중 아님
	uint64 ActiveTradeId_ActorOnly() const { return _activeTradeId; }
	void SetActiveTradeId_ActorOnly(uint64 v) { _activeTradeId = v; }

	// ===== 이동 검증 (Move Validation) =====
	// 클라이언트가 스피드핵 쓰거나 순간이동 하는 거 막기 위한 검증 데이터들
	// 역시 Room 스레드에서만 접근해야 함

	bool HasMoveStamp_ActorOnly() const { return _hasMoveStamp; }
	uint32 LastMoveSeq_ActorOnly() const { return _lastMoveSeq; }
	uint32 LastClientTimeMs_ActorOnly() const { return _lastClientTimeMs; }

	const Protocol::PositionInfo& LastAcceptedPos_ActorOnly() const { return _lastAcceptedPos; }
	uint64 LastAcceptedServerMs_ActorOnly() const { return _lastAcceptedServerMs; }

	// 유효한 이동으로 판정되면 기록을 갱신함
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

	// 맵 이동하거나 텔레포트 할 때 이동 검증 정보를 초기화
	void ResetMoveStamp_ActorOnly()
	{
		_hasMoveStamp = false;
		_lastMoveSeq = 0;
		_lastClientTimeMs = 0;
		_lastAcceptedServerMs = 0;
		_lastAcceptedPos.Clear();
	}

protected:
	// [Core Data] 플레이어 핵심 데이터
	// Protobuf 객체를 그대로 멤버로 들고 있음
	Protocol::PlayerInfo	_playerInfo;
	Vector<Protocol::ItemInfo> _items;
	uint64 _activeTradeId = 0;

	// [References]
	// 순환 참조 막기 위해 weak_ptr 사용
	std::weak_ptr<PlayerSession> _session;

	// 위치 정보들
	int32 _channelId = 1;
	int32 _mapId = 1;
	int64 _instanceId = 0;
	int32 _returnMapId = 1;
	int64 _returnInstanceId = 0;
	Protocol::PositionInfo _returnPos;

	// ===== 이동 검증 변수들 =====
	bool _hasMoveStamp = false;
	uint32 _lastMoveSeq = 0;
	uint32 _lastClientTimeMs = 0;
	uint64 _lastAcceptedServerMs = 0;     // 서버 시간 기준 체크
	Protocol::PositionInfo _lastAcceptedPos; // 마지막으로 허용된 위치

private:
	// AOI 목록 (ID만 저장)
	HashSet<uint64> _visiblePlayers;
	HashSet<uint64> _visibleMonsters;
	HashSet<uint64> _visibleProjectiles;

	// AOI 갱신 지연 처리를 위한 변수
	Protocol::PositionInfo _lastAoiPos;
	uint64 _lastAoiTickMs = 0;

	uint32 _snapshotSeq = 0;
};