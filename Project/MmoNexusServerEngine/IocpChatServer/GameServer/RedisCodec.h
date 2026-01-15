#pragma once

namespace Persistence
{
    // templateId|slot|count|equipped(0/1)
    std::string PackItem(int32 templateId, int32 slot, int32 count, bool equipped);

    // refType|refId
    std::string PackQuick(int32 refType, uint64 refId);
    bool UnpackQuick(const std::string& s, int32& outRefType, uint64& outRefId);

    // returns false if parse fail
    bool UnpackItem(const std::string& s,
        int32& outTemplateId,
        int32& outSlot,
        int32& outCount,
        bool& outEquipped);
}

