#include "pch.h"
#include "GameRoom.h"
#include "GameMap.h"
#include "Player.h"
#include "PlayerSession.h"
#include "Monster.h"
#include "RoomManager.h"
#include "GameRoom.Net.h"
#include "MoveValidationUtils.h"
#include <cmath>
#include <iostream> // 로그용 헤더 (없으면 추가)
#include <iomanip>  // 소수점 예쁘게 찍기용

static bool IsFinitePos(const Protocol::PositionInfo& p)
{
    return std::isfinite(p.x()) && std::isfinite(p.y()) &&
        std::isfinite(p.z()) && std::isfinite(p.yaw());
}

static bool IsCrazyPos(const Protocol::PositionInfo& p)
{
    constexpr float kMaxAbs = 100000.0f; // 안전장치 (프로젝트에 맞게 조절 가능)
    return (std::fabs(p.x()) > kMaxAbs) || (std::fabs(p.y()) > kMaxAbs) || (std::fabs(p.z()) > kMaxAbs);
}

void GameRoom::HandleMove(PlayerSessionRef session, PlayerRef player, Protocol::C_MOVE pkt)
{
    if (!session || !player) return;

    const uint64 playerId = player->GetPlayerId();
    if (_players.find(playerId) == _players.end()) return;

    // ===== Step0) 입력 정상성 =====
    const auto& reqRaw = pkt.posinfo();
    if (!IsFinitePos(reqRaw) || IsCrazyPos(reqRaw))
    {
        std::cout << " [MOVE_INVALID_INPUT] ID: " << playerId << std::endl;
        return;
    }

    if (_grid.GetZoneIndex(reqRaw) < 0)
    {
        std::cout << " [MOVE_INVALID_INPUT] OUT_OF_BOUNDS ID: " << playerId << std::endl;
        return;
    }

    // 현재 권위 위치
    const Protocol::PositionInfo cur = *player->GetPosInfo();
    if (!IsFinitePos(cur))
        return;

    // ===== Step1) seq/time 검증 + dt 계산 =====
    const uint32 seq = pkt.move_seq();
    const uint32 tms = pkt.client_time_ms();

    const bool hasStamp = player->HasMoveStamp_ActorOnly();

    if (hasStamp)
    {
        const uint32 lastSeq = player->LastMoveSeq_ActorOnly();
        if (!MoveValidate::IsSeqNewer(seq, lastSeq))
        {
            if (seq == lastSeq)
            {
                // duplicate: 조용히 무시
                return;
            }

            std::cout << " [MOVE_SEQ_REWIND_DROP] ID: " << playerId
                << " seq=" << seq << " last=" << lastSeq << std::endl;
            return;
        }
    }

    const float dtSec = MoveValidate::ComputeDtSec(
        tms,
        player->LastClientTimeMs_ActorOnly(),
        0.02f, 0.25f, hasStamp);

    // speed 소스
    float speed = 0.f;
    if (auto* st = player->GetStatInfo())
        speed = static_cast<float>(st->speed());

    // ===== Speed clamp (기본 정책) =====
    Protocol::PositionInfo reqClamped;
    const auto speedRes = MoveValidate::CheckSpeed2D(
        cur, reqRaw, dtSec, speed, 0.30f, reqClamped);

    if (speedRes.policy == MoveValidate::SpeedPolicy::CLAMPED)
    {
        std::cout << " [SPEED_EXCEEDED_CLAMP] ID: " << playerId
            << " reqDist=" << speedRes.reqDist2D
            << " maxDist=" << speedRes.maxDist
            << " dt=" << speedRes.dtSec << std::endl;
    }
    // else: SPEED_OK 로그는 너무 시끄러우면 생략해도 됨.

    // ===== Step2~5) Nav Validate (B 제공) =====
    Protocol::PositionInfo fixed;
    if (!_map || _map->ValidateMove(cur, reqClamped, fixed) == false)
    {
        std::cout << " [FAIL] ID: " << playerId << " blocked by NavMesh!" << std::endl;
        return;
    }

    // 서버 권위 결과에 상태/회전/액션 붙여주기 (Nav는 x/y/z 중심)
    fixed.set_state(reqClamped.state());
    fixed.set_actionstate(reqClamped.actionstate());
    fixed.set_yaw(reqClamped.yaw());

    // ===== Step6) 반영 + zone/AOI + 브로드캐스트 =====
    const int32 oldZoneIndex = player->GetZoneIndex();
    const int32 newZoneIndex = _grid.GetZoneIndex(fixed);

    // (안전장치) 그리드 밖이면 드랍
    if (newZoneIndex < 0)
        return;

    player->SetPosInfo(fixed);

    const bool zoneChanged = (oldZoneIndex != newZoneIndex);

    if (zoneChanged)
    {
        Zone& oldZone = _grid.GetZone(oldZoneIndex);
        Zone& newZone = _grid.GetZone(newZoneIndex);
        oldZone.players.erase(player);
        newZone.players.insert(player);
        player->SetZoneIndex(newZoneIndex);
    }

    if (ShouldUpdateAOI(player, zoneChanged))
        UpdateAOI(session, player, false);

    // MOVE는 "내 visiblePlayers"에게만 전송
    Protocol::S_MOVE movePkt;
    movePkt.set_objectid(playerId);
    *movePkt.mutable_posinfo() = fixed;
    SendBufferRef sb = ClientPacketHandler::MakeSendBuffer(movePkt);

    auto& vis = player->VisiblePlayers_ActorOnly();
    for (uint64 vid : vis)
    {
        if (vid == playerId) continue;
        SendToPlayer(vid, sb);
    }

    // ===== 마지막에 MoveStamp 갱신 (Room thread ONLY) =====
    player->SetMoveStamp_ActorOnly(seq, tms, fixed, ::GetTickCount64());
}


void GameRoom::HandleMoveById(PlayerSessionRef session, uint64 playerId, Protocol::C_MOVE pkt)
{
	auto it = _players.find(playerId);
	if (it == _players.end())
		return;

	HandleMove(session, it->second, pkt); // 기존 로직 재사용
}
