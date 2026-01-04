#include "pch.h"
#include "GameRoom.h"
#include "GameMap.h"
#include "Player.h"
#include "PlayerSession.h"
#include "Monster.h"
#include "RoomManager.h"
#include "GameRoom.Net.h"

#include <iostream> // 로그용 헤더 (없으면 추가)
#include <iomanip>  // 소수점 예쁘게 찍기용

void GameRoom::HandleMove(PlayerSessionRef session, PlayerRef player, Protocol::C_MOVE pkt)
{
    if (!session || !player) return;

    uint64 playerId = player->GetPlayerId();
    if (_players.find(playerId) == _players.end()) return;

    // 0) 서버 권위 이동 검증 + 보정
    Protocol::PositionInfo fixed;
    const Protocol::PositionInfo cur = *player->GetPosInfo();

    // ValidateMove가 실패하면 로그 찍고 리턴 (벽에 막힘 등)
    if (!_map || _map->ValidateMove(cur, pkt.posinfo(), fixed) == false)
    {
        // [Debug] 이동 실패 로그 (벽 충돌 등)
        std::cout << "❌ [FAIL] ID: " << playerId << " blocked by NavMesh!" << std::endl;
        return;
    }

    int32 oldZoneIndex = player->GetZoneIndex();
    int32 newZoneIndex = _grid.GetZoneIndex(fixed);

    fixed.set_state(pkt.posinfo().state());
    fixed.set_yaw(pkt.posinfo().yaw());
    // 위치 갱신 (요청값이 아니라 "보정값"이 권위)
    player->SetPosInfo(fixed);

    // ✅ [GigaChad Log] 실시간 좌표 확인
    // std::fixed + std::setprecision(2) : 소수점 2자리까지만 깔끔하게
    std::cout << "🏃 [MOVE] ID: " << playerId
        << " | Pos: (" << std::fixed << std::setprecision(2)
        << fixed.x() << ", " << fixed.y() << ", " << fixed.z() << ")"
        << " | Zone: " << newZoneIndex << std::endl;

    const bool zoneChanged = (oldZoneIndex != newZoneIndex);

    // zone membership 이동(먼저 반영)
    if (zoneChanged)
    {
        Zone& oldZone = _grid.GetZone(oldZoneIndex);
        Zone& newZone = _grid.GetZone(newZoneIndex);
        oldZone.players.erase(player);
        newZone.players.insert(player);
        player->SetZoneIndex(newZoneIndex);

        // [Debug] 존 변경 로그
        std::cout << "   -> 🌐 Zone Changed: " << oldZoneIndex << " -> " << newZoneIndex << std::endl;
    }

    // AOI 재계산(정책)
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
}
void GameRoom::HandleMoveById(PlayerSessionRef session, uint64 playerId, Protocol::C_MOVE pkt)
{
	auto it = _players.find(playerId);
	if (it == _players.end())
		return;

	HandleMove(session, it->second, pkt); // 기존 로직 재사용
}
