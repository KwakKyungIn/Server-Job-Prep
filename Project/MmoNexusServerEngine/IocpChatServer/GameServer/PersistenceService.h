#pragma once
#include <string>
#include <unordered_map>
#include <vector>

#include "Protocol.pb.h"
#include "Protocol_S2S.pb.h"

class RedisManager;

namespace Persistence
{
    // DB와 게임 서버 사이에서 Redis를 캐시 레이어로 사용하는 서비스 클래스
    // DB 부하를 줄이기 위해 Write-Back(Write-Behind) 패턴을 흉내내서 구현함
    // 데이터 흐름: DB -> Redis(Prime) -> Game Logic(Update) -> Redis(Update) -> DB(Snapshot)
    class PersistenceService
    {
    public:
        // 싱글톤 패턴 사용. 어디서든 접근 가능해야 함
        static PersistenceService& I();

        // 서버 켜질 때 RedisManager 연결
        void Init(RedisManager* redis);

        // ===== Prime (DB -> Redis) =====
        // 플레이어가 로그인할 때 DB에 있는 데이터를 Redis로 캐싱하는 단계
        // DB 접근을 최소화하기 위해 최초 1회만 로딩함
        void PrimeFromDb_PlayerCore(uint64 pid, const Protocol::StatInfo& st);
        void PrimeFromDb_Inventory(uint64 pid, const google::protobuf::RepeatedPtrField<Protocol::ItemInfo>& items);

        // ===== Runtime updates (Game -> Redis + Dirty) =====
        // 게임 플레이 도중 스탯이나 경험치가 변하면 Redis에 즉시 반영
        // markDirty가 true면 저장 대상(Dirty Set)에 등록해서 추후 DB 저장을 유도함
        void UpdatePlayerCore(uint64 pid, int32 level, int32 hp, int64 totalExp, bool markDirty = true);

        // 아이템 획득, 이동, 장착 변경 시 호출
        // Redis Hash 구조를 써서 특정 아이템 필드만 빠르게 갱신하도록 설계
        void UpdateInventoryItem(uint64 pid, uint64 itemUid,
            int32 templateId, int32 slotIndex, int32 count, bool equipped,
            bool markDirty = true);

        // 아이템 삭제 처리. Redis에서 지우고 Tombstone(삭제 기록)을 남김
        void RemoveInventoryItem(uint64 pid, uint64 itemUid, bool markDirty = true);

        // Dirty explicit
        // 수동으로 저장 필요 상태로 만듦
        void MarkDirty_PlayerCore(uint64 pid);
        void MarkDirty_Inventory(uint64 pid);

        // Tombstone
        // DB에 삭제 쿼리를 날리기 위해 삭제된 아이템의 UID를 별도 Set에 저장해둠
        void OnItemRemoved(uint64 pid, uint64 itemUid);

        // ===== Snapshot build (Redis -> S2S Req) =====
        // 주기적인 DB 저장을 위해 Redis에 있는 최신 데이터를 긁어와서 Protobuf 메시지로 만듦
        // DB 에이전트 서버로 보낼 패킷을 생성하는 과정
        bool BuildSnapshot_PlayerCore(uint64 pid, Protocol::S2S_REQ_SAVE_PLAYER_CORE& out);
        bool BuildSnapshot_Inventory(uint64 pid, Protocol::S2S_REQ_SAVE_INVENTORY& out);

        // ===== Clear on commit success =====
        // DB 저장이 성공했다고 응답이 오면 Dirty Flag를 해제함
        // 이제 Redis와 DB가 동기화된 상태
        void ClearDirtyOnCommitSuccess(uint64 pid, bool coreOk, bool invOk, bool qsOk);


        // ===== Prime (DB -> Redis) =====
        // 퀵슬롯 데이터 캐싱
        void PrimeFromDb_QuickSlot(uint64 pid, const google::protobuf::RepeatedPtrField<Protocol::QuickSlotInfo>& slots);

        // ===== Runtime updates =====
        // 퀵슬롯 변경 사항 반영
        void UpdateQuickSlot(uint64 pid, int32 slotIndex, Protocol::QuickSlotRefType refType, uint64 refId, bool markDirty = true);
        void MarkDirty_QuickSlot(uint64 pid);

        // ===== Snapshot build =====
        // 저장할 퀵슬롯 정보를 패킷으로 직렬화
        bool BuildSnapshot_QuickSlot(uint64 pid, Protocol::S2S_REQ_SAVE_QUICKSLOT& out);


    private:
        PersistenceService() = default;

        RedisManager* _redis = nullptr;
    };
}