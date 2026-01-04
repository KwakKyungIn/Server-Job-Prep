// LobbyRoom.h
#pragma once
#include "RoomActor.h"
#include "JobQueue.h"
#include "Job.h"
#include "Protocol.pb.h"
#include "ClientPacketHandler.h"  // 인벤 동기화 보내려면 필요
#include "S2SPacketHandler.h"
#include <unordered_map>
#include <memory>
#include <functional>

// forward
class Player;
class PlayerSession;

using PlayerRef = std::shared_ptr<Player>;
using PlayerSessionRef = std::shared_ptr<PlayerSession>;

class LobbyRoom : public RoomActor, public std::enable_shared_from_this<LobbyRoom>
{
public:
    LobbyRoom()
    {
        _jobQueue = MakeShared<JobQueue>();
    }

    virtual RoomKind GetKind() const override { return RoomKind::Lobby; }

    virtual void Push(std::function<void()> fn) override
    {
        _jobQueue->Push(MakeShared<Job>([fn = std::move(fn)]() mutable { fn(); }));
    }

public:
    void Init(int32 channelId) { _channelId = channelId; }
    int32 GetChannelId() const { return _channelId; }

    // =========================================================
    // Step2 핵심 API
    // - EnterGame 시점: Player 생성/소유 (Session은 PlayerRef 소유 X)
    // - DB 응답 시점: Lobby가 데이터 흡수
    // =========================================================
    void EnterGame(PlayerSessionRef ps, uint64 playerId, int32 channelId, int32 mapId, const Protocol::PositionInfo& spawn);
    void OnItemsLoaded(uint64 playerId, const Protocol::S2S_RES_ITEMS_LOAD& pkt);
    void OnStatLoaded(uint64 playerId, const Protocol::S2S_RES_LOAD_PLAYER_DATA& pkt);
    void TryEnterWorldIfReady(uint64 playerId); // Actor thread only
    // Transfer 단계(네가 Step4에서 붙일 때 편하게)
    bool      IsReady(uint64 playerId) const;      // Actor thread only
    PlayerRef DetachIfReady(uint64 playerId);      // Actor thread only

public:
    // Lobby는 "보관/이동"만 한다. AOI/전투/아이템 사용 같은 로직 금지.
    void Adopt(PlayerRef player, bool isTransfer = false);
    PlayerRef Detach(uint64 playerId);
    PlayerRef Find(uint64 playerId) const;

    int32 GetCount() const { return static_cast<int32>(_players.size()); }


private:
    struct Pending
    {
        PlayerRef player;
        bool itemsLoaded = false;
        bool statLoaded = false;
    };

private:
    int32 _channelId = 1;
    std::shared_ptr<JobQueue> _jobQueue;

    // Actor thread에서만 접근(외부는 Push로만 호출)
    std::unordered_map<uint64, Pending> _players;
};
extern std::shared_ptr<LobbyRoom> GLobbyRoom;
