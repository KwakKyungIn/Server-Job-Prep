using Google.Protobuf;
using Google.Protobuf.Collections;
using Protocol;
using System;
using UnityEngine;

public class PacketHandler
{
    // [GIGACHAD] UI 및 게임 로직으로 이벤트를 토스하기 위한 Action들
    public static Action<bool> OnLoginResult;
    public static Action<S_ENTER_GAME_RES> OnEnterGame; // 내 캐릭터 입장
    public static Action<S_SPAWN> OnSpawn;              // 다른 캐릭터(또는 몬스터) 출현
    public static Action<S_DESPAWN> OnDespawn;          // 오브젝트 사라짐
    public static Action<S_MOVE> OnMove;                // 이동
    public static Action<string> OnChatMsg;

    // [Inventory Events]
    public static Action<RepeatedField<ItemInfo>> OnItemList;
    public static Action<ItemInfo> OnUpdateItem;
    public static Action<ulong> OnRemoveItem;

    public static Action<S_EQUIP_ITEM> OnEquipItem;
    public static Action<StatInfo> OnChangeStat;

    // [S_LOGIN_RES] 로그인 응답
    public static void S_LOGIN_RESHandler(ServerSession session, IMessage packet)
    {
        S_LOGIN_RES res = packet as S_LOGIN_RES;

        if (res.Success)
        {
            Debug.Log($"[Login Success] ID: {res.PlayerId}");
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

    // [S_ENTER_GAME_RES] 게임 입장 성공
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

    // [S_SPAWN] 다른 플레이어 + 몬스터 출현
    public static void S_SPAWNHandler(ServerSession session, IMessage packet)
    {
        S_SPAWN spawnPkt = packet as S_SPAWN;

        // [Modify] 이제 몬스터도 같이 온다. 로그 수정.
        int playerCount = spawnPkt.Players == null ? 0 : spawnPkt.Players.Count;
        int monsterCount = spawnPkt.Monsters == null ? 0 : spawnPkt.Monsters.Count;

        Debug.Log($"[Client Log] Received S_SPAWN. Players: {playerCount}, Monsters: {monsterCount}");

        if (OnSpawn != null)
            OnSpawn.Invoke(spawnPkt);
    }

    // [S_DESPAWN] 오브젝트 사라짐
    public static void S_DESPAWNHandler(ServerSession session, IMessage packet)
    {
        S_DESPAWN despawnPkt = packet as S_DESPAWN;

        // [Modify] PlayerIds -> ObjectIds로 이름 변경됨 (C# GenPackets 확인 필요)
        Debug.Log($"[Client Log] S_DESPAWN Count: {despawnPkt.ObjectIds.Count}");

        if (OnDespawn != null)
            OnDespawn.Invoke(despawnPkt);
    }

    // [S_MOVE] 이동 패킷
    public static void S_MOVEHandler(ServerSession session, IMessage packet)
    {
        S_MOVE movePkt = packet as S_MOVE;

        // [Modify] PlayerId -> ObjectId로 변경됨
        // 로그를 찍고 싶다면 movePkt.ObjectId 를 써야 함.
        // Debug.Log($"[Move] ObjID: {movePkt.ObjectId} Pos: {movePkt.PosInfo.X}, {movePkt.PosInfo.Z}");

        if (OnMove != null)
            OnMove.Invoke(movePkt);
    }

    // ============================================================
    // [ITEM & INVENTORY HANDLERS]
    // ============================================================

    public static void S_ITEM_LISTHandler(ServerSession session, IMessage packet)
    {
        S_ITEM_LIST pkt = packet as S_ITEM_LIST;
        Debug.Log($"[Inventory] Loaded {pkt.Items.Count} items.");

        if (OnItemList != null)
            OnItemList.Invoke(pkt.Items);
    }

    public static void S_CHANGE_ITEMHandler(ServerSession session, IMessage packet)
    {
        S_CHANGE_ITEM pkt = packet as S_CHANGE_ITEM;
        Debug.Log($"[Inventory] Item Updated: UID:{pkt.Item.ItemUid} ID:{pkt.Item.TemplateId}");

        if (OnUpdateItem != null)
            OnUpdateItem.Invoke(pkt.Item);
    }

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
        Debug.Log($"[Equip] ItemUID: {pkt.ItemUid}, Slot: {pkt.SlotIndex}, Equipped: {pkt.Equipped}");

        if (OnEquipItem != null)
            OnEquipItem.Invoke(pkt);
    }

    public static void S_CHANGE_STATHandler(ServerSession session, IMessage packet)
    {
        S_CHANGE_STAT pkt = packet as S_CHANGE_STAT;
        Debug.Log($"[Stat] Refreshed! HP: {pkt.StatInfo.Hp}/{pkt.StatInfo.MaxHp} ATK: {pkt.StatInfo.Attack}");

        if (OnChangeStat != null)
            OnChangeStat.Invoke(pkt.StatInfo);
    }

    public static void S_CHAT_RESHandler(ServerSession session, IMessage packet)
    {
    }

    public static void S_CHAT_NTFHandler(ServerSession session, IMessage packet)
    {
    }

    public static void S_HEART_BEAT_RESHandler(ServerSession session, IMessage packet)
    {
    }
}