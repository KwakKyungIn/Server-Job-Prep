#pragma once
#include "Creature.h" // [Inheritance] 부모 클래스 포함
#include "Protocol.pb.h"

class PlayerSession;
// GameRoom은 Creature.h에서 전방 선언 되어 있거나 포함되어 있으면 생략 가능하지만, 안전하게 둠
class GameRoom;

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

protected:
	// [Core Data] Player 고유 데이터
	Protocol::PlayerInfo	_playerInfo;
	std::vector<Protocol::ItemInfo> _items;

	// [References]
	std::weak_ptr<PlayerSession> _session;

	// _room, _zoneIndex는 Creature로 이사 갔으니 삭제.
};