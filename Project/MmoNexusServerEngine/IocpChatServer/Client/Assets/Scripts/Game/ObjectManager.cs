using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using Protocol;

public class ObjectManager : MonoBehaviour
{
    public static ObjectManager Instance { get; private set; }

    // [My Player ID] ulong으로 정확하게 선언
    public static ulong MyPlayerId { get; set; }

    // [Storage] Dictionary 키를 ulong으로 변경 (Protobuf PlayerId와 일치)
    Dictionary<ulong, GameObject> _players = new Dictionary<ulong, GameObject>();

    // [Resources] 유니티 에디터에서 연결할 프리팹
    public GameObject MyPlayerPrefab;
    public GameObject OtherPlayerPrefab;

    void Awake()
    {
        Instance = this;
    }

    void Start()
    {
        // [Event Subscription] 패킷 핸들러 이벤트 구독
        PacketHandler.OnEnterGame += OnEnterGame;
        PacketHandler.OnSpawn += OnSpawn;
        PacketHandler.OnDespawn += OnDespawn;
        PacketHandler.OnMove += OnMove;
    }

    // 1. 내 캐릭터 입장 (S_ENTER_GAME_RES)
    void OnEnterGame(S_ENTER_GAME_RES pkt)
    {
        if (pkt.Success == false) return;

        MyPlayerId = pkt.MyPlayer.PlayerId; // ulong으로 바로 받음

        // 나를 생성 (MyPlayerPrefab 사용)
        Spawn(pkt.MyPlayer, true);
    }

    // 2. 다른 플레이어 출현 (S_SPAWN)
    void OnSpawn(S_SPAWN pkt)
    {
        foreach (PlayerInfo player in pkt.Players)
        {
            if (player.PlayerId == MyPlayerId) continue; // ulong끼리 비교
            Spawn(player, false);
        }
    }

    // 3. 플레이어 사라짐 (S_DESPAWN)
    void OnDespawn(S_DESPAWN pkt)
    {
        // PlayerIds는 ulong 리스트이므로, 키도 ulong으로 찾는다.
        foreach (ulong id in pkt.PlayerIds)
        {
            if (_players.ContainsKey(id))
            {
                Destroy(_players[id]);
                _players.Remove(id);
            }
        }
    }

    // 4. 이동 패킷 처리 (S_MOVE)
    // ObjectManager.cs - OnMove 함수 내부
    void OnMove(S_MOVE pkt)
    {
        // 1. 받은 패킷의 ID와 내 ID가 같은가?
        Debug.Log($"[S_MOVE] Recv ID: {pkt.PlayerId}, My ID: {MyPlayerId}. Self? {pkt.PlayerId == MyPlayerId}");
        if (pkt.PlayerId == MyPlayerId) return;

        // 2. 이 ID가 Dictionary에 있는가?
        if (_players.TryGetValue(pkt.PlayerId, out GameObject go))
        {
            Debug.Log($"[S_MOVE] Found Player ID: {pkt.PlayerId}. Moving!");
            PlayerController pc = go.GetComponent<PlayerController>();
            if (pc != null)
            {
                pc.SetTargetPosition(new Vector3(pkt.PosInfo.X, pkt.PosInfo.Y, pkt.PosInfo.Z));
            }
        }
        else
        {
            // [CRITICAL] 여기에 찍혔다면 Spawn 자체가 안 됐다는 뜻이다.
            Debug.LogError($"[S_MOVE] ERROR: Player ID {pkt.PlayerId} not found in ObjectManager!");
        }
    }

    // [Internal Spawn Logic]
    void Spawn(PlayerInfo info, bool isMine)
    {
        // [FIX] ulong 키로 ContainsKey 체크
        if (_players.ContainsKey(info.PlayerId)) return;

        GameObject go = null;
        Vector3 pos = new Vector3(info.PosInfo.X, info.PosInfo.Y, info.PosInfo.Z);

        if (isMine)
        {
            go = Instantiate(MyPlayerPrefab, pos, Quaternion.identity);
            go.AddComponent<MyPlayerController>();
        }
        else
        {
            go = Instantiate(OtherPlayerPrefab, pos, Quaternion.identity);
            go.AddComponent<PlayerController>();
        }

        go.name = $"Player_{info.PlayerId}_{info.Name}";
        // [FIX] ulong 키로 Add
        _players.Add(info.PlayerId, go);
    }
}