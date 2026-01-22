#include "pch.h"
#include "RedisCodec.h"

namespace Persistence
{
    std::string PackItem(int32 templateId, int32 slot, int32 count, bool equipped)
    {
        return std::to_string(templateId) + "|" +
            std::to_string(slot) + "|" +
            std::to_string(count) + "|" +
            (equipped ? "1" : "0");
    }

    static bool ParseI32(const std::string& s, int32& out)
    {
        try
        {
            long long v = std::stoll(s);
            if (v < INT32_MIN || v > INT32_MAX) return false;
            out = static_cast<int32>(v);
            return true;
        }
        catch (...) { return false; }
    }

    bool UnpackItem(const std::string& s,
        int32& outTemplateId,
        int32& outSlot,
        int32& outCount,
        bool& outEquipped)
    {
        size_t p1 = s.find('|');
        if (p1 == std::string::npos) return false;
        size_t p2 = s.find('|', p1 + 1);
        if (p2 == std::string::npos) return false;
        size_t p3 = s.find('|', p2 + 1);
        if (p3 == std::string::npos) return false;

        std::string a = s.substr(0, p1);
        std::string b = s.substr(p1 + 1, p2 - (p1 + 1));
        std::string c = s.substr(p2 + 1, p3 - (p2 + 1));
        std::string d = s.substr(p3 + 1);

        int32 t = 0, slot = 0, cnt = 0;
        if (!ParseI32(a, t)) return false;
        if (!ParseI32(b, slot)) return false;
        if (!ParseI32(c, cnt)) return false;

        if (d == "1") outEquipped = true;
        else if (d == "0") outEquipped = false;
        else return false;

        outTemplateId = t;
        outSlot = slot;
        outCount = cnt;
        return true;
    }

    std::string PackQuick(int32 refType, uint64 refId)
    {
        return std::to_string(refType) + "|" + std::to_string(refId);
    }

    static bool ParseU64(const std::string& s, uint64& out)
    {
        try { out = (uint64)std::stoull(s); return true; }
        catch (...) { return false; }
    }

    bool UnpackQuick(const std::string& s, int32& outRefType, uint64& outRefId)
    {
        size_t p = s.find('|');
        if (p == std::string::npos) return false;

        std::string a = s.substr(0, p);
        std::string b = s.substr(p + 1);

        int32 rt = 0;
        if (!ParseI32(a, rt)) return false;

        uint64 rid = 0;
        if (!ParseU64(b, rid)) return false;

        outRefType = rt;
        outRefId = rid;
        return true;
    }
}
