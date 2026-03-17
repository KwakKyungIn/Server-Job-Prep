#include "pch.h"
#include "GameMetrics.h"

#include "ClientPacketHandler.h"
#include "PacketMetricsHooks.h"
#include "Protocol_S2S.pb.h"
#include "S2SPacketHandler.h"

#include "Metrics.h"
#include "MetricsSystem.h"

#include <chrono>
#include <cstring>
#include <mutex>
#include <unordered_map>

namespace
{
    struct GameMetricsRegistry
    {
        std::shared_ptr<Counter> packetIngressCounter;
        std::shared_ptr<Histogram> packetHandleHistogram;
        std::shared_ptr<Counter> packetFailureCounter;
        std::shared_ptr<Histogram> lobbyWaitHistogram;
        std::shared_ptr<Histogram> s2sRttHistogram;
        std::shared_ptr<Histogram> broadcastRecipientsHistogram;
        std::shared_ptr<Histogram> aoiUpdateHistogram;
        std::shared_ptr<Counter> aoiUpdateCounter;
        std::shared_ptr<Histogram> aoiCandidatesHistogram;
        std::shared_ptr<Histogram> aoiVisibleHistogram;
        std::shared_ptr<Histogram> autocommitTickHistogram;
        std::shared_ptr<Gauge> autocommitTargetsGauge;
        std::shared_ptr<Gauge> autocommitInflightGauge;
        std::shared_ptr<Counter> autocommitSentCounter;
        std::shared_ptr<Counter> autocommitSkipCounter;
        std::shared_ptr<Gauge> dirtySetSizeGauge;
        std::shared_ptr<Counter> persistenceMutationCounter;
        std::shared_ptr<Gauge> ccuGauge;
        std::shared_ptr<Gauge> ingameGauge;
    };

    const std::vector<double>& CountHistogramBuckets()
    {
        static const std::vector<double> kBuckets = {
            0.0,
            1.0,
            2.0,
            4.0,
            8.0,
            16.0,
            32.0,
            64.0,
            128.0,
            256.0,
            512.0,
            1024.0,
        };

        return kBuckets;
    }

    const std::vector<double>& AoiUpdateSecondsBuckets()
    {
        static const std::vector<double> kBuckets = {
            MetricsMillisecondsToSeconds(0.001),
            MetricsMillisecondsToSeconds(0.005),
            MetricsMillisecondsToSeconds(0.01),
            MetricsMillisecondsToSeconds(0.05),
            MetricsMillisecondsToSeconds(0.1),
            MetricsMillisecondsToSeconds(0.5),
            MetricsMillisecondsToSeconds(1.0),
            MetricsMillisecondsToSeconds(5.0),
            MetricsMillisecondsToSeconds(10.0),
            MetricsMillisecondsToSeconds(50.0),
            MetricsMillisecondsToSeconds(100.0),
        };

        return kBuckets;
    }

    GameMetricsRegistry& GetGameMetricsRegistry()
    {
        static GameMetricsRegistry metrics{
            MetricsSystem::Instance().RegisterCounter(
                "packet_ingress_total",
                "Total client packets dispatched by packet op.",
                { "op" }),
            MetricsSystem::Instance().RegisterHistogram(
                "packet_handle_seconds",
                "Handle_C_* execution time in seconds.",
                MetricsHistogramBuckets::PacketHandleSeconds(),
                { "op" }),
            MetricsSystem::Instance().RegisterCounter(
                "packet_failure_total",
                "Total client packet failures by reason.",
                { "op", "reason" }),
            MetricsSystem::Instance().RegisterHistogram(
                "lobby_wait_seconds",
                "Time from LobbyRoom::EnterGame to world entry ready in seconds.",
                MetricsHistogramBuckets::JobQueueWaitSeconds(),
                { "type" }),
            MetricsSystem::Instance().RegisterHistogram(
                "s2s_rtt_seconds",
                "S2S request-to-response RTT in seconds.",
                MetricsHistogramBuckets::JobQueueWaitSeconds(),
                { "op" }),
            MetricsSystem::Instance().RegisterHistogram(
                "broadcast_recipients",
                "Recipients reached by Hot Room broadcast fan-out.",
                CountHistogramBuckets(),
                { "kind", "mode" }),
            MetricsSystem::Instance().RegisterHistogram(
                "aoi_update_seconds",
                "AOI update execution time in seconds.",
                AoiUpdateSecondsBuckets()),
            MetricsSystem::Instance().RegisterCounter(
                "aoi_update_total",
                "Total AOI update executions."),
            MetricsSystem::Instance().RegisterHistogram(
                "aoi_candidates",
                "AOI broad-phase candidate count per update.",
                CountHistogramBuckets(),
                { "kind" }),
            MetricsSystem::Instance().RegisterHistogram(
                "aoi_visible",
                "AOI visible object count per update.",
                CountHistogramBuckets(),
                { "kind" }),
            MetricsSystem::Instance().RegisterHistogram(
                "autocommit_tick_seconds",
                "AutoCommit tick execution time in seconds.",
                MetricsHistogramBuckets::JobQueueWaitSeconds()),
            MetricsSystem::Instance().RegisterGauge(
                "autocommit_targets",
                "Number of persistence targets collected in the latest AutoCommit tick."),
            MetricsSystem::Instance().RegisterGauge(
                "autocommit_inflight",
                "Number of player ids currently waiting for AutoCommit save responses."),
            MetricsSystem::Instance().RegisterCounter(
                "autocommit_sent_total",
                "Total AutoCommit save requests sent by domain.",
                { "domain" }),
            MetricsSystem::Instance().RegisterCounter(
                "autocommit_skip_total",
                "Total AutoCommit skips by reason.",
                { "reason" }),
            MetricsSystem::Instance().RegisterGauge(
                "dirty_set_size",
                "Current dirty set size by persistence domain.",
                { "domain" }),
            MetricsSystem::Instance().RegisterCounter(
                "persistence_mutation_total",
                "Total persistence mutations by domain and path.",
                { "domain", "path" }),
            MetricsSystem::Instance().RegisterGauge(
                "ccu",
                "Current connected session count."),
            MetricsSystem::Instance().RegisterGauge(
                "ingame_players",
                "Current bound in-game player count."),
        };

        return metrics;
    }

    std::uint64_t NowSteadyMicroseconds()
    {
        return static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count());
    }

    const char* ClientPacketOpLabel(std::uint16_t packetId)
    {
        switch (packetId)
        {
        case ClientPacketHandler::PKT_C_LOGIN: return "login";
        case ClientPacketHandler::PKT_C_ENTER_GAME: return "enter_game";
        case ClientPacketHandler::PKT_C_MOVE: return "move";
        case ClientPacketHandler::PKT_C_SKILL: return "skill";
        case ClientPacketHandler::PKT_C_RESPAWN_REQ: return "respawn_req";
        case ClientPacketHandler::PKT_C_USE_ITEM: return "use_item";
        case ClientPacketHandler::PKT_C_EQUIP_ITEM: return "equip_item";
        case ClientPacketHandler::PKT_C_MAP_CHANGE_REQ: return "map_change_req";
        case ClientPacketHandler::PKT_C_MAP_CHANGE_ACK: return "map_change_ack";
        case ClientPacketHandler::PKT_C_CHANNEL_CHANGE_REQ: return "channel_change_req";
        case ClientPacketHandler::PKT_C_CHAT_REQ: return "chat_req";
        case ClientPacketHandler::PKT_C_HEART_BEAT_REQ: return "heart_beat_req";
        case ClientPacketHandler::PKT_C_PARTY_CHAT_REQ: return "party_chat_req";
        case ClientPacketHandler::PKT_C_PARTY_CREATE_REQ: return "party_create_req";
        case ClientPacketHandler::PKT_C_PARTY_INVITE_REQ: return "party_invite_req";
        case ClientPacketHandler::PKT_C_PARTY_INVITE_ACCEPT_REQ: return "party_invite_accept_req";
        case ClientPacketHandler::PKT_C_PARTY_LEAVE_REQ: return "party_leave_req";
        case ClientPacketHandler::PKT_C_PARTY_KICK_REQ: return "party_kick_req";
        case ClientPacketHandler::PKT_C_PARTY_DISBAND_REQ: return "party_disband_req";
        case ClientPacketHandler::PKT_C_PARTY_STATUS_REQ: return "party_status_req";
        case ClientPacketHandler::PKT_C_DUNGEON_ENTER_REQ: return "dungeon_enter_req";
        case ClientPacketHandler::PKT_C_DUNGEON_EXIT_REQ: return "dungeon_exit_req";
        case ClientPacketHandler::PKT_C_SET_QUICKSLOT: return "set_quickslot";
        case ClientPacketHandler::PKT_C_TRADE_REQ: return "trade_req";
        case ClientPacketHandler::PKT_C_TRADE_INVITE_RESP: return "trade_invite_resp";
        case ClientPacketHandler::PKT_C_TRADE_OFFER_SET: return "trade_offer_set";
        case ClientPacketHandler::PKT_C_TRADE_GOLD_SET: return "trade_gold_set";
        case ClientPacketHandler::PKT_C_TRADE_READY: return "trade_ready";
        case ClientPacketHandler::PKT_C_TRADE_CONFIRM: return "trade_confirm";
        case ClientPacketHandler::PKT_C_TRADE_CANCEL: return "trade_cancel";
        case ClientPacketHandler::PKT_C_INV_DRAG_DROP: return "inv_drag_drop";
        default:
            return "other";
        }
    }

    const char* ClientFailureReasonLabel(GameMetrics::ClientPacketFailureReason reason)
    {
        switch (reason)
        {
        case GameMetrics::ClientPacketFailureReason::Parse:
            return "parse";
        case GameMetrics::ClientPacketFailureReason::Validate:
            return "validate";
        case GameMetrics::ClientPacketFailureReason::Handler:
            return "handler";
        default:
            return "other";
        }
    }

    const char* BroadcastKindLabel(GameMetrics::HotRoomBroadcastKind kind)
    {
        switch (kind)
        {
        case GameMetrics::HotRoomBroadcastKind::Move:
            return "move";
        case GameMetrics::HotRoomBroadcastKind::Skill:
            return "skill";
        case GameMetrics::HotRoomBroadcastKind::Hp:
            return "hp";
        case GameMetrics::HotRoomBroadcastKind::Spawn:
            return "spawn";
        case GameMetrics::HotRoomBroadcastKind::Despawn:
            return "despawn";
        default:
            return "other";
        }
    }

    const char* BroadcastModeLabel(GameMetrics::HotRoomBroadcastMode mode)
    {
        switch (mode)
        {
        case GameMetrics::HotRoomBroadcastMode::Aoi:
            return "aoi";
        case GameMetrics::HotRoomBroadcastMode::Room:
            return "room";
        default:
            return "other";
        }
    }

    const char* AoiObjectKindLabel(GameMetrics::AoiObjectKind kind)
    {
        switch (kind)
        {
        case GameMetrics::AoiObjectKind::Player:
            return "player";
        case GameMetrics::AoiObjectKind::Monster:
            return "monster";
        case GameMetrics::AoiObjectKind::Projectile:
            return "projectile";
        default:
            return "other";
        }
    }

    const char* AutoCommitDomainLabel(GameMetrics::AutoCommitDomain domain)
    {
        switch (domain)
        {
        case GameMetrics::AutoCommitDomain::Core:
            return "core";
        case GameMetrics::AutoCommitDomain::Inventory:
            return "inv";
        case GameMetrics::AutoCommitDomain::QuickSlot:
            return "qs";
        default:
            return "other";
        }
    }

    const char* AutoCommitSkipReasonLabel(GameMetrics::AutoCommitSkipReason reason)
    {
        switch (reason)
        {
        case GameMetrics::AutoCommitSkipReason::Inflight:
            return "inflight";
        case GameMetrics::AutoCommitSkipReason::EmptySnapshot:
            return "empty_snapshot";
        case GameMetrics::AutoCommitSkipReason::RedisMissing:
            return "redis_missing";
        default:
            return "other";
        }
    }

    const char* DirtySetDomainLabel(GameMetrics::DirtySetDomain domain)
    {
        switch (domain)
        {
        case GameMetrics::DirtySetDomain::Player:
            return "player";
        case GameMetrics::DirtySetDomain::Inventory:
            return "inv";
        case GameMetrics::DirtySetDomain::QuickSlot:
            return "qs";
        default:
            return "other";
        }
    }

    const char* PersistenceMutationDomainLabel(GameMetrics::PersistenceMutationDomain domain)
    {
        switch (domain)
        {
        case GameMetrics::PersistenceMutationDomain::QuickSlot:
            return "qs";
        default:
            return "other";
        }
    }

    const char* PersistenceMutationPathLabel(GameMetrics::PersistenceMutationPath path)
    {
        switch (path)
        {
        case GameMetrics::PersistenceMutationPath::Writeback:
            return "writeback";
        case GameMetrics::PersistenceMutationPath::Immediate:
            return "immediate";
        default:
            return "other";
        }
    }

    enum class S2SOpCode : std::uint8_t
    {
        Other = 0,
        LoadPlayerData,
        ItemsLoad,
        SavePlayerCore,
        SaveInventory,
        ItemCreate,
        QuickslotLoad,
        SaveQuickslot,
        TradeCommit,
    };

    const char* S2SOpLabel(S2SOpCode op)
    {
        switch (op)
        {
        case S2SOpCode::LoadPlayerData:
            return "load_player_data";
        case S2SOpCode::ItemsLoad:
            return "items_load";
        case S2SOpCode::SavePlayerCore:
            return "save_player_core";
        case S2SOpCode::SaveInventory:
            return "save_inventory";
        case S2SOpCode::ItemCreate:
            return "item_create";
        case S2SOpCode::QuickslotLoad:
            return "quickslot_load";
        case S2SOpCode::SaveQuickslot:
            return "save_quickslot";
        case S2SOpCode::TradeCommit:
            return "trade_commit";
        default:
            return "other";
        }
    }

    S2SOpCode S2SOpFromRequestPacket(std::uint16_t packetId)
    {
        switch (packetId)
        {
        case S2SPacketHandler::PKT_S2S_REQ_LOAD_PLAYER_DATA:
            return S2SOpCode::LoadPlayerData;
        case S2SPacketHandler::PKT_S2S_REQ_ITEMS_LOAD:
            return S2SOpCode::ItemsLoad;
        case S2SPacketHandler::PKT_S2S_REQ_SAVE_PLAYER_CORE:
            return S2SOpCode::SavePlayerCore;
        case S2SPacketHandler::PKT_S2S_REQ_SAVE_INVENTORY:
            return S2SOpCode::SaveInventory;
        case S2SPacketHandler::PKT_S2S_REQ_ITEM_CREATE:
            return S2SOpCode::ItemCreate;
        case S2SPacketHandler::PKT_S2S_REQ_QUICKSLOT_LOAD:
            return S2SOpCode::QuickslotLoad;
        case S2SPacketHandler::PKT_S2S_REQ_SAVE_QUICKSLOT:
            return S2SOpCode::SaveQuickslot;
        case S2SPacketHandler::PKT_S2S_REQ_TRADE_COMMIT:
            return S2SOpCode::TradeCommit;
        default:
            return S2SOpCode::Other;
        }
    }

    S2SOpCode S2SOpFromResponsePacket(std::uint16_t packetId)
    {
        switch (packetId)
        {
        case S2SPacketHandler::PKT_S2S_RES_LOAD_PLAYER_DATA:
            return S2SOpCode::LoadPlayerData;
        case S2SPacketHandler::PKT_S2S_RES_ITEMS_LOAD:
            return S2SOpCode::ItemsLoad;
        case S2SPacketHandler::PKT_S2S_RES_SAVE_PLAYER_CORE:
            return S2SOpCode::SavePlayerCore;
        case S2SPacketHandler::PKT_S2S_RES_SAVE_INVENTORY:
            return S2SOpCode::SaveInventory;
        case S2SPacketHandler::PKT_S2S_RES_ITEM_CREATE:
            return S2SOpCode::ItemCreate;
        case S2SPacketHandler::PKT_S2S_RES_QUICKSLOT_LOAD:
            return S2SOpCode::QuickslotLoad;
        case S2SPacketHandler::PKT_S2S_RES_SAVE_QUICKSLOT:
            return S2SOpCode::SaveQuickslot;
        case S2SPacketHandler::PKT_S2S_RES_TRADE_COMMIT:
            return S2SOpCode::TradeCommit;
        default:
            return S2SOpCode::Other;
        }
    }

    struct InflightKey
    {
        S2SOpCode op = S2SOpCode::Other;
        std::uint64_t correlationKey = 0;

        bool operator==(const InflightKey& rhs) const
        {
            return op == rhs.op && correlationKey == rhs.correlationKey;
        }
    };

    struct InflightKeyHasher
    {
        size_t operator()(const InflightKey& key) const
        {
            const size_t opPart = static_cast<size_t>(static_cast<std::uint8_t>(key.op));
            const size_t keyPart = static_cast<size_t>(key.correlationKey);
            return (keyPart * 1315423911u) ^ opPart;
        }
    };

    std::mutex GLobbyLock;
    std::unordered_map<std::uint64_t, std::uint64_t> GLobbyEnterStartUs;

    std::mutex GS2SInflightLock;
    std::unordered_map<InflightKey, std::uint64_t, InflightKeyHasher> GS2SInflight;

    constexpr std::uint64_t kS2STtlUs = 30ull * 1000ull * 1000ull;
    constexpr std::uint64_t kS2SCleanupIntervalUs = 1ull * 1000ull * 1000ull;
    constexpr std::uint64_t kS2SOverflowWarnIntervalUs = 5ull * 1000ull * 1000ull;
    constexpr size_t kS2SMaxInflight = 10000;

    std::uint64_t GS2SNextCleanupUs = 0;
    std::uint64_t GS2SLastOverflowWarnUs = 0;

    void CleanupS2SInflightLocked(std::uint64_t nowUs)
    {
        if (nowUs < GS2SNextCleanupUs)
            return;

        for (auto it = GS2SInflight.begin(); it != GS2SInflight.end();)
        {
            if (nowUs >= it->second && (nowUs - it->second) > kS2STtlUs)
                it = GS2SInflight.erase(it);
            else
                ++it;
        }

        GS2SNextCleanupUs = nowUs + kS2SCleanupIntervalUs;
    }

    void TrackS2SRequestInternal(S2SOpCode op, std::uint64_t correlationKey)
    {
        if (op == S2SOpCode::Other || correlationKey == 0)
            return;

        const std::uint64_t nowUs = NowSteadyMicroseconds();

        std::lock_guard<std::mutex> guard(GS2SInflightLock);
        CleanupS2SInflightLocked(nowUs);

        if (GS2SInflight.size() >= kS2SMaxInflight)
        {
            if (nowUs >= GS2SLastOverflowWarnUs &&
                (nowUs - GS2SLastOverflowWarnUs) >= kS2SOverflowWarnIntervalUs)
            {
                GS2SLastOverflowWarnUs = nowUs;
                std::cout << "[Metrics][WARN] S2S inflight map limit reached(" << kS2SMaxInflight
                    << "). New entries are dropped." << std::endl;
            }
            return;
        }

        GS2SInflight[{ op, correlationKey }] = nowUs;
    }

    void TrackS2SResponseInternal(S2SOpCode op, std::uint64_t correlationKey)
    {
        if (op == S2SOpCode::Other || correlationKey == 0)
            return;

        const std::uint64_t nowUs = NowSteadyMicroseconds();

        std::uint64_t startUs = 0;
        {
            std::lock_guard<std::mutex> guard(GS2SInflightLock);
            CleanupS2SInflightLocked(nowUs);

            const InflightKey key{ op, correlationKey };
            auto findIt = GS2SInflight.find(key);
            if (findIt == GS2SInflight.end())
                return;

            startUs = findIt->second;
            GS2SInflight.erase(findIt);
        }

        if (nowUs < startUs)
            return;

        const double elapsedSeconds = static_cast<double>(nowUs - startUs) / 1000000.0;
        GetGameMetricsRegistry().s2sRttHistogram->Observe(elapsedSeconds, { { "op", S2SOpLabel(op) } });
    }

    bool IsHandlerName(const char* handlerName, const char* expectedName)
    {
        if (handlerName == nullptr || expectedName == nullptr)
            return false;

        return std::strcmp(handlerName, expectedName) == 0;
    }

    GameMetrics::ClientPacketFailureReason ToClientFailureReason(PacketMetricsHooks::FailureReason reason)
    {
        switch (reason)
        {
        case PacketMetricsHooks::FailureReason::Parse:
            return GameMetrics::ClientPacketFailureReason::Parse;
        case PacketMetricsHooks::FailureReason::Validate:
            return GameMetrics::ClientPacketFailureReason::Validate;
        case PacketMetricsHooks::FailureReason::Handler:
            return GameMetrics::ClientPacketFailureReason::Handler;
        default:
            return GameMetrics::ClientPacketFailureReason::Handler;
        }
    }

    void RouteS2SRequestPacket(uint16 packetId, const void* packetObject)
    {
        if (packetObject == nullptr)
            return;

        switch (packetId)
        {
        case S2SPacketHandler::PKT_S2S_REQ_LOAD_PLAYER_DATA:
            GameMetrics::TrackS2SRequestPacket(packetId, *static_cast<const Protocol::S2S_REQ_LOAD_PLAYER_DATA*>(packetObject));
            break;
        case S2SPacketHandler::PKT_S2S_REQ_ITEMS_LOAD:
            GameMetrics::TrackS2SRequestPacket(packetId, *static_cast<const Protocol::S2S_REQ_ITEMS_LOAD*>(packetObject));
            break;
        case S2SPacketHandler::PKT_S2S_REQ_SAVE_PLAYER_CORE:
            GameMetrics::TrackS2SRequestPacket(packetId, *static_cast<const Protocol::S2S_REQ_SAVE_PLAYER_CORE*>(packetObject));
            break;
        case S2SPacketHandler::PKT_S2S_REQ_SAVE_INVENTORY:
            GameMetrics::TrackS2SRequestPacket(packetId, *static_cast<const Protocol::S2S_REQ_SAVE_INVENTORY*>(packetObject));
            break;
        case S2SPacketHandler::PKT_S2S_REQ_ITEM_CREATE:
            GameMetrics::TrackS2SRequestPacket(packetId, *static_cast<const Protocol::S2S_REQ_ITEM_CREATE*>(packetObject));
            break;
        case S2SPacketHandler::PKT_S2S_REQ_QUICKSLOT_LOAD:
            GameMetrics::TrackS2SRequestPacket(packetId, *static_cast<const Protocol::S2S_REQ_QUICKSLOT_LOAD*>(packetObject));
            break;
        case S2SPacketHandler::PKT_S2S_REQ_SAVE_QUICKSLOT:
            GameMetrics::TrackS2SRequestPacket(packetId, *static_cast<const Protocol::S2S_REQ_SAVE_QUICKSLOT*>(packetObject));
            break;
        case S2SPacketHandler::PKT_S2S_REQ_TRADE_COMMIT:
            GameMetrics::TrackS2SRequestPacket(packetId, *static_cast<const Protocol::S2S_REQ_TRADE_COMMIT*>(packetObject));
            break;
        default:
            break;
        }
    }

    void RouteS2SResponsePacket(uint16 packetId, const void* packetObject)
    {
        if (packetObject == nullptr)
            return;

        switch (packetId)
        {
        case S2SPacketHandler::PKT_S2S_RES_LOAD_PLAYER_DATA:
            GameMetrics::TrackS2SResponsePacket(packetId, *static_cast<const Protocol::S2S_RES_LOAD_PLAYER_DATA*>(packetObject));
            break;
        case S2SPacketHandler::PKT_S2S_RES_ITEMS_LOAD:
            GameMetrics::TrackS2SResponsePacket(packetId, *static_cast<const Protocol::S2S_RES_ITEMS_LOAD*>(packetObject));
            break;
        case S2SPacketHandler::PKT_S2S_RES_SAVE_PLAYER_CORE:
            GameMetrics::TrackS2SResponsePacket(packetId, *static_cast<const Protocol::S2S_RES_SAVE_PLAYER_CORE*>(packetObject));
            break;
        case S2SPacketHandler::PKT_S2S_RES_SAVE_INVENTORY:
            GameMetrics::TrackS2SResponsePacket(packetId, *static_cast<const Protocol::S2S_RES_SAVE_INVENTORY*>(packetObject));
            break;
        case S2SPacketHandler::PKT_S2S_RES_ITEM_CREATE:
            GameMetrics::TrackS2SResponsePacket(packetId, *static_cast<const Protocol::S2S_RES_ITEM_CREATE*>(packetObject));
            break;
        case S2SPacketHandler::PKT_S2S_RES_QUICKSLOT_LOAD:
            GameMetrics::TrackS2SResponsePacket(packetId, *static_cast<const Protocol::S2S_RES_QUICKSLOT_LOAD*>(packetObject));
            break;
        case S2SPacketHandler::PKT_S2S_RES_SAVE_QUICKSLOT:
            GameMetrics::TrackS2SResponsePacket(packetId, *static_cast<const Protocol::S2S_RES_SAVE_QUICKSLOT*>(packetObject));
            break;
        case S2SPacketHandler::PKT_S2S_RES_TRADE_COMMIT:
            GameMetrics::TrackS2SResponsePacket(packetId, *static_cast<const Protocol::S2S_RES_TRADE_COMMIT*>(packetObject));
            break;
        default:
            break;
        }
    }

    void HookOnDispatch(const char* handlerName, uint16 packetId)
    {
        if (IsHandlerName(handlerName, "ClientPacketHandler"))
            GameMetrics::OnClientPacketDispatch(packetId);
    }

    void HookOnHandled(const char* handlerName, uint16 packetId, double elapsedSeconds)
    {
        if (IsHandlerName(handlerName, "ClientPacketHandler"))
            GameMetrics::OnClientPacketHandled(packetId, elapsedSeconds);
    }

    void HookOnFailure(const char* handlerName, uint16 packetId, PacketMetricsHooks::FailureReason reason)
    {
        if (IsHandlerName(handlerName, "ClientPacketHandler"))
            GameMetrics::OnClientPacketFailure(packetId, ToClientFailureReason(reason));
    }

    void HookOnMakeSendBuffer(const char* handlerName, uint16 packetId, const void* packetObject)
    {
        if (IsHandlerName(handlerName, "S2SPacketHandler"))
            RouteS2SRequestPacket(packetId, packetObject);
    }

    void HookOnPacketParsed(const char* handlerName, uint16 packetId, const void* packetObject)
    {
        if (IsHandlerName(handlerName, "S2SPacketHandler"))
            RouteS2SResponsePacket(packetId, packetObject);
    }
}

namespace GameMetrics
{
    void Initialize()
    {
        PacketMetricsHooks::SetHooks(
            HookOnDispatch,
            HookOnHandled,
            HookOnFailure,
            HookOnMakeSendBuffer,
            HookOnPacketParsed);

        OnSessionCountChanged(0);
        OnIngamePlayerCountChanged(0);
        OnAutoCommitTargets(0);
        OnAutoCommitInflight(0);
        OnDirtySetSize(DirtySetDomain::Player, 0);
        OnDirtySetSize(DirtySetDomain::Inventory, 0);
        OnDirtySetSize(DirtySetDomain::QuickSlot, 0);
        OnAutoCommitSent(AutoCommitDomain::Core, 0);
        OnAutoCommitSent(AutoCommitDomain::Inventory, 0);
        OnAutoCommitSent(AutoCommitDomain::QuickSlot, 0);
        OnAutoCommitSkip(AutoCommitSkipReason::Inflight, 0);
        OnAutoCommitSkip(AutoCommitSkipReason::EmptySnapshot, 0);
        OnAutoCommitSkip(AutoCommitSkipReason::RedisMissing, 0);
        OnPersistenceMutation(PersistenceMutationDomain::QuickSlot, PersistenceMutationPath::Writeback, 0);
        OnPersistenceMutation(PersistenceMutationDomain::QuickSlot, PersistenceMutationPath::Immediate, 0);
    }

    void Shutdown()
    {
        PacketMetricsHooks::ClearHooks();

        {
            std::lock_guard<std::mutex> guard(GLobbyLock);
            GLobbyEnterStartUs.clear();
        }

        {
            std::lock_guard<std::mutex> guard(GS2SInflightLock);
            GS2SInflight.clear();
            GS2SNextCleanupUs = 0;
            GS2SLastOverflowWarnUs = 0;
        }
    }

    void OnClientPacketDispatch(std::uint16_t packetId)
    {
        GetGameMetricsRegistry().packetIngressCounter->Inc(1.0, { { "op", ClientPacketOpLabel(packetId) } });
    }

    void OnClientPacketHandled(std::uint16_t packetId, double elapsedSeconds)
    {
        if (elapsedSeconds < 0.0)
            return;

        GetGameMetricsRegistry().packetHandleHistogram->Observe(elapsedSeconds, { { "op", ClientPacketOpLabel(packetId) } });
    }

    void OnClientPacketFailure(std::uint16_t packetId, ClientPacketFailureReason reason)
    {
        GetGameMetricsRegistry().packetFailureCounter->Inc(
            1.0,
            { { "op", ClientPacketOpLabel(packetId) }, { "reason", ClientFailureReasonLabel(reason) } });
    }

    void OnLobbyEnterStart(std::uint64_t playerId)
    {
        if (playerId == 0)
            return;

        const std::uint64_t nowUs = NowSteadyMicroseconds();
        std::lock_guard<std::mutex> guard(GLobbyLock);
        GLobbyEnterStartUs[playerId] = nowUs;
    }

    void OnLobbyEnterComplete(std::uint64_t playerId)
    {
        if (playerId == 0)
            return;

        const std::uint64_t nowUs = NowSteadyMicroseconds();

        std::uint64_t startUs = 0;
        {
            std::lock_guard<std::mutex> guard(GLobbyLock);
            auto findIt = GLobbyEnterStartUs.find(playerId);
            if (findIt == GLobbyEnterStartUs.end())
                return;

            startUs = findIt->second;
            GLobbyEnterStartUs.erase(findIt);
        }

        if (nowUs < startUs)
            return;

        const double elapsedSeconds = static_cast<double>(nowUs - startUs) / 1000000.0;
        GetGameMetricsRegistry().lobbyWaitHistogram->Observe(elapsedSeconds, { { "type", "enter_game" } });
    }

    void OnLobbyEnterCancelled(std::uint64_t playerId)
    {
        if (playerId == 0)
            return;

        std::lock_guard<std::mutex> guard(GLobbyLock);
        GLobbyEnterStartUs.erase(playerId);
    }

    void OnSessionCountChanged(std::int64_t sessionCount)
    {
        GetGameMetricsRegistry().ccuGauge->Set(static_cast<double>(sessionCount));
    }

    void OnIngamePlayerCountChanged(std::int64_t ingameCount)
    {
        GetGameMetricsRegistry().ingameGauge->Set(static_cast<double>(ingameCount));
    }

    void OnBroadcastRecipients(HotRoomBroadcastKind kind, HotRoomBroadcastMode mode, std::size_t recipients)
    {
        GetGameMetricsRegistry().broadcastRecipientsHistogram->Observe(
            static_cast<double>(recipients),
            {
                { "kind", BroadcastKindLabel(kind) },
                { "mode", BroadcastModeLabel(mode) },
            });
    }

    void OnAoiUpdate(double elapsedSeconds)
    {
        if (elapsedSeconds < 0.0)
            return;

        GetGameMetricsRegistry().aoiUpdateHistogram->Observe(elapsedSeconds);
    }

    void OnAoiUpdateCount()
    {
        GetGameMetricsRegistry().aoiUpdateCounter->Inc();
    }

    void OnAoiCandidates(AoiObjectKind kind, std::size_t count)
    {
        GetGameMetricsRegistry().aoiCandidatesHistogram->Observe(
            static_cast<double>(count),
            { { "kind", AoiObjectKindLabel(kind) } });
    }

    void OnAoiVisible(AoiObjectKind kind, std::size_t count)
    {
        GetGameMetricsRegistry().aoiVisibleHistogram->Observe(
            static_cast<double>(count),
            { { "kind", AoiObjectKindLabel(kind) } });
    }

    void OnAutoCommitTick(double elapsedSeconds)
    {
        if (elapsedSeconds < 0.0)
            return;

        GetGameMetricsRegistry().autocommitTickHistogram->Observe(elapsedSeconds);
    }

    void OnAutoCommitTargets(std::size_t count)
    {
        GetGameMetricsRegistry().autocommitTargetsGauge->Set(static_cast<double>(count));
    }

    void OnAutoCommitInflight(std::size_t count)
    {
        GetGameMetricsRegistry().autocommitInflightGauge->Set(static_cast<double>(count));
    }

    void OnAutoCommitSent(AutoCommitDomain domain, std::size_t count)
    {
        GetGameMetricsRegistry().autocommitSentCounter->Inc(
            static_cast<double>(count),
            { { "domain", AutoCommitDomainLabel(domain) } });
    }

    void OnAutoCommitSkip(AutoCommitSkipReason reason, std::size_t count)
    {
        GetGameMetricsRegistry().autocommitSkipCounter->Inc(
            static_cast<double>(count),
            { { "reason", AutoCommitSkipReasonLabel(reason) } });
    }

    void OnDirtySetSize(DirtySetDomain domain, std::size_t count)
    {
        GetGameMetricsRegistry().dirtySetSizeGauge->Set(
            static_cast<double>(count),
            { { "domain", DirtySetDomainLabel(domain) } });
    }

    void OnPersistenceMutation(PersistenceMutationDomain domain, PersistenceMutationPath path, std::size_t count)
    {
        GetGameMetricsRegistry().persistenceMutationCounter->Inc(
            static_cast<double>(count),
            {
                { "domain", PersistenceMutationDomainLabel(domain) },
                { "path", PersistenceMutationPathLabel(path) },
            });
    }

    void TrackS2SRequestPacket(std::uint16_t packetId, const Protocol::S2S_REQ_LOAD_PLAYER_DATA& pkt)
    {
        TrackS2SRequestInternal(S2SOpFromRequestPacket(packetId), pkt.gamesessionid());
    }

    void TrackS2SRequestPacket(std::uint16_t packetId, const Protocol::S2S_REQ_ITEMS_LOAD& pkt)
    {
        TrackS2SRequestInternal(S2SOpFromRequestPacket(packetId), pkt.gamesessionid());
    }

    void TrackS2SRequestPacket(std::uint16_t packetId, const Protocol::S2S_REQ_SAVE_PLAYER_CORE& pkt)
    {
        TrackS2SRequestInternal(S2SOpFromRequestPacket(packetId), pkt.playerid());
    }

    void TrackS2SRequestPacket(std::uint16_t packetId, const Protocol::S2S_REQ_SAVE_INVENTORY& pkt)
    {
        TrackS2SRequestInternal(S2SOpFromRequestPacket(packetId), pkt.playerid());
    }

    void TrackS2SRequestPacket(std::uint16_t packetId, const Protocol::S2S_REQ_ITEM_CREATE& pkt)
    {
        TrackS2SRequestInternal(S2SOpFromRequestPacket(packetId), pkt.requestid());
    }

    void TrackS2SRequestPacket(std::uint16_t packetId, const Protocol::S2S_REQ_QUICKSLOT_LOAD& pkt)
    {
        TrackS2SRequestInternal(S2SOpFromRequestPacket(packetId), pkt.gamesessionid());
    }

    void TrackS2SRequestPacket(std::uint16_t packetId, const Protocol::S2S_REQ_SAVE_QUICKSLOT& pkt)
    {
        TrackS2SRequestInternal(S2SOpFromRequestPacket(packetId), pkt.playerid());
    }

    void TrackS2SRequestPacket(std::uint16_t packetId, const Protocol::S2S_REQ_TRADE_COMMIT& pkt)
    {
        TrackS2SRequestInternal(S2SOpFromRequestPacket(packetId), pkt.requestid());
    }

    void TrackS2SResponsePacket(std::uint16_t packetId, const Protocol::S2S_RES_LOAD_PLAYER_DATA& pkt)
    {
        TrackS2SResponseInternal(S2SOpFromResponsePacket(packetId), pkt.gamesessionid());
    }

    void TrackS2SResponsePacket(std::uint16_t packetId, const Protocol::S2S_RES_ITEMS_LOAD& pkt)
    {
        TrackS2SResponseInternal(S2SOpFromResponsePacket(packetId), pkt.gamesessionid());
    }

    void TrackS2SResponsePacket(std::uint16_t packetId, const Protocol::S2S_RES_SAVE_PLAYER_CORE& pkt)
    {
        TrackS2SResponseInternal(S2SOpFromResponsePacket(packetId), pkt.playerid());
    }

    void TrackS2SResponsePacket(std::uint16_t packetId, const Protocol::S2S_RES_SAVE_INVENTORY& pkt)
    {
        TrackS2SResponseInternal(S2SOpFromResponsePacket(packetId), pkt.playerid());
    }

    void TrackS2SResponsePacket(std::uint16_t packetId, const Protocol::S2S_RES_ITEM_CREATE& pkt)
    {
        TrackS2SResponseInternal(S2SOpFromResponsePacket(packetId), pkt.requestid());
    }

    void TrackS2SResponsePacket(std::uint16_t packetId, const Protocol::S2S_RES_QUICKSLOT_LOAD& pkt)
    {
        TrackS2SResponseInternal(S2SOpFromResponsePacket(packetId), pkt.gamesessionid());
    }

    void TrackS2SResponsePacket(std::uint16_t packetId, const Protocol::S2S_RES_SAVE_QUICKSLOT& pkt)
    {
        TrackS2SResponseInternal(S2SOpFromResponsePacket(packetId), pkt.playerid());
    }

    void TrackS2SResponsePacket(std::uint16_t packetId, const Protocol::S2S_RES_TRADE_COMMIT& pkt)
    {
        TrackS2SResponseInternal(S2SOpFromResponsePacket(packetId), pkt.requestid());
    }
}
