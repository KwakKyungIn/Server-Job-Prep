#include "pch.h"
#include "PersistenceService.h"
#include "RedisManager.h"
#include "RedisKeys.h"
#include "RedisCodec.h"

namespace
{
    static bool ToU64(const std::string& s, uint64& out)
    {
        try { out = static_cast<uint64>(std::stoull(s)); return true; }
        catch (...) { return false; }
    }

    static bool ToI32(const std::string& s, int32& out)
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

    static bool ToI64(const std::string& s, int64& out)
    {
        try { out = static_cast<int64>(std::stoll(s)); return true; }
        catch (...) { return false; }
    }
}

namespace Persistence
{
    PersistenceService& PersistenceService::I()
    {
        static PersistenceService s;
        return s;
    }

    void PersistenceService::Init(RedisManager* redis)
    {
        _redis = redis;
    }

    void PersistenceService::PrimeFromDb_PlayerCore(uint64 pid, const Protocol::StatInfo& st)
    {
        if (!_redis) return;

        const std::string key = KeyPlayerCore(pid);

        _redis->HSet(key, "level", std::to_string(st.level()));
        _redis->HSet(key, "hp", std::to_string(st.hp()));
        _redis->HSet(key, "totalExp", std::to_string(static_cast<long long>(st.totalexp())));

        // Prime는 dirty 찍지 않는다. 혹시 남아있으면 제거.
        _redis->SRem(KeyDirtyPlayer(), std::to_string(pid));
    }

    void PersistenceService::PrimeFromDb_Inventory(uint64 pid, const google::protobuf::RepeatedPtrField<Protocol::ItemInfo>& items)
    {
        if (!_redis) return;

        const std::string invKey = KeyPlayerInv(pid);
        const std::string delKey = KeyPlayerInvDel(pid);

        // 전체 스냅샷으로 Prime
        _redis->Del(invKey);

        for (const auto& it : items)
        {
            const uint64 itemUid = static_cast<uint64>(it.itemuid());
            const std::string field = std::to_string(itemUid);
            const std::string val = PackItem(it.templateid(), it.slot(), it.count(), it.isequipped());
            _redis->HSet(invKey, field, val);
        }

        // tombstone 초기화
        _redis->Del(delKey);

        // Prime는 dirty 제거
        _redis->SRem(KeyDirtyInv(), std::to_string(pid));
    }

    void PersistenceService::MarkDirty_PlayerCore(uint64 pid)
    {
        if (!_redis) return;
        _redis->SAdd(KeyDirtyPlayer(), std::to_string(pid));
    }

    void PersistenceService::MarkDirty_Inventory(uint64 pid)
    {
        if (!_redis) return;
        _redis->SAdd(KeyDirtyInv(), std::to_string(pid));
    }

    void PersistenceService::UpdatePlayerCore(uint64 pid, int32 level, int32 hp, int64 totalExp, bool markDirty)
    {
        if (!_redis) return;

        const std::string key = KeyPlayerCore(pid);

        _redis->HSet(key, "level", std::to_string(level));
        _redis->HSet(key, "hp", std::to_string(hp));
        _redis->HSet(key, "totalExp", std::to_string(static_cast<long long>(totalExp)));

        if (markDirty) MarkDirty_PlayerCore(pid);
    }

    void PersistenceService::UpdateInventoryItem(uint64 pid, uint64 itemUid,
        int32 templateId, int32 slotIndex, int32 count, bool equipped,
        bool markDirty)
    {
        if (!_redis) return;

        const std::string invKey = KeyPlayerInv(pid);
        const std::string field = std::to_string(itemUid);
        const std::string val = PackItem(templateId, slotIndex, count, equipped);

        _redis->HSet(invKey, field, val);

        if (markDirty) MarkDirty_Inventory(pid);
    }

    void PersistenceService::OnItemRemoved(uint64 pid, uint64 itemUid)
    {
        if (!_redis) return;
        _redis->SAdd(KeyPlayerInvDel(pid), std::to_string(itemUid));
    }

    void PersistenceService::RemoveInventoryItem(uint64 pid, uint64 itemUid, bool markDirty)
    {
        if (!_redis) return;

        const std::string invKey = KeyPlayerInv(pid);
        const std::string field = std::to_string(itemUid);

        _redis->HDel(invKey, field);
        OnItemRemoved(pid, itemUid);

        if (markDirty) MarkDirty_Inventory(pid);
    }

    bool PersistenceService::BuildSnapshot_PlayerCore(uint64 pid, Protocol::S2S_REQ_SAVE_PLAYER_CORE& out)
    {
        if (!_redis) return false;

        std::unordered_map<std::string, std::string> kv;
        if (!_redis->HGetAll(KeyPlayerCore(pid), kv))
            return false;

        // 필수 필드 없으면 실패
        if (kv.count("level") == 0 || kv.count("hp") == 0 || kv.count("totalExp") == 0)
            return false;

        int32 level = 0, hp = 0;
        int64 totalExp = 0;

        if (!ToI32(kv["level"], level)) return false;
        if (!ToI32(kv["hp"], hp)) return false;
        if (!ToI64(kv["totalExp"], totalExp)) return false;

        out.set_playerid(pid);
        out.set_level(level);
        out.set_hp(hp);
        out.set_totalexp(totalExp);
        return true;
    }

    bool PersistenceService::BuildSnapshot_Inventory(uint64 pid, Protocol::S2S_REQ_SAVE_INVENTORY& out)
    {
        if (!_redis) return false;

        std::unordered_map<std::string, std::string> inv;
        if (!_redis->HGetAll(KeyPlayerInv(pid), inv))
            return false;

        std::vector<std::string> dels;
        _redis->SMembers(KeyPlayerInvDel(pid), dels); // 없으면 빈 벡터로 OK

        out.set_playerid(pid);
        out.clear_items();
        out.clear_deleteditemuids();

        for (const auto& kv : inv)
        {
            const std::string& fieldItemUidStr = kv.first;
            const std::string& packed = kv.second;

            uint64 itemUid = 0;
            if (!ToU64(fieldItemUidStr, itemUid))
                continue;

            int32 templateId = 0, slot = 0, count = 0;
            bool eq = false;
            if (!UnpackItem(packed, templateId, slot, count, eq))
                continue;

            // auto 대신 타입 명시해도 OK (더 안전)
            Protocol::ItemInfo* item = out.add_items();
            item->set_itemuid(itemUid);
            item->set_templateid(templateId);
            item->set_slot(slot);
            item->set_count(count);
            item->set_isequipped(eq);
        }


        for (const auto& s : dels)
        {
            uint64 uid = 0;
            if (!ToU64(s, uid)) continue;
            out.add_deleteditemuids(uid);
        }

        return true;
    }

    void PersistenceService::ClearDirtyOnCommitSuccess(uint64 pid, bool coreOk, bool invOk)
    {
        if (!_redis) return;

        const std::string pidStr = std::to_string(pid);

        if (coreOk)
            _redis->SRem(KeyDirtyPlayer(), pidStr);

        if (invOk)
        {
            _redis->SRem(KeyDirtyInv(), pidStr);
            _redis->Del(KeyPlayerInvDel(pid)); // 인벤 저장 성공했으면 tombstone 비움
        }
    }
}
