#include "pch.h"
#include "LobbyRoom.h"
#include "Player.h"
#include "PlayerSession.h"
#include "S2SPacketHandler.h"
#include "RoomManager.h"
#include "PersistenceService.h"
#include "GameSessionManager.h"
#include "GameMetrics.h"
#include "ExperimentUtils.h"

// 퀵슬롯 최대 개수 제한 (프로토콜 및 DB 스키마와 일치시켜야 함)
static constexpr int32 QS_MAX = 12; // 0~11

// [비동기 진입 진입점]
// 클라이언트가 최초 접속하거나 캐릭터를 선택했을 때 호출됨
// 여기서 바로 인게임으로 보내지 않고, 필요한 데이터를 DB에서 다 긁어올 때까지 Lobby에서 대기시킴
void LobbyRoom::EnterGame(PlayerSessionRef ps, uint64 playerId, int32 channelId, int32 mapId, const Protocol::PositionInfo& spawn, const std::string& playerName)
{
    if (!ps) return;
    if (playerId == 0) return;

    // 맵 이동(Transfer) 중일 때 중복 진입 막기 위한 방어 코드
    // 네트워크 지연으로 패킷이 두 번 오거나 따닥 눌렀을 때 터지는 거 방지
    if (ps->IsMapChanging())
    {
        printf(" [Lobby] EnterGame blocked - MapChanging in progress: %llu\n", playerId);
        return;
    }

    // 채널 ID 유효성 검증
    if (channelId != _channelId)
    {
        channelId = _channelId;
    }

    // 플레이어 슬롯 확보 (맵이나 해시맵 접근 시 항상 레퍼런스로 받아서 복사 비용 줄임)
    auto& slot = _players[playerId];

    // 이미 로딩 다 끝내고 대기 중인데 또 요청오면 무시함
    if (slot.player && slot.itemsLoaded && slot.statLoaded && slot.quickLoaded)
    {
        printf(" [Lobby] EnterGame blocked - Player already ready: %llu\n", playerId);
        return;
    }

    GameMetrics::OnLobbyEnterStart(playerId);


    // 이름 결정: Redis에서 못 가져왔으면 기존 이름이나 기본값으로 보정
    std::string finalName = playerName;
    if (finalName.empty() && slot.player)
        finalName = slot.player->GetName();
    if (finalName.empty())
        finalName = "Player_" + std::to_string(playerId);

    // 슬롯 상태에 따라 객체 생성 분기
    // 1. 아예 처음 들어온 경우 -> Player 객체 새로 생성
    // 2. 재접속이나 로딩 중 재요청 -> 기존 객체 재활용 (메모리 파편화 방지)
    if (!slot.player)
    {
        PlayerRef p = MakeShared<Player>();

        Protocol::PlayerInfo info;
        info.set_playerid(playerId);
        info.set_name(finalName);
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
        if (!finalName.empty())
            p->GetPlayerInfo()->set_name(finalName);

        p->GetPlayerInfo()->mutable_posinfo()->CopyFrom(spawn);
        p->SetSession(ps);
    }

    // 파티/채팅 등에서 사용할 이름 캐싱
    if (!finalName.empty() && GameSessionManager::GSessionManager)
        GameSessionManager::GSessionManager->SetPlayerName(playerId, finalName);

    // DB 로딩 상태 플래그 초기화 (Race Condition 방지의 핵심)
    // 이 플래그들이 전부 true가 되어야만 실제 월드로 진입 가능함
    slot.itemsLoaded = false;
    slot.statLoaded = false;
    slot.quickLoaded = false;

    // 일단 로비 룸 소속으로 등록해서 관리함 (스마트 포인터 레퍼런스 카운트 확보)
    Adopt(slot.player, false);  // isTransfer = false (로그인 진입)
}

// [게이트키퍼]
// DB 로딩이 하나 끝날 때마다 호출되어서, 모든 조건이 만족되었는지 체크함
// 전부 완료되었으면 그때 비로소 실제 게임 룸(World)으로 플레이어를 넘김
void LobbyRoom::TryEnterWorldIfReady(uint64 playerId)
{
    // 아직 로딩 덜 된 거 있으면 대기
    if (!IsReady(playerId))
        return;

    // 로비에서 플레이어 분리 (이제 월드로 갈 거니까)
    PlayerRef p = DetachIfReady(playerId);
    if (!p)
        return;

    PlayerSessionRef ps = p->GetSession();
    if (!ps)
    {
        // 세션 끊겼으면 다시 로비에 묶어둠 (낙동강 오리알 방지)
        Adopt(p);
        return;
    }

    // 룸 매니저가 없으면 진행 불가
    if (!GRoomManager)
    {
        Adopt(p);
        return;
    }

    const int32 ch = p->GetChannelId();
    const int32 mapId = p->GetMapId();
    const int64 instId = 0; // 필드는 인스턴스 ID 0번

    // 실제 게임 룸을 가져오거나 없으면 생성
    auto world = GRoomManager->GetOrCreateRoom(ch, mapId, instId);
    if (!world)
    {
        Adopt(p);
        return;
    }

    // [중요] 월드 진입이 확정된 시점이므로 세션의 Pending 상태를 해제함
    // 반드시 Actor 스레드 컨텍스트 안에서 실행되어야 동기화 문제가 없음
    ps->Post([](PlayerSessionRef self)
        {
            self->ClearPendingEnter_ActorOnly();
        });

    GameMetrics::OnLobbyEnterComplete(playerId);

    if (ExperimentUtils::ShouldRandomizeEnterSpawn())
    {
        Protocol::PositionInfo randomizedSpawn;
        if (auto* pos = p->GetPosInfo())
            randomizedSpawn.CopyFrom(*pos);

        if (ExperimentUtils::TryRandomizeSpawn(mapId, randomizedSpawn, world->GetMap().get()))
        {
            if (auto* pos = p->GetPosInfo())
            {
                pos->CopyFrom(randomizedSpawn);
                p->ResetMoveStamp_ActorOnly();
            }

            printf(" [Experiment] Random enter spawn: player=%llu map=%d pos=(%.1f, %.1f, %.1f)\n",
                playerId, mapId, randomizedSpawn.x(), randomizedSpawn.y(), randomizedSpawn.z());
        }
    }

    // 월드 룸의 JobQueue에 입장 작업을 밀어넣음 (스레드 전환)
    world->Push([world, ps, p]() mutable
        {
            world->Enter(ps, p);
        });
}

void LobbyRoom::OnDbLoadFailed(uint64 playerId, const char* reason)
{
    GameMetrics::OnLobbyEnterCancelled(playerId);

    PlayerSessionRef ps;

    auto it = _players.find(playerId);
    if (it != _players.end())
    {
        if (it->second.player)
        {
            ps = it->second.player->GetSession();
            it->second.player->SetSession(nullptr);
            it->second.player->SetRoom(nullptr);
        }

        _players.erase(it);
    }

    if (!ps && GameSessionManager::GSessionManager)
        ps = GameSessionManager::GSessionManager->FindByPlayerId(playerId);

    if (!ps)
        return;

    if (ps->HasPendingEnter_AnyThread() == false)
        return;

    const char* tag = (reason ? reason : "Unknown");
    printf(" [Lobby] DB load failed (%s): %llu\n", tag, playerId);

    ps->Post([playerId](PlayerSessionRef self)
        {
            Protocol::S_ENTER_GAME failPkt;
            failPkt.set_success(false);
            self->Send(ClientPacketHandler::MakeSendBuffer(failPkt));

            self->ClearPendingEnter_ActorOnly();
            if (GameSessionManager::GSessionManager)
                GameSessionManager::GSessionManager->UnbindPlayerId(playerId);
            self->ClearPlayerId_ActorOnly();
            self->Disconnect(L"DB Load Failed");
        });
}


// [DB 콜백 1] 아이템 정보 로드 완료
void LobbyRoom::OnItemsLoaded(uint64 playerId, const Protocol::S2S_RES_ITEMS_LOAD& pkt)
{
    if (!pkt.success())
    {
        OnDbLoadFailed(playerId, "Items");
        return;
    }

    auto it = _players.find(playerId);
    if (it == _players.end()) return;
    if (!it->second.player) return;

    PlayerRef p = it->second.player;

    // 1. 메모리에 아이템 정보 반영
    p->SetItems(pkt.items());

    // 1.1 장비 슬롯 정책 정규화 (Sanitization)
    // DB에 이상한 데이터가 있거나, 버그로 무기 2개를 동시에 끼고 있는 경우 등을
    // 서버 로딩 단계에서 강제로 바로잡음. (무기/갑옷/투구 슬롯 구분)
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
            // 중복 장착 감지되면 강제로 장착 해제 및 DB/Redis 동기화
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

    // 2. 장착 아이템에 따른 스탯 재계산 (공격력, 방어력 등)
    p->RefreshStats();

    // 3. Redis Priming (캐싱 전략)
    // DB에서 막 가져온 따끈따끈한 데이터를 Redis에 밀어넣어서
    // 이후 발생하는 조회 요청이 DB까지 안 가고 Redis에서 해결되도록 함
    google::protobuf::RepeatedPtrField<Protocol::ItemInfo> primeItems;
    for (const auto& item : itemsVec)
        primeItems.Add()->CopyFrom(item);

    Persistence::PersistenceService::I().PrimeFromDb_Inventory(playerId, primeItems);

    // 4. 클라이언트에게 인벤토리 정보 전송 (동기화)
    if (auto ps = p->GetSession())
    {
        Protocol::S_ITEM_LIST clientPkt;
        for (const auto& item : itemsVec)
            clientPkt.add_items()->CopyFrom(item);

        ps->Send(ClientPacketHandler::MakeSendBuffer(clientPkt));
    }

    // 아이템 로딩 완료 마킹하고 다음 단계 체크
    it->second.itemsLoaded = true;
    TryEnterWorldIfReady(playerId);
}

// [DB 콜백 2] 플레이어 스탯 정보 로드 완료
void LobbyRoom::OnStatLoaded(uint64 playerId, const Protocol::S2S_RES_LOAD_PLAYER_DATA& pkt)
{
    if (!pkt.success())
    {
        OnDbLoadFailed(playerId, "Stat");
        return;
    }

    auto it = _players.find(playerId);
    if (it == _players.end()) return;
    if (!it->second.player) return;

    PlayerRef p = it->second.player;

    if (auto st = p->GetStatInfo())
        st->CopyFrom(pkt.statinfo());
    p->SetGold(pkt.gold());

    // 기본 스탯 + 아이템 스탯 합산
    p->RefreshStats();

    // Redis 캐시에 핵심 정보(레벨, HP, 경험치) 업데이트
    if (auto st = p->GetStatInfo())
        Persistence::PersistenceService::I().PrimeFromDb_PlayerCore(playerId, *st, p->GetGold());

    it->second.statLoaded = true;
    TryEnterWorldIfReady(playerId);
}

// 모든 데이터 로딩이 끝났는지 확인하는 헬퍼 함수
bool LobbyRoom::IsReady(uint64 playerId) const
{
    auto it = _players.find(playerId);
    if (it == _players.end()) return false;
    // 플레이어 객체가 있고 + 아이템/스탯/퀵슬롯 3박자가 모두 로드되어야 함
    return (it->second.player != nullptr) && it->second.itemsLoaded && it->second.statLoaded && it->second.quickLoaded;

}

PlayerRef LobbyRoom::DetachIfReady(uint64 playerId)
{
    if (!IsReady(playerId))
        return nullptr;
    return Detach(playerId);
}

// =========================================================
// 플레이어 관리 API (Adopt / Detach)
// =========================================================

void LobbyRoom::Adopt(PlayerRef player, bool isTransfer)
{
    if (!player) return;

    const uint64 pid = player->GetPlayerId();
    if (pid == 0) return;

    // 맵 이동(Transfer)으로 온 경우
    // 데이터를 DB에서 다시 읽지 않고 기존 메모리 상태를 유지할 수도 있음 (정책에 따라 다름)
    if (isTransfer)
    {
        auto it = _players.find(pid);
        if (it != _players.end())
        {
            // 이미 슬롯이 있으면 덮어쓰기 (재진입 방지)
            it->second.player = player;
            it->second.itemsLoaded = true;
            it->second.statLoaded = true;
            it->second.quickLoaded = true;
        }
        else
        {
            // Transfer인데 슬롯이 없으면 새로 파되, 이미 데이터는 있다고 가정하고 Ready 상태로 둠
            Pending& slot = _players[pid];
            slot.player = player;
            slot.itemsLoaded = true;
            slot.statLoaded = true;
            slot.quickLoaded = true;
        }
    }
    else
    {
        // 일반 로그인: Pending 슬롯 만들고 DB 로딩 대기 (flags = false)
        auto& slot = _players[pid];
        slot.player = player;
        // itemsLoaded, statLoaded는 false 상태로 시작
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

    return p;
}

PlayerRef LobbyRoom::Find(uint64 playerId) const
{
    auto it = _players.find(playerId);
    if (it == _players.end())
        return nullptr;
    return it->second.player;
}

// [DB 콜백 3] 퀵슬롯 정보 로드 완료
void LobbyRoom::OnQuickSlotsLoaded(uint64 playerId, const Protocol::S2S_RES_QUICKSLOT_LOAD& pkt)
{
    if (!pkt.success())
    {
        OnDbLoadFailed(playerId, "QuickSlot");
        return;
    }

    auto it = _players.find(playerId);
    if (it == _players.end()) return;
    if (!it->second.player) return;

    PlayerRef p = it->second.player;

    // Redis 캐싱 (Priming)
    Persistence::PersistenceService::I().PrimeFromDb_QuickSlot(playerId, pkt.slots());

    // 클라이언트 동기화 (Snapshot 전송)
    if (auto ps = p->GetSession())
    {
        Protocol::S_QUICKSLOT_LIST out;

        // DB 해킹이나 오류로 이상한 인덱스가 들어올 수 있으니 필터링 (Safety Check)
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
