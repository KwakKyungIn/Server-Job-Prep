#include "pch.h"
#include "LobbyRoom.h"
#include "Player.h"
#include "PlayerSession.h"
#include "S2SPacketHandler.h"
#include "RoomManager.h"
#include "PersistenceService.h"

static constexpr int32 QS_MAX = 12; // 0~11

void LobbyRoom::EnterGame(PlayerSessionRef ps, uint64 playerId, int32 channelId, int32 mapId, const Protocol::PositionInfo& spawn)
{
    if (!ps) return;
    if (playerId == 0) return;

    // ✅ Transfer 중이면 거부
    if (ps->IsMapChanging())
    {
        printf("⚠️ [Lobby] EnterGame blocked - MapChanging in progress: %llu\n", playerId);
        return;
    }

    // channelId 검증
    if (channelId != _channelId)
    {
        channelId = _channelId;
    }

    auto& slot = _players[playerId];

    // ✅ Transfer 슬롯이 이미 있고 Ready 상태면 재생성 금지
    if (slot.player && slot.itemsLoaded && slot.statLoaded && slot.quickLoaded)
    {
        printf("⚠️ [Lobby] EnterGame blocked - Player already ready: %llu\n", playerId);
        return;
    }


    // 새로 만들거나, 있으면 갱신(재접속/중복 EnterGame)
    if (!slot.player)
    {
        PlayerRef p = MakeShared<Player>();

        Protocol::PlayerInfo info;
        info.set_playerid(playerId);
        info.set_name("Player_" + std::to_string(playerId));
        info.mutable_posinfo()->CopyFrom(spawn);

        p->Init(info);
        p->SetChannelId(channelId);
        p->SetMapId(mapId);
        p->SetInstanceId(0);
        p->SetSession(ps);

        slot.player = p;
    }
    else
    {
        PlayerRef p = slot.player;
        p->SetChannelId(channelId);
        p->SetMapId(mapId);
        p->SetInstanceId(0);
        p->GetPlayerInfo()->mutable_posinfo()->CopyFrom(spawn);
        p->SetSession(ps);
    }

    // 이번 EnterGame 사이클에서 DB 응답을 다시 받을 거니까 flag reset
    slot.itemsLoaded = false;
    slot.statLoaded = false;
    slot.quickLoaded = false;

    // Lobby가 소유 + "어떤 Room에도 속함" 보장
    Adopt(slot.player, false);  // ✅ isTransfer = false
}

void LobbyRoom::TryEnterWorldIfReady(uint64 playerId)
{
    if (!IsReady(playerId))
        return;

    PlayerRef p = DetachIfReady(playerId);
    if (!p)
        return;

    PlayerSessionRef ps = p->GetSession();
    if (!ps)
    {
        Adopt(p);
        return;
    }

    if (!GRoomManager)
    {
        Adopt(p);
        return;
    }

    const int32 ch = p->GetChannelId();
    const int32 mapId = p->GetMapId();
    const int64 instId = 0;

    auto world = GRoomManager->GetOrCreateRoom(ch, mapId, instId);
    if (!world)
    {
        Adopt(p);
        return;
    }

    // ✅ [A 마무리] 월드 진입 “확정” 시점에서 pending enter 제거 (Actor thread에서만!)
    ps->Post([](PlayerSessionRef self)
        {
            self->ClearPendingEnter_ActorOnly();
        });

    world->Push([world, ps, p]() mutable
        {
            world->Enter(ps, p);
        });
}


void LobbyRoom::OnItemsLoaded(uint64 playerId, const Protocol::S2S_RES_ITEMS_LOAD& pkt)
{
    auto it = _players.find(playerId);
    if (it == _players.end()) return;
    if (!it->second.player) return;

    PlayerRef p = it->second.player;

    // 1) 인벤 반영
    p->SetItems(pkt.items());

    // 1.1) ✅ 장비 3슬롯 정책 정규화(로그인 시 중복 장착 방지)
    // - 1000~1999 : Weapon
    // - 2000~2999 : Body
    // - 4000~4999 : Head
    auto getEquipSlot = [](int32 templateId) -> int32
        {
            if (templateId >= 1000 && templateId < 2000) return 1; // Weapon
            if (templateId >= 2000 && templateId < 3000) return 2; // Body
            if (templateId >= 4000 && templateId < 5000) return 3; // Head
            return 0; // None
        };

    bool usedWeapon = false;
    bool usedBody = false;
    bool usedHead = false;

    auto& itemsVec = p->GetItems();
    for (auto& item : itemsVec)
    {
        if (!item.isequipped())
            continue;

        const int32 slot = getEquipSlot(item.templateid());
        if (slot == 0)
            continue;

        bool* used = nullptr;
        if (slot == 1) used = &usedWeapon;
        else if (slot == 2) used = &usedBody;
        else if (slot == 3) used = &usedHead;

        if (used && *used)
        {
            // 중복 장착 -> 자동 해제
            item.set_isequipped(false);

            Persistence::PersistenceService::I().UpdateInventoryItem(
                playerId,
                item.itemuid(),
                item.templateid(),
                item.slot(),
                item.count(),
                item.isequipped(),
                true
            );
        }
        else if (used)
        {
            *used = true;
        }
    }

    // 2) 장착 보너스 포함 스탯 재계산
    p->RefreshStats();

    // 3) Prime(DB -> Redis) : 정규화된 아이템으로 넣는다.
    google::protobuf::RepeatedPtrField<Protocol::ItemInfo> primeItems;
    for (const auto& item : itemsVec)
        primeItems.Add()->CopyFrom(item);

    Persistence::PersistenceService::I().PrimeFromDb_Inventory(playerId, primeItems);

    // 4) 클라 동기화(인벤)
    if (auto ps = p->GetSession())
    {
        Protocol::S_ITEM_LIST clientPkt;
        for (const auto& item : itemsVec)
            clientPkt.add_items()->CopyFrom(item);

        ps->Send(ClientPacketHandler::MakeSendBuffer(clientPkt));
    }

    it->second.itemsLoaded = true;
    TryEnterWorldIfReady(playerId);
}

void LobbyRoom::OnStatLoaded(uint64 playerId, const Protocol::S2S_RES_LOAD_PLAYER_DATA& pkt)
{
    auto it = _players.find(playerId);
    if (it == _players.end()) return;
    if (!it->second.player) return;

    PlayerRef p = it->second.player;

    if (auto st = p->GetStatInfo())
        st->CopyFrom(pkt.statinfo());

    p->RefreshStats();

    // Prime은 core에 level/hp/totalExp만 박는다.
    if (auto st = p->GetStatInfo())
        Persistence::PersistenceService::I().PrimeFromDb_PlayerCore(playerId, *st);

    it->second.statLoaded = true;
    TryEnterWorldIfReady(playerId);
}

bool LobbyRoom::IsReady(uint64 playerId) const
{
    auto it = _players.find(playerId);
    if (it == _players.end()) return false;
    return (it->second.player != nullptr) && it->second.itemsLoaded && it->second.statLoaded && it->second.quickLoaded;

}

PlayerRef LobbyRoom::DetachIfReady(uint64 playerId)
{
    if (!IsReady(playerId))
        return nullptr;
    return Detach(playerId);
}

// =========================================================
// 기존 API들 (Pending 구조로 맞춰서 수정)
// =========================================================

void LobbyRoom::Adopt(PlayerRef player, bool isTransfer)
{
    if (!player) return;

    const uint64 pid = player->GetPlayerId();
    if (pid == 0) return;

    // ✅ Transfer 중에는 find로 먼저 체크 (operator[] 함정 회피)
    if (isTransfer)
    {
        auto it = _players.find(pid);
        if (it != _players.end())
        {
            // 이미 있으면 덮어쓰기 (재진입 방지)
            it->second.player = player;
            it->second.itemsLoaded = true;
            it->second.statLoaded = true;
            it->second.quickLoaded = true;
        }
        else
        {
            // Transfer인데 slot 없으면 새로 만들되, 즉시 Ready 상태
            Pending& slot = _players[pid];
            slot.player = player;
            slot.itemsLoaded = true;
            slot.statLoaded = true;
            slot.quickLoaded = true;
        }
    }
    else
    {
        // 로그인: 기존 로직 (DB 로딩 대기)
        auto& slot = _players[pid];
        slot.player = player;
        // itemsLoaded, statLoaded는 false 유지
    }

    player->SetRoom(shared_from_this());
}

PlayerRef LobbyRoom::Detach(uint64 playerId)
{
    auto it = _players.find(playerId);
    if (it == _players.end())
        return nullptr;

    PlayerRef p = it->second.player;
    _players.erase(it);

    // Detach 시점에 SetRoom(nullptr) 할지 여부는 너가 Transfer 파이프라인에서 결정.
    // (다음 Room이 즉시 Adopt한다면 굳이 nullptr 안 둬도 됨)

    return p;
}

PlayerRef LobbyRoom::Find(uint64 playerId) const
{
    auto it = _players.find(playerId);
    if (it == _players.end())
        return nullptr;
    return it->second.player;
}

void LobbyRoom::OnQuickSlotsLoaded(uint64 playerId, const Protocol::S2S_RES_QUICKSLOT_LOAD& pkt)
{
    auto it = _players.find(playerId);
    if (it == _players.end()) return;
    if (!it->second.player) return;
    if (!pkt.success()) return;

    PlayerRef p = it->second.player;

    // Redis Prime
    Persistence::PersistenceService::I().PrimeFromDb_QuickSlot(playerId, pkt.slots());

    // ✅ Client sync (snapshot)
    if (auto ps = p->GetSession())
    {
        Protocol::S_QUICKSLOT_LIST out;

        // 안전: DB에 이상치가 있어도 클라엔 0~11만 보냄
        for (const auto& s : pkt.slots())
        {
            if (s.slotindex() < 0 || s.slotindex() >= QS_MAX)
                continue;

            auto* add = out.add_slots();
            add->CopyFrom(s);
        }

        ps->Send(ClientPacketHandler::MakeSendBuffer(out));
    }

    it->second.quickLoaded = true;
    TryEnterWorldIfReady(playerId);
}

