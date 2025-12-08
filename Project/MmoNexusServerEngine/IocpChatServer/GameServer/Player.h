#pragma once
#include "Protocol.pb.h"

class PlayerSession;
class GameRoom;

class Player : public enable_shared_from_this<Player>
{
public:
	Player();
	virtual ~Player();

	// [Init] 로그인 직후 기본 정보 세팅
	void					Init(const Protocol::PlayerInfo& info);

	// [Update] 주기적 실행
	void					Update();

	void RefreshStats();

public:
	// [Network Link]
	void					SetSession(std::shared_ptr<PlayerSession> session) { _session = session; }
	std::shared_ptr<PlayerSession> GetSession() { return _session.lock(); }

	// [Topology] 방 입장/퇴장 관리
	void					SetRoom(std::shared_ptr<GameRoom> room) { _room = room; }
	std::shared_ptr<GameRoom> GetRoom() { return _room.lock(); }

	// [Spatial - AOI]
	void					SetZoneIndex(int32 index) { _zoneIndex = index; }
	int32					GetZoneIndex() const { return _zoneIndex; }

public:
	// [Data Access]
	uint64					GetPlayerId() const { return _playerInfo.playerid(); }
	std::string				GetName() const { return _playerInfo.name(); }

	Protocol::PlayerInfo* GetPlayerInfo() { return &_playerInfo; }
	Protocol::PositionInfo* GetPosInfo() { return _playerInfo.mutable_posinfo(); }
	Protocol::StatInfo* GetStatInfo() { return _playerInfo.mutable_statinfo(); }

	// 좌표 갱신 (패킷 핸들러에서 호출)
	void					SetPosInfo(const Protocol::PositionInfo& posInfo)
	{
		*_playerInfo.mutable_posinfo() = posInfo;
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
	// [Core Data] 여기가 진짜 데이터 저장소다.
	Protocol::PlayerInfo	_playerInfo;
	std::vector<Protocol::ItemInfo> _items;

	// [References]
	std::weak_ptr<PlayerSession> _session;
	std::weak_ptr<GameRoom>		 _room;

	// [Logic State]
	int32					_zoneIndex = -1;
};