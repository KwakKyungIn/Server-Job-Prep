#pragma once
#include "RoomActor.h"
#include "JobQueue.h"
#include "Job.h"
#include "Protocol.pb.h"
#include "ClientPacketHandler.h" 
#include "S2SPacketHandler.h"
#include <unordered_map>
#include <memory>
#include <functional>
#include <string>

// forward declaration으로 컴파일 속도 최적화
class Player;
class PlayerSession;

using PlayerRef = std::shared_ptr<Player>;
using PlayerSessionRef = std::shared_ptr<PlayerSession>;

// 로비 룸 클래스
// 실제 게임 공간(좌표가 있는 맵)이 아니라, 플레이어가 접속 후 
// 데이터를 로딩하고 준비될 때까지 대기하는 '논리적인' 공간임.
// JobQueue를 상속받거나 포함하여 비동기 처리를 보장해야 함.
class LobbyRoom : public RoomActor, public std::enable_shared_from_this<LobbyRoom>
{
public:
    LobbyRoom()
    {
        _jobQueue = MakeShared<JobQueue>();
    }

    virtual RoomKind GetKind() const override { return RoomKind::Lobby; }

    // JobQueue 패턴 적용: 외부에서 호출 시 락 대신 Job을 밀어넣는 방식
    virtual void Push(std::function<void()> fn) override
    {
        _jobQueue->Push(ObjectPool<Job>::MakeShared([fn = std::move(fn)]() mutable { fn(); }));
    }

public:
    void Init(int32 channelId) { _channelId = channelId; }
    int32 GetChannelId() const { return _channelId; }

    // =========================================================
    // 접속 및 데이터 로딩 파이프라인 API
    // - EnterGame: 최초 진입점. 빈 껍데기 플레이어 생성
    // - OnXXXLoaded: DB/Redis에서 데이터 가져온 후 콜백 처리
    // - TryEnterWorldIfReady: 모든 데이터가 준비되면 실제 게임방으로 이관
    // =========================================================
    void EnterGame(PlayerSessionRef ps, uint64 playerId, int32 channelId, int32 mapId, const Protocol::PositionInfo& spawn, const std::string& playerName);
    void OnItemsLoaded(uint64 playerId, const Protocol::S2S_RES_ITEMS_LOAD& pkt);
    void OnStatLoaded(uint64 playerId, const Protocol::S2S_RES_LOAD_PLAYER_DATA& pkt);
    void TryEnterWorldIfReady(uint64 playerId); // Actor thread 전용

    // 맵 이동(Transfer)이나 로딩 완료 여부 체크
    bool      IsReady(uint64 playerId) const;      // Actor thread 전용
    PlayerRef DetachIfReady(uint64 playerId);      // Actor thread 전용

public:
    // 플레이어 객체 관리 (소유권 이전용)
    // Adopt: 로비가 플레이어 관리 권한을 가져옴
    // Detach: 로비에서 플레이어 관리 권한을 놓음 (월드로 이동 시)
    void Adopt(PlayerRef player, bool isTransfer = false);
    PlayerRef Detach(uint64 playerId);
    PlayerRef Find(uint64 playerId) const;

    int32 GetCount() const { return static_cast<int32>(_players.size()); }

    void OnQuickSlotsLoaded(uint64 playerId, const Protocol::S2S_RES_QUICKSLOT_LOAD& pkt);

private:
    // 로딩 상태 추적용 구조체
    // DB 로딩은 비동기라 순서가 보장되지 않으므로, 플래그로 상태를 체크해야 함
    struct Pending
    {
        PlayerRef player;
        bool itemsLoaded = false;
        bool statLoaded = false;
        bool quickLoaded = false;
    };

private:
    int32 _channelId = 1;
    std::shared_ptr<JobQueue> _jobQueue;

    // 대기 중인 플레이어 목록
    // Actor Thread 내부에서만 접근하므로 별도의 Mutex가 필요 없음 (Lock-Free 로직의 기반)
    HashMap<uint64, Pending> _players;
};
