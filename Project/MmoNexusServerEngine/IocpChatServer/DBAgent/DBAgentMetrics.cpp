#include "pch.h"
#include "DBAgentMetrics.h"

#include "DBAgentPacketHandler.h"
#include "Metrics.h"
#include "MetricsSystem.h"
#include "PacketMetricsHooks.h"

#include <chrono>
#include <cstring>

namespace
{
    struct DBAgentMetricsRegistry
    {
        std::shared_ptr<Counter> reqCounter;
        std::shared_ptr<Histogram> reqHandleHistogram;
        std::shared_ptr<Counter> reqFailureCounter;
        std::shared_ptr<Histogram> queryHistogram;
        std::shared_ptr<Histogram> poolWaitHistogram;
        std::shared_ptr<Gauge> poolSizeGauge;
        std::shared_ptr<Gauge> poolInUseGauge;
    };

    enum class RequestOp : std::uint8_t
    {
        Other = 0,
        Login,
        LoadPlayerData,
        ItemsLoad,
        LoadGameData,
        HeartBeat,
        SavePlayerCore,
        SaveInventory,
        ItemCreate,
        GameItemUidSeed,
        QuickslotLoad,
        SaveQuickslot,
        TradeCommit,
    };

    thread_local RequestOp GTlsCurrentOp = RequestOp::Other;

    DBAgentMetricsRegistry& GetDBAgentMetricsRegistry()
    {
        static DBAgentMetricsRegistry metrics{
            MetricsSystem::Instance().RegisterCounter(
                "req_total",
                "Total DBAgent S2S requests by op.",
                { "op" }),
            MetricsSystem::Instance().RegisterHistogram(
                "req_handle_seconds",
                "DBAgent request handling time in seconds.",
                MetricsHistogramBuckets::DbQuerySeconds(),
                { "op" }),
            MetricsSystem::Instance().RegisterCounter(
                "req_failure_total",
                "Total DBAgent request failures by op and reason.",
                { "op", "reason" }),
            MetricsSystem::Instance().RegisterHistogram(
                "query_seconds",
                "Database query execution time in seconds.",
                MetricsHistogramBuckets::DbQuerySeconds(),
                { "op" }),
            MetricsSystem::Instance().RegisterHistogram(
                "pool_wait_seconds",
                "DB connection pool wait time in seconds.",
                MetricsHistogramBuckets::JobQueueWaitSeconds(),
                { "op" }),
            MetricsSystem::Instance().RegisterGauge(
                "pool_size",
                "Configured DB connection pool size."),
            MetricsSystem::Instance().RegisterGauge(
                "pool_inuse",
                "Number of DB connections currently borrowed."),
        };

        return metrics;
    }

    std::uint64_t NowSteadyMicroseconds()
    {
        return static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count());
    }

    RequestOp RequestOpFromPacketId(uint16 packetId)
    {
        switch (packetId)
        {
        case DBAgentPacketHandler::PKT_S2S_REQ_LOGIN:
            return RequestOp::Login;
        case DBAgentPacketHandler::PKT_S2S_REQ_LOAD_PLAYER_DATA:
            return RequestOp::LoadPlayerData;
        case DBAgentPacketHandler::PKT_S2S_REQ_ITEMS_LOAD:
            return RequestOp::ItemsLoad;
        case DBAgentPacketHandler::PKT_S2S_REQ_LOAD_GAME_DATA:
            return RequestOp::LoadGameData;
        case DBAgentPacketHandler::PKT_S2S_REQ_HEART_BEAT:
            return RequestOp::HeartBeat;
        case DBAgentPacketHandler::PKT_S2S_REQ_SAVE_PLAYER_CORE:
            return RequestOp::SavePlayerCore;
        case DBAgentPacketHandler::PKT_S2S_REQ_SAVE_INVENTORY:
            return RequestOp::SaveInventory;
        case DBAgentPacketHandler::PKT_S2S_REQ_ITEM_CREATE:
            return RequestOp::ItemCreate;
        case DBAgentPacketHandler::PKT_S2S_REQ_GAME_ITEM_UID_SEED:
            return RequestOp::GameItemUidSeed;
        case DBAgentPacketHandler::PKT_S2S_REQ_QUICKSLOT_LOAD:
            return RequestOp::QuickslotLoad;
        case DBAgentPacketHandler::PKT_S2S_REQ_SAVE_QUICKSLOT:
            return RequestOp::SaveQuickslot;
        case DBAgentPacketHandler::PKT_S2S_REQ_TRADE_COMMIT:
            return RequestOp::TradeCommit;
        default:
            return RequestOp::Other;
        }
    }

    const char* RequestOpLabel(RequestOp op)
    {
        switch (op)
        {
        case RequestOp::Login:
            return "login";
        case RequestOp::LoadPlayerData:
            return "load_player_data";
        case RequestOp::ItemsLoad:
            return "items_load";
        case RequestOp::LoadGameData:
            return "load_game_data";
        case RequestOp::HeartBeat:
            return "heart_beat";
        case RequestOp::SavePlayerCore:
            return "save_player_core";
        case RequestOp::SaveInventory:
            return "save_inventory";
        case RequestOp::ItemCreate:
            return "item_create";
        case RequestOp::GameItemUidSeed:
            return "game_item_uid_seed";
        case RequestOp::QuickslotLoad:
            return "quickslot_load";
        case RequestOp::SaveQuickslot:
            return "save_quickslot";
        case RequestOp::TradeCommit:
            return "trade_commit";
        default:
            return "other";
        }
    }

    const char* FailureReasonLabel(PacketMetricsHooks::FailureReason reason)
    {
        switch (reason)
        {
        case PacketMetricsHooks::FailureReason::Parse:
            return "parse";
        case PacketMetricsHooks::FailureReason::Validate:
            return "validate";
        case PacketMetricsHooks::FailureReason::Handler:
            return "handler";
        default:
            return "other";
        }
    }

    bool IsHandlerName(const char* handlerName, const char* expectedName)
    {
        if (handlerName == nullptr || expectedName == nullptr)
            return false;

        return std::strcmp(handlerName, expectedName) == 0;
    }

    void ObserveRequestDispatch(uint16 packetId)
    {
        const RequestOp op = RequestOpFromPacketId(packetId);
        GetDBAgentMetricsRegistry().reqCounter->Inc(1.0, { { "op", RequestOpLabel(op) } });
    }

    void ObserveRequestFailure(uint16 packetId, PacketMetricsHooks::FailureReason reason)
    {
        const RequestOp op = RequestOpFromPacketId(packetId);
        GetDBAgentMetricsRegistry().reqFailureCounter->Inc(
            1.0,
            { { "op", RequestOpLabel(op) }, { "reason", FailureReasonLabel(reason) } });
    }

    void ObserveRequestHandled(RequestOp op, double elapsedSeconds)
    {
        if (elapsedSeconds < 0.0)
            return;

        GetDBAgentMetricsRegistry().reqHandleHistogram->Observe(elapsedSeconds, { { "op", RequestOpLabel(op) } });
    }

    void HookOnDispatch(const char* handlerName, uint16 packetId)
    {
        if (IsHandlerName(handlerName, "DBAgentPacketHandler"))
            ObserveRequestDispatch(packetId);
    }

    void HookOnFailure(const char* handlerName, uint16 packetId, PacketMetricsHooks::FailureReason reason)
    {
        if (IsHandlerName(handlerName, "DBAgentPacketHandler"))
            ObserveRequestFailure(packetId, reason);
    }

    const char* CurrentOpLabel()
    {
        return RequestOpLabel(GTlsCurrentOp);
    }
}

namespace DBAgentMetrics
{
    ScopedRequestMetrics::ScopedRequestMetrics(std::uint16_t packetId)
    {
        const RequestOp op = RequestOpFromPacketId(static_cast<uint16>(packetId));
        _op = static_cast<std::uint8_t>(op);
        _previousOp = static_cast<std::uint8_t>(GTlsCurrentOp);
        _startUs = NowSteadyMicroseconds();
        GTlsCurrentOp = op;
    }

    ScopedRequestMetrics::~ScopedRequestMetrics()
    {
        const std::uint64_t nowUs = NowSteadyMicroseconds();
        const RequestOp op = static_cast<RequestOp>(_op);

        if (nowUs >= _startUs)
        {
            const double elapsedSeconds = static_cast<double>(nowUs - _startUs) / 1000000.0;
            ObserveRequestHandled(op, elapsedSeconds);
        }

        GTlsCurrentOp = static_cast<RequestOp>(_previousOp);
    }

    void Initialize()
    {
        PacketMetricsHooks::SetHooks(
            HookOnDispatch,
            nullptr,
            HookOnFailure,
            nullptr,
            nullptr);

        ObservePoolState(0, 0);
    }

    void Shutdown()
    {
        PacketMetricsHooks::ClearHooks();
    }

    void ObservePoolState(std::int64_t poolSize, std::int64_t poolInUse)
    {
        if (poolSize < 0)
            poolSize = 0;
        if (poolInUse < 0)
            poolInUse = 0;

        auto& metrics = GetDBAgentMetricsRegistry();
        metrics.poolSizeGauge->Set(static_cast<double>(poolSize));
        metrics.poolInUseGauge->Set(static_cast<double>(poolInUse));
    }

    void ObservePoolWait(double elapsedSeconds)
    {
        if (elapsedSeconds < 0.0)
            return;

        GetDBAgentMetricsRegistry().poolWaitHistogram->Observe(
            elapsedSeconds,
            { { "op", CurrentOpLabel() } });
    }

    void ObserveQueryDuration(double elapsedSeconds)
    {
        if (elapsedSeconds < 0.0)
            return;

        GetDBAgentMetricsRegistry().queryHistogram->Observe(
            elapsedSeconds,
            { { "op", CurrentOpLabel() } });
    }
}
