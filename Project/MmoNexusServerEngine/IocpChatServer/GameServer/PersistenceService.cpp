#include "pch.h"
#include "PersistenceService.h"
#include "RedisManager.h"
#include "RedisKeys.h"
#include "RedisCodec.h"
#include <unordered_set>

static constexpr int32 QS_MAX = 12; // 0~11
namespace
{
    // Redis는 모든 데이터를 문자열로 저장하니까 형변환이 필수임
    // stoull 같은 거 쓰다가 예외 터지면 서버 죽으니까 try-catch로 감싸서 안전하게 변환
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
            // int32 범위 넘어가는 오버플로우 체크
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

    // 로그인 직후 DB 데이터를 Redis에 밀어넣음
    void PersistenceService::PrimeFromDb_PlayerCore(uint64 pid, const Protocol::StatInfo& st, int64 gold)
    {
        if (!_redis) return;

        const std::string key = KeyPlayerCore(pid);

        // Hash 자료구조를 써서 필드별로 저장
        _redis->HSet(key, "level", std::to_string(st.level()));
        _redis->HSet(key, "hp", std::to_string(st.hp()));
        _redis->HSet(key, "totalExp", std::to_string(static_cast<long long>(st.totalexp())));
        _redis->HSet(key, "gold", std::to_string(static_cast<long long>(gold)));

        // DB에서 막 가져온 싱싱한 데이터니까 Dirty Flag는 끈다
        // 혹시라도 남아있을지 모를 이전 세션의 잔재를 제거
        _redis->SRem(KeyDirtyPlayer(), std::to_string(pid));
    }

    void PersistenceService::PrimeFromDb_Inventory(uint64 pid, const google::protobuf::RepeatedPtrField<Protocol::ItemInfo>& items)
    {
        if (!_redis) return;

        const std::string invKey = KeyPlayerInv(pid);
        const std::string delKey = KeyPlayerInvDel(pid);

        // 기존에 캐싱된 인벤토리 정보가 있다면 싹 날리고 시작해야 꼬이지 않음
        _redis->Del(invKey);

        for (const auto& it : items)
        {
            const uint64 itemUid = static_cast<uint64>(it.itemuid());
            const std::string field = std::to_string(itemUid);
            // 아이템 상세 정보는 패킹해서 하나의 문자열로 저장 (Redis 메모리 절약)
            const std::string val = PackItem(it.templateid(), it.slot(), it.count(), it.isequipped());
            _redis->HSet(invKey, field, val);
        }

        // 삭제 대기열(Tombstone)도 초기화
        _redis->Del(delKey);

        // 얘도 마찬가지로 DB와 동기화된 상태니까 Dirty 제거
        _redis->SRem(KeyDirtyInv(), std::to_string(pid));
    }

    void PersistenceService::MarkDirty_PlayerCore(uint64 pid)
    {
        if (!_redis) return;
        // 변경된 유저 목록(Set)에 내 PID를 등록. 나중에 배치로 저장할 때 이 목록을 참조함
        _redis->SAdd(KeyDirtyPlayer(), std::to_string(pid));
    }

    void PersistenceService::MarkDirty_Inventory(uint64 pid)
    {
        if (!_redis) return;
        _redis->SAdd(KeyDirtyInv(), std::to_string(pid));
    }

    // 게임 중 레벨업하거나 데미지 입었을 때 호출
    void PersistenceService::UpdatePlayerCore(uint64 pid, int32 level, int32 hp, int64 totalExp, bool markDirty)
    {
        if (!_redis) return;

        const std::string key = KeyPlayerCore(pid);

        _redis->HSet(key, "level", std::to_string(level));
        _redis->HSet(key, "hp", std::to_string(hp));
        _redis->HSet(key, "totalExp", std::to_string(static_cast<long long>(totalExp)));

        // 보통은 무조건 Dirty 찍어서 저장 예약함
        if (markDirty) MarkDirty_PlayerCore(pid);
    }

    void PersistenceService::UpdatePlayerGold(uint64 pid, int64 gold, bool markDirty)
    {
        if (!_redis) return;

        const std::string key = KeyPlayerCore(pid);
        _redis->HSet(key, "gold", std::to_string(static_cast<long long>(gold)));

        if (markDirty) MarkDirty_PlayerCore(pid);
    }

    void PersistenceService::UpdateInventoryItem(uint64 pid, uint64 itemUid,
        int32 templateId, int32 slotIndex, int32 count, bool equipped,
        bool markDirty)
    {
        if (!_redis) return;

        const std::string invKey = KeyPlayerInv(pid);
        const std::string field = std::to_string(itemUid);
        // 아이템 정보가 바뀌면(수량 변경, 장착 등) 다시 패킹해서 덮어씌움
        const std::string val = PackItem(templateId, slotIndex, count, equipped);

        _redis->HSet(invKey, field, val);

        if (markDirty) MarkDirty_Inventory(pid);
    }

    void PersistenceService::OnItemRemoved(uint64 pid, uint64 itemUid)
    {
        if (!_redis) return;
        // DB에서도 지워야 하니까 삭제된 아이템 UID를 별도로 기록해둠 (Tombstone 패턴)
        _redis->SAdd(KeyPlayerInvDel(pid), std::to_string(itemUid));
    }

    void PersistenceService::RemoveInventoryItem(uint64 pid, uint64 itemUid, bool markDirty)
    {
        if (!_redis) return;

        const std::string invKey = KeyPlayerInv(pid);
        const std::string field = std::to_string(itemUid);

        // Redis 캐시에서 즉시 삭제
        _redis->HDel(invKey, field);
        // DB 삭제를 위해 기록
        OnItemRemoved(pid, itemUid);

        if (markDirty) MarkDirty_Inventory(pid);
    }

    // DB 저장을 위해 현재 Redis에 있는 스탯 정보를 긁어와서 패킷으로 만듦
    bool PersistenceService::BuildSnapshot_PlayerCore(uint64 pid, Protocol::S2S_REQ_SAVE_PLAYER_CORE& out)
    {
        if (!_redis) return false;

        std::unordered_map<std::string, std::string> kv;
        if (!_redis->HGetAll(KeyPlayerCore(pid), kv))
            return false;

        // 필수 데이터가 하나라도 없으면 저장하면 안 됨 (데이터 오염 방지)
        if (kv.count("level") == 0 || kv.count("hp") == 0 || kv.count("totalExp") == 0 || kv.count("gold") == 0)
            return false;

        int32 level = 0, hp = 0;
        int64 totalExp = 0;
        int64 gold = 0;

        if (!ToI32(kv["level"], level)) return false;
        if (!ToI32(kv["hp"], hp)) return false;
        if (!ToI64(kv["totalExp"], totalExp)) return false;
        if (!ToI64(kv["gold"], gold)) return false;

        out.set_playerid(pid);
        out.set_level(level);
        out.set_hp(hp);
        out.set_totalexp(totalExp);
        out.set_gold(gold);
        return true;
    }

    // Redis에 있는 인벤토리 전체 + 삭제된 아이템 목록을 패킷으로 구성
    bool PersistenceService::BuildSnapshot_Inventory(uint64 pid, Protocol::S2S_REQ_SAVE_INVENTORY& out)
    {
        if (!_redis) return false;

        std::unordered_map<std::string, std::string> inv;
        // HGetAll로 인벤토리 전체 조회
        if (!_redis->HGetAll(KeyPlayerInv(pid), inv))
            return false;

        std::vector<std::string> dels;
        _redis->SMembers(KeyPlayerInvDel(pid), dels); // 삭제 대기 목록 조회

        out.set_playerid(pid);
        out.clear_items();
        out.clear_deleteditemuids();

        for (const auto& kv : inv)
        {
            const std::string& fieldItemUidStr = kv.first;
            const std::string& packed = kv.second;

            uint64 itemUid = 0;
            if (!ToU64(fieldItemUidStr, itemUid))
                continue; // 키가 이상하면 스킵

            int32 templateId = 0, slot = 0, count = 0;
            bool eq = false;
            // 패킹된 문자열 파싱
            if (!UnpackItem(packed, templateId, slot, count, eq))
                continue;

            Protocol::ItemInfo* item = out.add_items();
            item->set_itemuid(itemUid);
            item->set_templateid(templateId);
            item->set_slot(slot);
            item->set_count(count);
            item->set_isequipped(eq);
        }

        // 삭제된 아이템 UID들도 같이 보내줘야 DB에서 DELETE 쿼리를 날릴 수 있음
        for (const auto& s : dels)
        {
            uint64 uid = 0;
            if (!ToU64(s, uid)) continue;
            out.add_deleteditemuids(uid);
        }

        return true;
    }

    void PersistenceService::PrimeFromDb_QuickSlot(uint64 pid,
        const google::protobuf::RepeatedPtrField<Protocol::QuickSlotInfo>& slots)
    {
        if (!_redis) return;

        const std::string key = KeyPlayerQuick(pid);
        _redis->Del(key);

        // [Rule] DB 데이터가 꼬여서 같은 아이템이 여러 퀵슬롯에 등록되어 있을 수 있음
        // 로딩 단계에서 중복 검사해서 걸러냄
        HashSet<uint64> seenItemUids;

        for (const auto& s : slots)
        {
            const int32 idx = s.slotindex();
            if (idx < 0 || idx >= QS_MAX)
                continue;

            const int32 rt = (int32)s.reftype();
            const uint64 rid = (uint64)s.refid();

            if (rt == (int32)Protocol::QS_NONE || rid == 0)
                continue;

            // 아이템 타입인 경우 중복 등록 방지 로직
            if (rt == (int32)Protocol::QS_ITEM)
            {
                if (seenItemUids.find(rid) != seenItemUids.end())
                    continue;
                seenItemUids.insert(rid);
            }

            _redis->HSet(key, std::to_string(idx), PackQuick(rt, rid));
        }

        _redis->SRem(KeyDirtyQuick(), std::to_string(pid));
    }


    void PersistenceService::MarkDirty_QuickSlot(uint64 pid)
    {
        if (!_redis) return;
        _redis->SAdd(KeyDirtyQuick(), std::to_string(pid));
    }

    void PersistenceService::UpdateQuickSlot(uint64 pid, int32 slotIndex, Protocol::QuickSlotRefType refType, uint64 refId, bool markDirty)
    {
        if (!_redis) return;

        const std::string key = KeyPlayerQuick(pid);
        const std::string field = std::to_string(slotIndex);

        // 빈 슬롯으로 변경하는 경우 Redis에서 필드 자체를 삭제
        if (refType == Protocol::QS_NONE || refId == 0)
            _redis->HDel(key, field);
        else
            _redis->HSet(key, field, PackQuick((int32)refType, refId));

        if (markDirty) MarkDirty_QuickSlot(pid);
    }

    bool PersistenceService::BuildSnapshot_QuickSlot(uint64 pid, Protocol::S2S_REQ_SAVE_QUICKSLOT& out)
    {
        if (!_redis) return false;

        std::unordered_map<std::string, std::string> kv;

        // RedisManager에서 HGetAll 실패 시 false 반환하도록 되어있음
        // 퀵슬롯은 없을 수도 있지만(Empty), DB 에러랑 구분하기 위해 체크
        _redis->HGetAll(KeyPlayerQuick(pid), kv);

        out.set_playerid(pid);
        out.clear_slots();

        for (const auto& it : kv)
        {
            int32 slotIndex = 0;
            if (!ToI32(it.first, slotIndex))
                continue;

            if (slotIndex < 0 || slotIndex >= QS_MAX)
                continue;

            int32 refType = 0;
            uint64 refId = 0;
            if (!UnpackQuick(it.second, refType, refId))
                continue;

            auto* s = out.add_slots();
            s->set_slotindex(slotIndex);
            s->set_reftype((Protocol::QuickSlotRefType)refType);
            s->set_refid(refId);
        }

        return true;
    }

    // DB 저장 작업이 완료되면 호출됨
    // 성공한 항목에 대해서만 Dirty Flag를 지워서 다음 주기까지 저장을 안 하게 만듦
    void PersistenceService::ClearDirtyOnCommitSuccess(uint64 pid, bool coreOk, bool invOk, bool qsOk)
    {
        if (!_redis) return;

        const std::string pidStr = std::to_string(pid);

        if (coreOk) _redis->SRem(KeyDirtyPlayer(), pidStr);

        if (invOk)
        {
            _redis->SRem(KeyDirtyInv(), pidStr);
            // 저장이 끝났으니 삭제 기록(Tombstone)도 이제 필요 없음
            _redis->Del(KeyPlayerInvDel(pid));
        }

        if (qsOk)
            _redis->SRem(KeyDirtyQuick(), pidStr);
    }
}
