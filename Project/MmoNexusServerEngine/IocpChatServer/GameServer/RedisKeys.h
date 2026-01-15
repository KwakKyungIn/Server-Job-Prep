#pragma once
#include <string>

namespace Persistence
{
    inline std::string KeyPlayerCore(uint64 pid) { return "p:" + std::to_string(pid) + ":core"; }
    inline std::string KeyPlayerInv(uint64 pid) { return "p:" + std::to_string(pid) + ":inv"; }
    inline std::string KeyPlayerInvDel(uint64 pid) { return "p:" + std::to_string(pid) + ":invdel"; }

    inline std::string KeyPlayerQuick(uint64 pid) { return "p:" + std::to_string(pid) + ":qs"; }
    inline std::string KeyDirtyQuick() { return "dirty:qs"; }

    inline std::string KeyDirtyPlayer() { return "dirty:player"; }
    inline std::string KeyDirtyInv() { return "dirty:inv"; }
}
