#pragma once
#include "Protocol.pb.h" 

class Player;
class PlayerSession;

using PlayerRef = std::shared_ptr<Player>;
using PlayerSessionRef = std::shared_ptr<PlayerSession>;

namespace MapChangeUtil
{
    // 토큰 생성
    uint64 MakeMapChangeToken(uint64 playerId, uint64 sessionId);

    // MapChange Begin 전송 + Session FSM Begin
    void SendMapChangeBegin(PlayerSessionRef ms,
        uint64 playerId,
        int32 targetChannelId,
        int32 targetMapId,
        int64 targetInstanceId,
        const Protocol::PositionInfo& spawn);

    // 안전한 월드 복귀 좌표 계산(리턴 위치/맵 유효성 검사)
    void MakeSafeReturn(PlayerRef p,
        int32& outMapId,
        int64& outInstId,
        Protocol::PositionInfo& outPos);

    // 던전/강제퇴출용: 현재 Room에서 ReturnLocation 기반 월드로 MapChangeBegin
    void ForceReturnToWorld(PlayerSessionRef ms);
}
