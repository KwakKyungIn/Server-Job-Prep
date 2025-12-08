#pragma once
#include "Session.h"
#include "JobQueue.h"
#include "Job.h"
#include "Protocol.pb.h"
#include "Player.h" // [중요] Player 클래스 정의를 알아야 위임 가능

class GameRoom;

class PlayerSession : public PacketSession
{
public:
	PlayerSession()
	{
		_jobQueue = MakeShared<JobQueue>();
	}
	virtual ~PlayerSession() {};

	virtual void OnConnected() override;
	virtual void OnDisconnected() override;
	virtual void OnRecvPacket(BYTE* buffer, int32 len) override;
	virtual void OnSend(int32 len) override;

	void PushJob(shared_ptr<Job> job) { _jobQueue->Push(job); }

public:
	// [Logic Object Link]
	void SetPlayer(shared_ptr<Player> player) { _player = player; }
	shared_ptr<Player> GetPlayer() { return _player; }

	// ============================================================
	// [Helper / Wrapper] 기존 코드 호환성 + 안전장치
	// 이제 데이터는 _player가 들고 있으니, 걔한테 물어봐야 한다.
	// ============================================================

	// [Safety Fix] 연결된 플레이어가 없으면 nullptr 반환 (폭발 방지)
	Protocol::PlayerInfo* GetPlayerInfo()
	{
		if (_player)
			return _player->GetPlayerInfo();
		return nullptr;
	}

	// SessionID는 네트워크 고유값이니 세션이 관리하는 게 맞음 (PacketSession에 있음)
	// PlayerID는 게임 로직 값이니 Player한테 물어봄
	uint64 GetPlayerId()
	{
		if (_player) return _player->GetPlayerId();
		return 0;
	}

	// 방 정보도 Player가 관리
	shared_ptr<GameRoom> GetRoom()
	{
		if (_player) return _player->GetRoom();
		return nullptr;
	}

public:
	shared_ptr<JobQueue> _jobQueue;

protected:
	// [Only One Owner] 게임 로직 객체
	// 이제 _playerInfo, _room 변수는 삭제됨.
	shared_ptr<Player> _player;
};