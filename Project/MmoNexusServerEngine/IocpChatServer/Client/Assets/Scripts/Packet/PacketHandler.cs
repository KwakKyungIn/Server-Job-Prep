using Google.Protobuf;
using Google.Protobuf.Collections;
using Protocol;
using System;
using UnityEngine;

public class PacketHandler
{
    // [GIGACHAD] UI 및 게임 로직으로 이벤트를 토스하기 위한 Action들
    // 패킷 핸들러는 로직을 모른다. 그냥 "왔다"고 알릴 뿐.
    public static Action<bool> OnLoginResult;
    public static Action<S_ENTER_GAME_RES> OnEnterGame; // 내 캐릭터 입장
    public static Action<S_SPAWN> OnSpawn;              // 다른 캐릭터(또는 몬스터) 출현
    public static Action<S_DESPAWN> OnDespawn;          // 캐릭터 사라짐
    public static Action<S_MOVE> OnMove;                // 이동
    public static Action<string> OnChatMsg;

    // [Inventory Events] - UI나 매니저가 구독할 이벤트
    public static Action<RepeatedField<ItemInfo>> OnItemList; // 전체 목록 갱신
    public static Action<ItemInfo> OnUpdateItem;              // 아이템 1개 변경/추가
    public static Action<ulong> OnRemoveItem;                 // 아이템 삭제 (ItemUID)

    public static Action<S_EQUIP_ITEM> OnEquipItem; // 장착/해제 결과 알림
    public static Action<StatInfo> OnChangeStat;    // 스탯 변화 알림

    // [S_LOGIN_RES] 로그인 응답
    public static void S_LOGIN_RESHandler(ServerSession session, IMessage packet)
    {
        S_LOGIN_RES res = packet as S_LOGIN_RES;

        if (res.Success)
        {
            Debug.Log($"[Login Success] ID: {res.PlayerId}");

            // 로그인 성공했으니, "게임 입장 요청(C_ENTER_GAME)"을 바로 보내거나 UI 처리를 함
            // 여기서는 UI 이벤트를 호출해줌
            if (OnLoginResult != null)
                OnLoginResult.Invoke(true);
        }
        else
        {
            Debug.Log($"[Login Failed] Check Server");
            if (OnLoginResult != null)
                OnLoginResult.Invoke(false);
        }
    }

    // [S_ENTER_GAME_RES] 게임 입장 성공 (내 캐릭터 생성 타이밍)
    public static void S_ENTER_GAME_RESHandler(ServerSession session, IMessage packet)
    {
        S_ENTER_GAME_RES res = packet as S_ENTER_GAME_RES;
        if (res.Success)
        {
            Debug.Log("[Enter Game] Success!");
            if (OnEnterGame != null)
                OnEnterGame.Invoke(res);
        }
    }

    // [S_SPAWN] 다른 플레이어(또는 몬스터)가 시야에 들어옴
    public static void S_SPAWNHandler(ServerSession session, IMessage packet)
    {
        S_SPAWN spawnPkt = packet as S_SPAWN;
        UnityEngine.Debug.Log($"[Client Log] Received S_SPAWN. Players Count: {spawnPkt.Players.Count}");

        if (OnSpawn != null)
            OnSpawn.Invoke(spawnPkt);
    }

    // [S_DESPAWN] 다른 플레이어가 시야에서 사라짐
    public static void S_DESPAWNHandler(ServerSession session, IMessage packet)
    {
        S_DESPAWN despawnPkt = packet as S_DESPAWN;
        if (OnDespawn != null)
            OnDespawn.Invoke(despawnPkt);
    }

    // [S_MOVE] 이동 패킷 수신 (좌표 동기화)
    public static void S_MOVEHandler(ServerSession session, IMessage packet)
    {
        S_MOVE movePkt = packet as S_MOVE;
        if (OnMove != null)
            OnMove.Invoke(movePkt);
    }

    // ============================================================
    // [ITEM & INVENTORY HANDLERS] (NEW)
    // ============================================================

    // [S_ITEM_LIST] 로그인 직후 인벤토리 전체 목록 수신
    public static void S_ITEM_LISTHandler(ServerSession session, IMessage packet)
    {
        S_ITEM_LIST pkt = packet as S_ITEM_LIST;
        Debug.Log($"[Inventory] Loaded {pkt.Items.Count} items.");

        // 인벤토리 UI나 데이터 매니저에게 "야, 목록 갱신해"라고 알림
        if (OnItemList != null)
            OnItemList.Invoke(pkt.Items);
    }

    // [S_CHANGE_ITEM] 아이템 획득, 이동, 개수 변경, 장착 등
    public static void S_CHANGE_ITEMHandler(ServerSession session, IMessage packet)
    {
        S_CHANGE_ITEM pkt = packet as S_CHANGE_ITEM;
        Debug.Log($"[Inventory] Item Updated: UID:{pkt.Item.ItemUid} ID:{pkt.Item.TemplateId}");

        if (OnUpdateItem != null)
            OnUpdateItem.Invoke(pkt.Item);
    }

    // [S_REMOVE_ITEM] 아이템 삭제/소모
    public static void S_REMOVE_ITEMHandler(ServerSession session, IMessage packet)
    {
        S_REMOVE_ITEM pkt = packet as S_REMOVE_ITEM;
        Debug.Log($"[Inventory] Item Removed: UID:{pkt.ItemUid}");

        if (OnRemoveItem != null)
            OnRemoveItem.Invoke(pkt.ItemUid);
    }

    public static void S_EQUIP_ITEMHandler(ServerSession session, IMessage packet)
    {
        S_EQUIP_ITEM pkt = packet as S_EQUIP_ITEM;

        // UI 갱신을 위해 어떤 아이템(UID)이, 몇 번 슬롯(SlotIndex)에서, 장착(True)/해제(False) 됐는지 알린다.
        Debug.Log($"[Equip] ItemUID: {pkt.ItemUid}, Slot: {pkt.SlotIndex}, Equipped: {pkt.Equipped}");

        if (OnEquipItem != null)
            OnEquipItem.Invoke(pkt);
    }

    // [S_CHANGE_STAT] 스탯 변화 수신 (장비 장착, 레벨업 등)
    public static void S_CHANGE_STATHandler(ServerSession session, IMessage packet)
    {
        S_CHANGE_STAT pkt = packet as S_CHANGE_STAT;

        Debug.Log($"[Stat] Refreshed! HP: {pkt.StatInfo.Hp}/{pkt.StatInfo.MaxHp} ATK: {pkt.StatInfo.Attack}");

        // 내 플레이어 컨트롤러나 스탯 UI 창에게 "야, 정보 갱신해" 라고 던짐
        if (OnChangeStat != null)
            OnChangeStat.Invoke(pkt.StatInfo);
    }

    // [S_CHAT_RES] 내 채팅 전송 성공 여부
    public static void S_CHAT_RESHandler(ServerSession session, IMessage packet)
    {
        // 필요하면 구현
    }

    // [S_CHAT_NTF] 남의 채팅 알림
    public static void S_CHAT_NTFHandler(ServerSession session, IMessage packet)
    {
        
    }

    public static void S_HEART_BEAT_RESHandler(ServerSession session, IMessage packet)
    {
    }
}