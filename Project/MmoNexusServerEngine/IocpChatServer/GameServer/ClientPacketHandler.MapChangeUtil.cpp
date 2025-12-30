#include "pch.h"
#include "ClientPacketHandler.MapChangeUtil.h"
#include "ClientPacketHandler.h"
#include "PlayerSession.h"
#include "Player.h"
#include "GameRoom.h"
#include "DataManager.h"
#include "RoomActor.h"
#include <atomic>
#include <chrono>

namespace MapChangeUtil
{
    namespace
    {
        std::atomic<uint64> G_MapChangeTokenSeq{ 1 };
    }

    uint64 MakeMapChangeToken(uint64 playerId, uint64 sessionId)
    {
        uint64 seq = G_MapChangeTokenSeq.fetch_add(1, std::memory_order_relaxed);
        uint64 now = (uint64)std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count();

        // 충돌만 안 나면 됨
        return (playerId << 32) ^ (sessionId << 16) ^ seq ^ now;
    }

    void MakeSafeReturn(PlayerRef p, int32& outMapId, int64& outInstId, Protocol::PositionInfo& outPos)
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

    void SendMapChangeBegin(PlayerSessionRef ms, uint64 playerId,
        int32 targetMapId, int64 targetInstanceId, const Protocol::PositionInfo& spawn)
    {
        if (!ms) return;
        if (ms->IsMapChanging()) return;
        if (playerId == 0) return;

        const uint64 token = MakeMapChangeToken(playerId, ms->GetSessionId());
        if (!ms->TryBeginMapChange(token, targetMapId, targetInstanceId, spawn))
            return;

        Protocol::S_MAP_CHANGE_BEGIN beginPkt;
        beginPkt.set_token(token);
        beginPkt.set_targetmapid(targetMapId);
        beginPkt.mutable_spawn()->CopyFrom(spawn);
        beginPkt.set_instanceid(targetInstanceId);
        ms->Send(ClientPacketHandler::MakeSendBuffer(beginPkt));
    }

    void ForceReturnToWorld(PlayerSessionRef ms)
    {
        if (!ms) return;

        ms->Post([](PlayerSessionRef self)
            {
                const uint64 pid = self->GetPlayerId_AnyThread();
                if (pid == 0) return;

                RoomActorRef room = self->GetCurrentRoom_ActorOnly();
                auto gr = (room && room->GetKind() == RoomKind::Game)
                    ? std::dynamic_pointer_cast<GameRoom>(room)
                    : nullptr;
                if (!gr) return;

                gr->PushJob([gr, self, pid]()
                    {
                        PlayerRef p = gr->FindPlayer_ActorOnly(pid);
                        if (!p) return;

                        int32 rm = 0; int64 ri = 0; Protocol::PositionInfo rp;
                        MakeSafeReturn(p, rm, ri, rp);

                        self->Post([pid, rm, ri, rp](PlayerSessionRef s) mutable
                            {
                                SendMapChangeBegin(s, pid, rm, ri, rp);
                            });
                    });
            });
    }
}
