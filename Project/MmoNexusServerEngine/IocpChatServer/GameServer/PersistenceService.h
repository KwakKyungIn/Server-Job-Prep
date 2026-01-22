#pragma once
#include <string>
#include <unordered_map>
#include <vector>

#include "Protocol.pb.h"
#include "Protocol_S2S.pb.h"

class RedisManager;

namespace Persistence
{
    class PersistenceService
    {
    public:
        static PersistenceService& I();

        void Init(RedisManager* redis);

        // ===== Prime (DB -> Redis) =====
        void PrimeFromDb_PlayerCore(uint64 pid, const Protocol::StatInfo& st);
        void PrimeFromDb_Inventory(uint64 pid, const google::protobuf::RepeatedPtrField<Protocol::ItemInfo>& items);

        // ===== Runtime updates (Game -> Redis + Dirty) =====
        void UpdatePlayerCore(uint64 pid, int32 level, int32 hp, int64 totalExp, bool markDirty = true);

        void UpdateInventoryItem(uint64 pid, uint64 itemUid,
            int32 templateId, int32 slotIndex, int32 count, bool equipped,
            bool markDirty = true);

        void RemoveInventoryItem(uint64 pid, uint64 itemUid, bool markDirty = true);

        // Dirty explicit
        void MarkDirty_PlayerCore(uint64 pid);
        void MarkDirty_Inventory(uint64 pid);

        // Tombstone
        void OnItemRemoved(uint64 pid, uint64 itemUid);

        // ===== Snapshot build (Redis -> S2S Req) =====
        bool BuildSnapshot_PlayerCore(uint64 pid, Protocol::S2S_REQ_SAVE_PLAYER_CORE& out);
        bool BuildSnapshot_Inventory(uint64 pid, Protocol::S2S_REQ_SAVE_INVENTORY& out);

        // ===== Clear on commit success =====
        void ClearDirtyOnCommitSuccess(uint64 pid, bool coreOk, bool invOk, bool qsOk);


        // ===== Prime (DB -> Redis) =====
        void PrimeFromDb_QuickSlot(uint64 pid, const google::protobuf::RepeatedPtrField<Protocol::QuickSlotInfo>& slots);

        // ===== Runtime updates =====
        void UpdateQuickSlot(uint64 pid, int32 slotIndex, Protocol::QuickSlotRefType refType, uint64 refId, bool markDirty = true);
        void MarkDirty_QuickSlot(uint64 pid);

        // ===== Snapshot build =====
        bool BuildSnapshot_QuickSlot(uint64 pid, Protocol::S2S_REQ_SAVE_QUICKSLOT& out);


    private:
        PersistenceService() = default;

        RedisManager* _redis = nullptr;
    };
}
