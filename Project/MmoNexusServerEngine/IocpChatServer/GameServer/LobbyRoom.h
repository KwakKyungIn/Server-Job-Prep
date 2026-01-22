// LobbyRoom.h
#pragma once
#include "RoomActor.h"
#include "JobQueue.h"
#include "Job.h"
#include "Protocol.pb.h"
#include "ClientPacketHandler.h"  // �κ� ����ȭ �������� �ʿ�
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
        _jobQueue->Push(ObjectPool<Job>::MakeShared([fn = std::move(fn)]() mutable { fn(); }));
    }

public:
    void Init(int32 channelId) { _channelId = channelId; }
    int32 GetChannelId() const { return _channelId; }

    // =========================================================
    // Step2 �ٽ� API
    // - EnterGame ����: Player ����/���� (Session�� PlayerRef ���� X)
    // - DB ���� ����: Lobby�� ������ ����
    // =========================================================
    void EnterGame(PlayerSessionRef ps, uint64 playerId, int32 channelId, int32 mapId, const Protocol::PositionInfo& spawn);
    void OnItemsLoaded(uint64 playerId, const Protocol::S2S_RES_ITEMS_LOAD& pkt);
    void OnStatLoaded(uint64 playerId, const Protocol::S2S_RES_LOAD_PLAYER_DATA& pkt);
    void TryEnterWorldIfReady(uint64 playerId); // Actor thread only
    // Transfer �ܰ�(�װ� Step4���� ���� �� ���ϰ�)
    bool      IsReady(uint64 playerId) const;      // Actor thread only
    PlayerRef DetachIfReady(uint64 playerId);      // Actor thread only

public:
    // Lobby�� "����/�̵�"�� �Ѵ�. AOI/����/������ ��� ���� ���� ����.
    void Adopt(PlayerRef player, bool isTransfer = false);
    PlayerRef Detach(uint64 playerId);
    PlayerRef Find(uint64 playerId) const;

    int32 GetCount() const { return static_cast<int32>(_players.size()); }

    void OnQuickSlotsLoaded(uint64 playerId, const Protocol::S2S_RES_QUICKSLOT_LOAD& pkt); // [NEW]

private:
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

    // Actor thread������ ����(�ܺδ� Push�θ� ȣ��)
    HashMap<uint64, Pending> _players;
};

