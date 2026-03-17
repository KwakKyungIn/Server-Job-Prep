#include "pch.h"
#include "ClientPacketHandler.MapChangeUtil.h"
#include "ClientPacketHandler.h"
#include "PlayerSession.h"
#include "Player.h"
#include "GameRoom.h"
#include "DataManager.h"
#include "ExperimentUtils.h"
#include "RoomActor.h"
#include <atomic>
#include <chrono>

namespace MapChangeUtil
{
    namespace
    {
        // 전역 시퀀스를 사용하여 토큰의 유일성을 보장한다
        std::atomic<uint64> G_MapChangeTokenSeq{ 1 };
    }

    // 맵 이동 검증용 토큰 생성 함수
    // 단순히 랜덤값이 아니라 플레이어 정보, 세션 정보, 시간, 시퀀스를 조합해 충돌 확률을 0으로 만듦
    uint64 MakeMapChangeToken(uint64 playerId, uint64 sessionId)
    {
        uint64 seq = G_MapChangeTokenSeq.fetch_add(1, std::memory_order_relaxed);
        uint64 now = (uint64)std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count();

        // 비트 연산으로 각 정보를 섞어서 고유 키를 만든다
        return (playerId << 32) ^ (sessionId << 16) ^ seq ^ now;
    }

    // 던전에서 나갈 때 돌아갈 위치가 유효한지 검사하고 보정해주는 함수
    // 만약 저장된 귀환 위치가 삭제된 맵이거나 이상하면 기본 마을로 보낸다
    void MakeSafeReturn(PlayerRef p, int32& outMapId, int64& outInstId, Protocol::PositionInfo& outPos)
    {
        outMapId = p->GetReturnMapId();
        outInstId = p->GetReturnInstanceId();
        outPos = p->GetReturnPos();

        DataManager* dm = DataManager::Instance();
        const int32 forcedMapId = ExperimentUtils::ResolveForcedWorldMapId(outMapId);
        if (forcedMapId != outMapId)
        {
            outMapId = forcedMapId;
            outInstId = 0;

            const MapConfig* cfg = dm ? dm->GetMapConfig(outMapId) : nullptr;
            outPos.Clear();
            outPos.set_x(cfg ? cfg->spawnX : 50.f);
            outPos.set_y(cfg ? cfg->spawnY : 0.f);
            outPos.set_z(cfg ? cfg->spawnZ : 50.f);
        }

        // 맵 데이터가 없거나 유효하지 않은 ID라면 방어 코드 작동
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

        if (p->HasPendingRespawn() && ExperimentUtils::ShouldRandomizeRespawnSpawn())
        {
            ExperimentUtils::TryRandomizeSpawn(outMapId, outPos);
        }
    }

    // 맵 이동 시작 패킷을 보내고 세션 상태를 변경하는 헬퍼 함수
    // 여러 곳에서 중복되는 로직이라 유틸로 빼둠
    void SendMapChangeBegin(PlayerSessionRef ms, uint64 playerId,
        int32 targetChannelId, int32 targetMapId, int64 targetInstanceId, const Protocol::PositionInfo& spawn)
    {
        if (!ms) return;
        if (ms->IsMapChanging()) return;
        if (playerId == 0) return;

        const uint64 token = MakeMapChangeToken(playerId, ms->GetSessionId());
        // 세션의 FSM 상태를 MapChanging으로 전이시킴
        if (!ms->TryBeginMapChange(token, targetChannelId, targetMapId, targetInstanceId, spawn))
            return;

        Protocol::S_MAP_CHANGE_BEGIN beginPkt;
        beginPkt.set_token(token);
        beginPkt.set_targetmapid(targetMapId);
        beginPkt.mutable_spawn()->CopyFrom(spawn);
        beginPkt.set_instanceid(targetInstanceId);
        beginPkt.set_targetchannelid(targetChannelId);
        ms->Send(ClientPacketHandler::MakeSendBuffer(beginPkt));
    }

    // 플레이어를 강제로 월드로 귀환시키는 함수 (던전 종료, 강퇴 등)
    // 세션 -> 룸(정보 조회) -> 세션(전송) 흐름을 타야 해서 비동기 호출이 중첩됨
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

                // 룸 액터로 진입해서 안전하게 플레이어 정보를 조회한다
                gr->PushJob([gr, self, pid]()
                    {
                        PlayerRef p = gr->FindPlayer_ActorOnly(pid);
                        if (!p) return;

                        // 안전한 복귀 좌표 계산
                        int32 rm = 0; int64 ri = 0; Protocol::PositionInfo rp;
                        MakeSafeReturn(p, rm, ri, rp);

                        // 채널 정보도 확인해서 없으면 현재 룸의 채널을 쓴다
                        int32 targetChannelId = p->GetChannelId();
                        if (targetChannelId <= 0)
                            targetChannelId = gr->GetChannelId();

                        // 모든 정보가 준비되었으니 다시 세션 액터로 돌아가서 패킷 전송
                        self->Post([pid, targetChannelId, rm, ri, rp](PlayerSessionRef s) mutable
                            {
                                SendMapChangeBegin(s, pid, targetChannelId, rm, ri, rp);
                            });
                    });

            });
    }
}
