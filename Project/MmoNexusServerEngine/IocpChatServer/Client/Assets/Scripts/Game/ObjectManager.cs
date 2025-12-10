using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using Protocol;

public class ObjectManager : MonoBehaviour
{
    public static ObjectManager Instance { get; private set; }
    public static ulong MyPlayerId { get; set; }

    Dictionary<ulong, GameObject> _objects = new Dictionary<ulong, GameObject>();

    public GameObject MyPlayerPrefab;
    public GameObject OtherPlayerPrefab;
    public GameObject MonsterPrefab;

    // [GIGACHAD ADD] MyPlayer 생성 전에 도착한 몬스터를 임시 저장할 큐
    List<MonsterInfo> _pendingMonsters = new List<MonsterInfo>(); // [ADD]

    void Awake()
    {
        Instance = this;
    }

    void Start()
    {
        PacketHandler.OnEnterGame += OnEnterGame; // S_ENTER_GAME
        PacketHandler.OnSpawn += OnSpawn;         // S_SPAWN
        PacketHandler.OnDespawn += OnDespawn;     // S_DESPAWN
        PacketHandler.OnMove += OnMove;           // S_MOVE
    }

    // [Change] 패킷 타입 변경 (S_ENTER_GAME_RES -> S_ENTER_GAME)
    void OnEnterGame(S_ENTER_GAME pkt)
    {
        Debug.Log($"📥 [OnEnterGame] Called! Success={pkt.Success}");

        if (pkt.Success == false) return;

        MyPlayerId = pkt.MyPlayer.PlayerId;
        Debug.Log($"✅ MyPlayerId set to {MyPlayerId}");

        Spawn(pkt.MyPlayer, true);

        ProcessPendingSpawns();
    }

    void OnSpawn(S_SPAWN pkt)
    {
        Debug.Log($"📥 [OnSpawn] Called! Players={pkt.Players?.Count ?? 0}, Monsters={pkt.Monsters?.Count ?? 0}");

        if (pkt.Players != null)
        {
            foreach (PlayerInfo player in pkt.Players)
            {
                if (player.PlayerId == MyPlayerId) continue;
                Spawn(player, false);
            }
        }

        if (pkt.Monsters != null)
        {
            Debug.Log($"🔍 Processing {pkt.Monsters.Count} monsters...");
            foreach (MonsterInfo monster in pkt.Monsters)
            {
                Debug.Log($"🔍 Monster in packet: ID={monster.ObjectId}, TemplateId={monster.TemplateId}");

                if (MyPlayerId == 0)
                {
                    _pendingMonsters.Add(monster);
                    Debug.LogWarning("[ObjectManager] Monster packet arrived before MyPlayer. Pending...");
                }
                else
                {
                    SpawnMonster(monster);
                }
            }
        }
    }

    // [GIGACHAD ADD] 대기 중인 몬스터를 처리하는 전용 함수
    void ProcessPendingSpawns()
    {
        if (_pendingMonsters.Count > 0)
        {
            Debug.Log($"[ObjectManager] Processing {_pendingMonsters.Count} pending monsters now.");
            foreach (MonsterInfo monster in _pendingMonsters)
            {
                SpawnMonster(monster);
            }
            _pendingMonsters.Clear();
        }
    }


    void OnDespawn(S_DESPAWN pkt)
    {
        foreach (ulong id in pkt.ObjectIds)
        {
            if (_objects.ContainsKey(id))
            {
                Destroy(_objects[id]);
                _objects.Remove(id);
            }
        }
    }

    void OnMove(S_MOVE pkt)
    {
        if (pkt.ObjectId == MyPlayerId) return;

        if (_objects.TryGetValue(pkt.ObjectId, out GameObject go))
        {
            PlayerController pc = go.GetComponent<PlayerController>();
            if (pc != null)
            {
                pc.SetTargetPosition(new Vector3(pkt.PosInfo.X, pkt.PosInfo.Y, pkt.PosInfo.Z));
            }
        }
    }

    void Spawn(PlayerInfo info, bool isMine)
    {
        if (_objects.ContainsKey(info.PlayerId)) return;

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
        _objects.Add(info.PlayerId, go);
    }

    void SpawnMonster(MonsterInfo info)
    {
        // ID 충돌/중복 체크
        if (_objects.ContainsKey(info.ObjectId))
        {
            Debug.LogError($"[Spawn Fail] Monster ID {info.ObjectId} already exists! Skipping.");
            return;
        }

        Vector3 pos = new Vector3(info.PosInfo.X, info.PosInfo.Y, info.PosInfo.Z);

        // [CRITICAL CHECK 1] MonsterPrefab이 연결되었는지 확인
        if (MonsterPrefab == null)
        {
            Debug.LogError("[Spawn Fail] MonsterPrefab is NULL! Assign Prefab in Inspector.");
            return; // 프리팹이 없으면 여기서 멈춰야 함
        }

        // [CRITICAL CHECK 2] 실제 생성 시도
        GameObject go = Instantiate(MonsterPrefab, pos, Quaternion.identity);

        if (go == null)
        {
            Debug.LogError("[Spawn Fail] Instantiate failed! Prefab might be corrupted.");
            return;
        }

        if (go.GetComponent<PlayerController>() == null)
            go.AddComponent<PlayerController>();

        go.name = $"Monster_{info.TemplateId}_{info.ObjectId}";
        _objects.Add(info.ObjectId, go);

        // [FINAL DEBUG] 생성 성공 확인
        Debug.Log($"👾 [SUCCESS] Monster Spawned: {go.name} at {pos}");
    }
}