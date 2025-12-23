#pragma once
#include <atomic>
#include <chrono>
#include "PlayerSession.h"
#include "Player.h"
#include "DataManager.h"
#include "ClientPacketHandler.h"

namespace DungeonUtils
{
    inline std::atomic<uint64> G_MapChangeTokenSeq{ 1 };

    inline uint64 MakeMapChangeToken(uint64 playerId, uint64 sessionId)
    {
        uint64 seq = G_MapChangeTokenSeq.fetch_add(1, std::memory_order_relaxed);
        uint64 now = (uint64)std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count();

        return (playerId << 32) ^ (sessionId << 16) ^ seq ^ now;
    }

    inline void MakeSafeReturn(PlayerRef p, int32& outMapId, int64& outInstId, Protocol::PositionInfo& outPos)
    {
        outMapId = p->GetReturnMapId();
        outInstId = p->GetReturnInstanceId();
        outPos = p->GetReturnPos();

        DataManager* dm = DataManager::Instance();
        if (!dm || !dm->IsValidMapId(outMapId))
        {
            outMapId = (dm ? dm->GetDefaultMapId() : 1);
            outInstId = 0;

            const MapConfig* cfg = dm ? dm->GetMapConfig(outMapId) : nullptr;
            outPos.Clear();
            outPos.set_x(cfg ? cfg->spawnX : 50.f);
            outPos.set_y(cfg ? cfg->spawnY : 0.f);
            outPos.set_z(cfg ? cfg->spawnZ : 50.f);
        }
    }

    inline void SendMapChangeBegin(PlayerSessionRef ms, PlayerRef p,
        int32 targetMapId, int64 targetInstanceId, const Protocol::PositionInfo& spawn)
    {
        if (!ms || !p) return;
        if (ms->IsMapChanging()) return;

        const uint64 token = MakeMapChangeToken(p->GetPlayerId(), ms->GetSessionId());
        if (!ms->TryBeginMapChange(token, targetMapId, targetInstanceId, spawn))
            return;

        Protocol::S_MAP_CHANGE_BEGIN beginPkt;
        beginPkt.set_token(token);
        beginPkt.set_targetmapid(targetMapId);
        beginPkt.mutable_spawn()->CopyFrom(spawn);
        beginPkt.set_instanceid(targetInstanceId);

        ms->Send(ClientPacketHandler::MakeSendBuffer(beginPkt));
    }

    inline void ForceReturnToWorld(PlayerSessionRef ms)
    {
        if (!ms) return;
        ms->PostPlayer([](PlayerSessionRef self, PlayerRef p)
            {
                int32 rm = 0; int64 ri = 0; Protocol::PositionInfo rp;
                MakeSafeReturn(p, rm, ri, rp);
                SendMapChangeBegin(self, p, rm, ri, rp);
            });
    }
}
