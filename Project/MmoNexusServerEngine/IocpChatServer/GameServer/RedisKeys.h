#pragma once
#include <string>

namespace Persistence
{
    // [Redis Key Schema]
    // 키 이름 꼬이면 나중에 디버깅 지옥이라 여기서 일괄 관리함
    // 형식: "p:{PlayerId}:{DataType}" (Redis 관례인 콜론 구분자 사용)

    inline std::string KeyPlayerCore(uint64 pid) { return "p:" + std::to_string(pid) + ":core"; }
    inline std::string KeyPlayerInv(uint64 pid) { return "p:" + std::to_string(pid) + ":inv"; }

    // 삭제된 아이템 추적용 (Tombstone 패턴)
    inline std::string KeyPlayerInvDel(uint64 pid) { return "p:" + std::to_string(pid) + ":invdel"; }

    inline std::string KeyPlayerQuick(uint64 pid) { return "p:" + std::to_string(pid) + ":qs"; }

    // [Dirty Sets]
    // Write-Back 패턴 구현용
    // 변경사항이 있는 유저들의 ID를 Set 자료구조에 모아둠
    // 별도 배치 스레드가 이 Set을 보고 DB에 갱신 쿼리를 날림
    inline std::string KeyDirtyQuick() { return "dirty:qs"; }
    inline std::string KeyDirtyPlayer() { return "dirty:player"; }
    inline std::string KeyDirtyInv() { return "dirty:inv"; }
}