using Google.Protobuf;
using Google.Protobuf.Collections;
using Protocol;
using System;
using UnityEngine;

public class PacketHandler
{
    // [GIGACHAD] UI 및 게임 로직으로 이벤트를 토스하기 위한 Action들

    // [Change] 로그인 결과: 이제 bool이 아니라 패킷(S_LOGIN)을 통째로 넘김 (토큰 포함)
    public static Action<S_LOGIN> OnLogin;

    // [Change] 게임 입장: S_ENTER_GAME_RES -> S_ENTER_GAME
    public static Action<S_ENTER_GAME> OnEnterGame;

    public static Action<S_SPAWN> OnSpawn;
    public static Action<S_DESPAWN> OnDespawn;
    public static Action<S_MOVE> OnMove;
    public static Action<string> OnChatMsg;
    public static Action<bool> OnChatRes; // 채팅 전송 성공여부

    // [Inventory Events]
    public static Action<RepeatedField<ItemInfo>> OnItemList;
    public static Action<ItemInfo> OnUpdateItem;
    public static Action<ulong> OnRemoveItem;

    // [Combat Events]
    public static Action<S_SKILL> OnSkill;
    public static Action<S_CHANGE_HP> OnChangeHp;

    public static Action<S_EQUIP_ITEM> OnEquipItem;
    public static Action<StatInfo> OnChangeStat;

    public static Action<S_MAP_CHANGE_BEGIN> OnMapChangeBegin;
    public static Action<S_MAP_CHANGE_END> OnMapChangeEnd;



    public static Action<S_DUNGEON_ENTER_RES> OnDungeonEnterRes;
    public static Action<S_DUNGEON_EXIT_RES> OnDungeonExitRes;


    public static Action<RepeatedField<QuickSlotInfo>> OnQuickSlotList;
    public static Action<QuickSlotInfo> OnQuickSlotChanged;


    //====================나중에 채울거임===========================
    public static Action<S_PARTY_CHAT_NTF> OnPartyChatNtf;
    public static Action<S_PARTY_INFO_NTF> OnPartyInfoNtf;
    public static Action<S_PARTY_RESULT> OnPartyResult;
    public static Action<S_PARTY_INVITE_NTF> OnPartyInviteNtf;
    public static Action<S_PARTY_STATUS_NTF> OnPartyStatusNtf;


    // [Trade Events]
    public static Action<S_TRADE_INVITE> OnTradeInvite;
    public static Action<S_TRADE_START> OnTradeStart;
    public static Action<S_TRADE_OFFER_UPDATE> OnTradeOfferUpdate;
    public static Action<S_TRADE_READY_STATE> OnTradeReadyState;
    public static Action<S_TRADE_LOCKED> OnTradeLocked;
    public static Action<S_TRADE_CANCELLED> OnTradeCancelled;
    public static Action<S_TRADE_RESULT> OnTradeResult;
    // ============================================================
    // [LOGIN & ENTRY HANDLERS]
    // ============================================================

    // [S_LOGIN] 로그인 응답 (토큰 수신)
    public static void S_LOGINHandler(ServerSession session, IMessage packet)
    {
        S_LOGIN pkt = packet as S_LOGIN;

        if (pkt.Success)
        {
            Debug.Log($"[Login Success] Token: {pkt.Token}");
        }
        else
        {
            Debug.Log($"[Login Failed] Check ID/PW");
        }

        if (OnLogin != null)
            OnLogin.Invoke(pkt);
    }

    // [S_ENTER_GAME] 게임 입장 성공 (캐릭터 정보 수신)
    public static void S_ENTER_GAMEHandler(ServerSession session, IMessage packet)
    {
        S_ENTER_GAME pkt = packet as S_ENTER_GAME;

        if (pkt.Success)
        {
            Debug.Log("[Enter Game] Success!");
            if (OnEnterGame != null)
                OnEnterGame.Invoke(pkt);

            if (pkt.MyPlayer != null && pkt.MyPlayer.StatInfo != null)
                OnChangeStat?.Invoke(pkt.MyPlayer.StatInfo);

        }
    }

    // ============================================================
    // [WORLD OBJECT HANDLERS]
    // ============================================================

    // [S_SPAWN] 다른 플레이어 + 몬스터 출현
    // [S_SPAWN] 다른 플레이어 + 몬스터 출현
    public static void S_SPAWNHandler(ServerSession session, IMessage packet)
    {
        Debug.Log($"🚨🚨🚨 [S_SPAWNHandler] ENTRY POINT!"); // ← 가장 첫 줄에 추가

        S_SPAWN pkt = packet as S_SPAWN;

        int playerCount = pkt.Players == null ? 0 : pkt.Players.Count;
        int monsterCount = pkt.Monsters == null ? 0 : pkt.Monsters.Count;

        Debug.Log($"🚨🚨🚨 [S_SPAWNHandler] CALLED! Players={playerCount}, Monsters={monsterCount}");

        if (OnSpawn != null)
        {
            Debug.Log($"✅ [S_SPAWNHandler] Invoking OnSpawn event");
            OnSpawn.Invoke(pkt);
        }
        else
        {
            Debug.LogError($"❌ [S_SPAWNHandler] OnSpawn is NULL! Event not registered!");
        }
    }
    // [S_DESPAWN] 오브젝트 사라짐
    public static void S_DESPAWNHandler(ServerSession session, IMessage packet)
    {
        S_DESPAWN pkt = packet as S_DESPAWN;

        // [Check] Protocol.proto에서 field name이 objectIds인지 확인
        // 만약 C#에서 ObjectIds로 생성되었다면 그대로 사용.
        Debug.Log($"[Despawn] Count: {pkt.ObjectIds.Count}");

        if (OnDespawn != null)
            OnDespawn.Invoke(pkt);
    }

    // [S_MOVE] 이동 패킷
    public static void S_MOVEHandler(ServerSession session, IMessage packet)
    {
        S_MOVE pkt = packet as S_MOVE;

        // Debug.Log($"[Move] ObjID: {pkt.ObjectId} Pos: {pkt.PosInfo.X}, {pkt.PosInfo.Z}");

        if (OnMove != null)
            OnMove.Invoke(pkt);
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

    // ============================================================
    // [COMBAT HANDLERS]
    // ============================================================
    // ============================================================
    // [MAP CHANGE HANDLERS]
    // ============================================================

    public static void S_MAP_CHANGE_BEGINHandler(ServerSession session, IMessage packet)
    {
        var pkt = packet as S_MAP_CHANGE_BEGIN;

        Debug.Log($"🗺️ [MapChange BEGIN] token={pkt.Token} targetMapId={pkt.TargetMapId} targetCh={pkt.TargetChannelId} " +
                  $"spawn=({pkt.Spawn.X},{pkt.Spawn.Y},{pkt.Spawn.Z})");

        // ✅ 상태 변경은 MapSceneRouter가 한다. 여기서는 이벤트만.
        OnMapChangeBegin?.Invoke(pkt);
    }

    public static void S_MAP_CHANGE_ENDHandler(ServerSession session, IMessage packet)
    {
        var pkt = packet as S_MAP_CHANGE_END;

        Debug.Log($"🗺️ [MapChange END] token={pkt.Token} mapId={pkt.MapId} targetCh={pkt.TargetChannelId} " +
                  $"pos=({pkt.Pos.X},{pkt.Pos.Y},{pkt.Pos.Z})");

        // ✅ 상태 변경은 MapSceneRouter가 한다. 여기서는 이벤트만.
        OnMapChangeEnd?.Invoke(pkt);
    }

    public static void S_SKILLHandler(ServerSession session, IMessage packet)
    {
        S_SKILL pkt = packet as S_SKILL;
        Debug.Log($"[Skill] Object {pkt.ObjectId} used Skill {pkt.SkillId}");

        if (OnSkill != null)
            OnSkill.Invoke(pkt);
    }

    public static void S_CHANGE_HPHandler(ServerSession session, IMessage packet)
    {
        S_CHANGE_HP pkt = packet as S_CHANGE_HP;
        Debug.Log($"🩸 [Hit] Target: {pkt.ObjectId} Dmg: {pkt.Damage} HP: {pkt.CurrentHp}");

        if (OnChangeHp != null)
            OnChangeHp.Invoke(pkt);
    }

    // ============================================================
    // [CHAT HANDLERS]
    // ============================================================

    public static void S_CHAT_RESHandler(ServerSession session, IMessage packet)
    {
        S_CHAT_RES pkt = packet as S_CHAT_RES;
        if (OnChatRes != null)
            OnChatRes.Invoke(pkt.Success);
    }

    public static void S_CHAT_NTFHandler(ServerSession session, IMessage packet)
    {
        S_CHAT_NTF pkt = packet as S_CHAT_NTF;
        if (OnChatMsg != null)
            OnChatMsg.Invoke(pkt.Message);
    }

    public static void S_HEART_BEAT_RESHandler(ServerSession session, IMessage packet)
    {
        C_HEART_BEAT_REQ pongPacket = new C_HEART_BEAT_REQ();

        // 2. NetworkManager를 통해 전송 (구조적 정답)
        // 이유: NetworkManager가 세션 null 체크와 로그를 관리하기 때문.
        // ID: PacketManager에 정의된 MsgId Enum을 캐스팅해서 사용 (하드코딩 금지)

        NetworkManager.Instance.Send(pongPacket, (ushort)PacketManager.MsgId.C_HEART_BEAT_REQ);
    }

    public static void S_PARTY_CHAT_NTFHandler(ServerSession session, IMessage packet)
    {
        var pkt = packet as S_PARTY_CHAT_NTF;
        if (pkt == null) return;

        Debug.Log($"[PartyChat] party={pkt.PartyId} {pkt.SenderName}({pkt.SenderId}): {pkt.Message}");
        OnPartyChatNtf?.Invoke(pkt);
    }

    public static void S_PARTY_INFO_NTFHandler(ServerSession session, IMessage packet)
    {
        var pkt = packet as S_PARTY_INFO_NTF;
        if (pkt == null) return;

        Debug.Log($"[PartyInfo] hasParty={pkt.HasParty} partyId={pkt.PartyId} leader={pkt.LeaderId} members={pkt.MemberIds.Count} ver={pkt.Version}");
        OnPartyInfoNtf?.Invoke(pkt);

        if (pkt.HasParty)
            PartyApi.RequestStatus();
    }

    public static void S_PARTY_RESULTHandler(ServerSession session, IMessage packet)
    {
        var pkt = packet as S_PARTY_RESULT;
        if (pkt == null) return;

        Debug.Log($"[PartyResult] op={pkt.Op} success={pkt.Success} reason={pkt.Reason} partyId={pkt.PartyId} ver={pkt.Version}");
        OnPartyResult?.Invoke(pkt);
    }

    public static void S_PARTY_INVITE_NTFHandler(ServerSession session, IMessage packet)
    {
        var pkt = packet as S_PARTY_INVITE_NTF;
        if (pkt == null) return;

        Debug.Log($"[PartyInvite] partyId={pkt.PartyId} inviter={pkt.InviterName}({pkt.InviterId})");
        OnPartyInviteNtf?.Invoke(pkt);
    }

    public static void S_PARTY_STATUS_NTFHandler(ServerSession session, IMessage packet)
    {
        var pkt = packet as S_PARTY_STATUS_NTF;
        if (pkt == null) return;

        Debug.Log($"[PartyStatus] partyId={pkt.PartyId} ver={pkt.Version} members={pkt.Members.Count}");
        OnPartyStatusNtf?.Invoke(pkt);
    }
    public static void S_DUNGEON_ENTER_RESHandler(ServerSession session, IMessage packet)
    {
        var pkt = packet as S_DUNGEON_ENTER_RES;
        if (pkt == null) return;

        Debug.Log($"[DungeonEnterRes] success={pkt.Success} map={pkt.DungeonMapId} inst={pkt.Instanceid} reason={pkt.Reason}");

        OnDungeonEnterRes?.Invoke(pkt);
    }

    public static void S_DUNGEON_EXIT_RESHandler(ServerSession session, IMessage packet)
    {
        var pkt = packet as S_DUNGEON_EXIT_RES;
        if (pkt == null) return;

        Debug.Log($"[DungeonExitRes] success={pkt.Success} returnMap={pkt.ReturnMapId} returnInst={pkt.ReturnInstanceid} reason={pkt.Reason}");

        OnDungeonExitRes?.Invoke(pkt);
    }
    public static void S_QUICKSLOT_LISTHandler(ServerSession session, IMessage packet)
    {
        var pkt = packet as S_QUICKSLOT_LIST;
        if (pkt == null) return;

        Debug.Log($"[QuickSlot] Loaded slots={pkt.Slots.Count}");
        OnQuickSlotList?.Invoke(pkt.Slots);
    }

    public static void S_SET_QUICKSLOTHandler(ServerSession session, IMessage packet)
    {
        var pkt = packet as S_SET_QUICKSLOT;
        if (pkt == null) return;

        Debug.Log($"[QuickSlot] Set slot success={pkt.Success} idx={pkt.Slot?.SlotIndex} type={pkt.Slot?.RefType} refId={pkt.Slot?.RefId}");
        if (pkt.Success && pkt.Slot != null)
            OnQuickSlotChanged?.Invoke(pkt.Slot);
    }


    // ============================================================
    // [TRADE HANDLERS]
    // ============================================================

    public static void S_TRADE_INVITEHandler(ServerSession session, IMessage packet)
    {
        var pkt = packet as S_TRADE_INVITE;
        if (pkt == null) return;

        Debug.Log($"[TradeInvite] from={pkt.FromPlayerId} name={pkt.FromName}");
        OnTradeInvite?.Invoke(pkt);
    }

    public static void S_TRADE_STARTHandler(ServerSession session, IMessage packet)
    {
        var pkt = packet as S_TRADE_START;
        if (pkt == null) return;

        Debug.Log($"[TradeStart] tradeId={pkt.TradeId} peer={pkt.PeerId} name={pkt.PeerName}");
        OnTradeStart?.Invoke(pkt);
    }

    public static void S_TRADE_OFFER_UPDATEHandler(ServerSession session, IMessage packet)
    {
        var pkt = packet as S_TRADE_OFFER_UPDATE;
        if (pkt == null) return;

        Debug.Log($"[TradeOfferUpdate] tradeId={pkt.TradeId} who={pkt.WhoPlayerId} items={pkt.Items.Count}");
        OnTradeOfferUpdate?.Invoke(pkt);
    }

    public static void S_TRADE_READY_STATEHandler(ServerSession session, IMessage packet)
    {
        var pkt = packet as S_TRADE_READY_STATE;
        if (pkt == null) return;

        Debug.Log($"[TradeReadyState] tradeId={pkt.TradeId} A={pkt.AReady} B={pkt.BReady}");
        OnTradeReadyState?.Invoke(pkt);
    }

    public static void S_TRADE_LOCKEDHandler(ServerSession session, IMessage packet)
    {
        var pkt = packet as S_TRADE_LOCKED;
        if (pkt == null) return;

        Debug.Log($"[TradeLocked] tradeId={pkt.TradeId}");
        OnTradeLocked?.Invoke(pkt);
    }

    public static void S_TRADE_CANCELLEDHandler(ServerSession session, IMessage packet)
    {
        var pkt = packet as S_TRADE_CANCELLED;
        if (pkt == null) return;

        Debug.Log($"[TradeCancelled] tradeId={pkt.TradeId} reason={pkt.Reason}");
        OnTradeCancelled?.Invoke(pkt);
    }

    public static void S_TRADE_RESULTHandler(ServerSession session, IMessage packet)
    {
        var pkt = packet as S_TRADE_RESULT;
        if (pkt == null) return;

        Debug.Log($"[TradeResult] tradeId={pkt.TradeId} success={pkt.Success} fail={pkt.FailCode} msg={pkt.Msg}");
        OnTradeResult?.Invoke(pkt);
    }

}


