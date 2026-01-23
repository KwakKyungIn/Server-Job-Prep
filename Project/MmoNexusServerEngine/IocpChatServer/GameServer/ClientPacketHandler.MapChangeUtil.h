#pragma once
#include "Protocol.pb.h" 

class Player;
class PlayerSession;

using PlayerRef = std::shared_ptr<Player>;
using PlayerSessionRef = std::shared_ptr<PlayerSession>;

namespace MapChangeUtil
{
    // 이동 검증을 위한 일회성 고유 토큰 생성
    uint64 MakeMapChangeToken(uint64 playerId, uint64 sessionId);

    // MapChange Begin 패킷을 구성하여 전송하고 세션의 상태를 변경함
    void SendMapChangeBegin(PlayerSessionRef ms,
        uint64 playerId,
        int32 targetChannelId,
        int32 targetMapId,
        int64 targetInstanceId,
        const Protocol::PositionInfo& spawn);

    // 귀환 위치가 유효한지 검사하고 이상하면 기본 맵으로 보정해주는 안전장치
    void MakeSafeReturn(PlayerRef p,
        int32& outMapId,
        int64& outInstId,
        Protocol::PositionInfo& outPos);

    // 던전 종료 시 호출되어 플레이어를 저장된 위치로 강제 복귀시키는 함수
    void ForceReturnToWorld(PlayerSessionRef ms);
}