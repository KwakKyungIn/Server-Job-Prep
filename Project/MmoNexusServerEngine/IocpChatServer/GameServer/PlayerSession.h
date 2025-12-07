#pragma once
#include "Session.h"
#include "JobQueue.h"
#include "Job.h"
#include "Protocol.pb.h" // PlayerInfo 구조체 사용

class GameRoom; // 전방 선언 (헤더 무거워짐 방지)

class PlayerSession : public PacketSession
{
public:
	PlayerSession()
	{
		// [Session-Local JobQueue]
		// 패킷 전송, DB 저장 등 '나만의' 작업을 처리하는 큐
		_jobQueue = MakeShared<JobQueue>();
	}

	virtual ~PlayerSession() {};

	virtual void OnConnected() override;
	virtual void OnDisconnected() override;
	virtual void OnRecvPacket(BYTE* buffer, int32 len) override;
	virtual void OnSend(int32 len) override;

	// [Interface] 외부에서 이 세션에게 일감을 던질 때 사용
	void PushJob(shared_ptr<Job> job) { _jobQueue->Push(job); }

public:
	// [Game Logic Integration]

	// 1. 내 정보 (DB ID, 좌표 등)
	void SetPlayerInfo(const Protocol::PlayerInfo& info) { _playerInfo = info; }
	Protocol::PlayerInfo* GetPlayerInfo() { return &_playerInfo; }

	// 2. 좌표 갱신 헬퍼 (Mutable 접근자 사용)
	void SetPosInfo(const Protocol::PositionInfo& posInfo)
	{
		*_playerInfo.mutable_posinfo() = posInfo;
	}

	// 3. 현재 내가 있는 방 (GameRoom)
	// [Memory Safety] 순환 참조(Cycle) 방지를 위해 반드시 weak_ptr 사용
	void SetRoom(shared_ptr<GameRoom> room) { _room = room; }
	shared_ptr<GameRoom> GetRoom() { return _room.lock(); }

	// 4. 유저 식별자 (편의 함수)
	uint64 GetPlayerId() const { return _playerInfo.playerid(); }

public:
	// 세션 전용 잡큐
	shared_ptr<JobQueue> _jobQueue;

private:
	// 실제 게임 데이터 (Protobuf 구조체)
	Protocol::PlayerInfo _playerInfo;

	// 내가 입장한 방 (Weak Reference)
	weak_ptr<GameRoom> _room;
};