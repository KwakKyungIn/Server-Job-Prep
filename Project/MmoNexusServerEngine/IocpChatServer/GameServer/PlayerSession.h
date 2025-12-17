#pragma once
#include "Session.h"
#include "JobQueue.h"
#include "Job.h"
#include "Protocol.pb.h"
#include "Player.h" // [중요] Player 클래스 정의를 알아야 위임 가능

#include <atomic>
#include <mutex>

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
	virtual void Ping() override;

	void PushJob(shared_ptr<Job> job) { _jobQueue->Push(job); }

public:
	// [Logic Object Link]
	void SetPlayer(shared_ptr<Player> player) { _player = player; }
	shared_ptr<Player> GetPlayer() { return _player; }

	// ============================================================
	// [Helper / Wrapper] 기존 코드 호환성 + 안전장치
	// ============================================================

	Protocol::PlayerInfo* GetPlayerInfo()
	{
		if (_player)
			return _player->GetPlayerInfo();
		return nullptr;
	}

	uint64 GetPlayerId()
	{
		if (_player) return _player->GetPlayerId();
		return 0;
	}

	shared_ptr<GameRoom> GetRoom()
	{
		if (_player) return _player->GetRoom();
		return nullptr;
	}

public:
	// ============================================================
	// [MAP CHANGE STATE] 2-step 프로토콜 지원
	//   REQ  -> S_BEGIN(token)
	//   ACK  -> S_END(token) + room enter 완료 시 EndMapChange()
	// ============================================================

	enum : int32
	{
		MAP_CHANGE_NONE = 0,
		MAP_CHANGE_WAITING_ACK = 1,
		MAP_CHANGE_SWITCHING = 2,
	};

	// 네트워크 스레드에서 빠르게 체크(입력 차단용)
	bool IsMapChanging() const
	{
		return _mapChangeState.load(std::memory_order_acquire) != MAP_CHANGE_NONE;
	}
	bool IsWaitingMapAck() const
	{
		return _mapChangeState.load(std::memory_order_acquire) == MAP_CHANGE_WAITING_ACK;
	}
	bool IsSwitchingMap() const
	{
		return _mapChangeState.load(std::memory_order_acquire) == MAP_CHANGE_SWITCHING;
	}

	// REQ 처리 시 호출: 이미 진행 중이면 false
	bool TryBeginMapChange(uint64 token, int32 targetMapId, const Protocol::PositionInfo& spawn)
	{
		std::lock_guard<std::mutex> lock(_mapChangeLock);

		if (_mapChangeState.load(std::memory_order_relaxed) != MAP_CHANGE_NONE)
			return false;

		_mapChangeToken = token;
		_pendingTargetMapId = targetMapId;
		_pendingSpawn.CopyFrom(spawn);

		_mapChangeState.store(MAP_CHANGE_WAITING_ACK, std::memory_order_release);
		return true;
	}

	// ACK 처리 시 호출: 토큰 검증 + 상태 전이
	// 성공하면 out 값 채워주고 SWITCHING 상태로 바뀜
	bool TryConsumeMapChangeAck(uint64 token, int32& outTargetMapId, Protocol::PositionInfo& outSpawn)
	{
		std::lock_guard<std::mutex> lock(_mapChangeLock);

		if (_mapChangeState.load(std::memory_order_relaxed) != MAP_CHANGE_WAITING_ACK)
			return false;

		if (_mapChangeToken != token)
			return false;

		outTargetMapId = _pendingTargetMapId;
		outSpawn.CopyFrom(_pendingSpawn);

		_mapChangeState.store(MAP_CHANGE_SWITCHING, std::memory_order_release);
		return true;
	}

	// 맵 전환이 완전히 끝났을 때(새 룸 Enter까지 끝난 시점) 호출
	void EndMapChange()
	{
		std::lock_guard<std::mutex> lock(_mapChangeLock);
		ResetMapChangeState_Locked();
	}

	// 실패/취소/끊김 등 강제 리셋
	void CancelMapChange()
	{
		std::lock_guard<std::mutex> lock(_mapChangeLock);
		ResetMapChangeState_Locked();
	}

	// 필요하면 서버가 현재 토큰을 조회할 수도 있음(로그/디버그용)
	uint64 GetMapChangeToken() const
	{
		std::lock_guard<std::mutex> lock(_mapChangeLock);
		return _mapChangeToken;
	}

public:
	shared_ptr<JobQueue> _jobQueue;

protected:
	shared_ptr<Player> _player;

private:
	void ResetMapChangeState_Locked()
	{
		_mapChangeToken = 0;
		_pendingTargetMapId = 0;
		_pendingSpawn.Clear();

		_mapChangeState.store(MAP_CHANGE_NONE, std::memory_order_release);
	}

private:
	// [Map Change State]
	mutable std::mutex _mapChangeLock;
	std::atomic<int32> _mapChangeState{ MAP_CHANGE_NONE };

	uint64 _mapChangeToken = 0;
	int32 _pendingTargetMapId = 0;
	Protocol::PositionInfo _pendingSpawn;
};
