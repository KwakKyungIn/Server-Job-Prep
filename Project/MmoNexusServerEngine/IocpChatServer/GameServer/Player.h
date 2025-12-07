#pragma once
#include "Protocol.pb.h"

class PlayerSession;
class GameRoom;

class Player : public enable_shared_from_this<Player>
{
public:
	Player();
	virtual ~Player();

	// [Init] 세션(DB)에 있는 유저 정보를 내 게임 데이터로 복사
	void					Init(const Protocol::PlayerInfo& info);

	// [Update] 주기적 실행 (필요시 GameRoom에서 호출)
	void					Update();

public:
	// [Network] 세션은 언제든 끊길 수 있으므로 weak_ptr
	void					SetSession(std::shared_ptr<PlayerSession> session) { _session = session; }
	std::shared_ptr<PlayerSession> GetSession() { return _session.lock(); }

	// [Topology] 내가 속한 방
	void					SetRoom(std::shared_ptr<GameRoom> room) { _room = room; }
	std::shared_ptr<GameRoom> GetRoom() { return _room.lock(); }

	// [Spatial - AOI] 현재 위치한 Grid Index
	void					SetZoneIndex(int32 index) { _zoneIndex = index; }
	int32					GetZoneIndex() const { return _zoneIndex; }

public:
	// [Data Access Wrapper]
	uint64					GetPlayerId() const { return _playerInfo.playerid(); }
	std::string				GetName() const { return _playerInfo.name(); }

	// 컨텐츠 코드에서 접근할 때 사용
	Protocol::PlayerInfo* GetPlayerInfo() { return &_playerInfo; }
	Protocol::PositionInfo* GetPosInfo() { return _playerInfo.mutable_posinfo(); }

	// 이동 패킷 처리 시 좌표 갱신
	void					SetPosInfo(const Protocol::PositionInfo& posInfo)
	{
		*_playerInfo.mutable_posinfo() = posInfo;
	}

protected:
	// 게임 로직에서 사용하는 실제 데이터
	Protocol::PlayerInfo	_playerInfo;

	// AOI Grid Index
	int32					_zoneIndex = -1;

	// 레퍼런스
	std::weak_ptr<PlayerSession> _session;
	std::weak_ptr<GameRoom>		 _room;
};