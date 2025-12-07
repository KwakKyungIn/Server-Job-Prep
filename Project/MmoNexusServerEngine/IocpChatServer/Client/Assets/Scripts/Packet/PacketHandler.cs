using Google.Protobuf;
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