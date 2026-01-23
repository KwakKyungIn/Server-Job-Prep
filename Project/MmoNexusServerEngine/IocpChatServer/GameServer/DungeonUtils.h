#pragma once
#include <atomic>
#include <chrono>
#include "PlayerSession.h"
#include "Player.h"
#include "DataManager.h"
#include "ClientPacketHandler.h"

namespace DungeonUtils
{
    // 맵 이동 요청이 동시에 몰릴 수 있으니 원자적으로 시퀀스 관리
    inline std::atomic<uint64> G_MapChangeTokenSeq{ 1 };

    // 맵 이동 검증 토큰 생성 함수
    // 단순히 1씩 증가하는 값만 쓰면 보안상 취약할 수 있어서
    // 플레이어ID + 세션ID + 시퀀스 + 시간값을 비트연산으로 섞어서 유니크한 토큰을 만듦
    inline uint64 MakeMapChangeToken(uint64 playerId, uint64 sessionId)
    {
        uint64 seq = G_MapChangeTokenSeq.fetch_add(1, std::memory_order_relaxed);
        uint64 now = (uint64)std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count();

        // 비트 XOR 연산으로 섞어줌 (충돌 방지 및 위변조 난이도 상승)
        return (playerId << 32) ^ (sessionId << 16) ^ seq ^ now;
    }

    // 던전에서 나갈 때 안전한 위치를 계산하는 함수
    // 만약 돌아갈 맵 정보가 유효하지 않다면(삭제된 맵 등) 강제로 마을로 보내야 함
    inline void MakeSafeReturn(PlayerRef p, int32& outMapId, int64& outInstId, Protocol::PositionInfo& outPos)
    {
        outMapId = p->GetReturnMapId();
        outInstId = p->GetReturnInstanceId();
        outPos = p->GetReturnPos();

        DataManager* dm = DataManager::Instance();
        // 돌아갈 맵 ID가 데이터 매니저에 없으면 뭔가 잘못된 상황임
        if (!dm || !dm->IsValidMapId(outMapId))
        {
            // 예외 처리: 기본 맵(마을)으로 강제 귀환 설정
            outMapId = (dm ? dm->GetDefaultMapId() : 1);
            outInstId = 0; // 마을은 보통 인스턴스가 아니라 정적 맵이라 0

            const MapConfig* cfg = dm ? dm->GetMapConfig(outMapId) : nullptr;
            outPos.Clear();
            // 마을의 스폰 포인트로 좌표 초기화
            outPos.set_x(cfg ? cfg->spawnX : 50.f);
            outPos.set_y(cfg ? cfg->spawnY : 0.f);
            outPos.set_z(cfg ? cfg->spawnZ : 50.f);
        }
    }

    // 클라이언트에게 맵 이동 시작을 알리는 패킷 전송
    // 여기서 토큰을 발급해서 클라가 로딩 완료 후 다시 보내게 함 (Handshaking)
    inline void SendMapChangeBegin(PlayerSessionRef ms, PlayerRef p,
        int32 targetMapId, int64 targetInstanceId, const Protocol::PositionInfo& spawn)
    {
        if (!ms || !p) return;
        // 이미 이동 중인 상태라면 중복 요청 무시
        if (ms->IsMapChanging()) return;

        const uint64 token = MakeMapChangeToken(p->GetPlayerId(), ms->GetSessionId());

        // 세션 상태를 '이동 중'으로 변경하고 토큰 저장
        if (!ms->TryBeginMapChange(token, targetMapId, targetInstanceId, spawn))
            return;

        Protocol::S_MAP_CHANGE_BEGIN beginPkt;
        beginPkt.set_token(token);
        beginPkt.set_targetmapid(targetMapId);
        beginPkt.mutable_spawn()->CopyFrom(spawn);
        beginPkt.set_instanceid(targetInstanceId);

        ms->Send(ClientPacketHandler::MakeSendBuffer(beginPkt));
    }

    // 강제로 월드(마을)로 소환하는 유틸리티
    // 던전 클리어하거나 사망해서 부활할 때 사용
    inline void ForceReturnToWorld(PlayerSessionRef ms)
    {
        if (!ms) return;
        // 세션 스레드에서 안전하게 실행하기 위해 JobQueue에 넣음
        ms->PostPlayer([](PlayerSessionRef self, PlayerRef p)
            {
                int32 rm = 0; int64 ri = 0; Protocol::PositionInfo rp;
                // 안전한 귀환 좌표 계산 후 이동 시작
                MakeSafeReturn(p, rm, ri, rp);
                SendMapChangeBegin(self, p, rm, ri, rp);
            });
    }
}