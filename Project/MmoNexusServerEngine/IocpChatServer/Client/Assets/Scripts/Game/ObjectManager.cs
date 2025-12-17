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

    List<MonsterInfo> _pendingMonsters = new List<MonsterInfo>();



    void Awake()
    {
        Instance = this;
        Debug.Log("🔧 [ObjectManager] Awake - Instance created");
    }

    void Start()
    {
        Debug.Log("🔧 [ObjectManager] Start - Registering events...");

        PacketHandler.OnEnterGame += OnEnterGame;
        PacketHandler.OnSpawn += OnSpawn;
        PacketHandler.OnDespawn += OnDespawn;
        PacketHandler.OnMove += OnMove;

        PacketHandler.OnMapChangeBegin += OnMapChangeBegin;
        PacketHandler.OnMapChangeEnd += OnMapChangeEnd;


        Debug.Log("✅ [ObjectManager] Events registered successfully");
    }

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
        Debug.Log($"📥📥📥 [OnSpawn] ===== ENTRY POINT ===== ");
        Debug.Log($"    Players={pkt.Players?.Count ?? 0}, Monsters={pkt.Monsters?.Count ?? 0}");
        Debug.Log($"    MyPlayerId = {MyPlayerId}");

        if (pkt.Players != null)
        {
            Debug.Log($"    Processing {pkt.Players.Count} players...");
            foreach (PlayerInfo player in pkt.Players)
            {
                Debug.Log($"      🔍 Player {player.PlayerId} (Name: {player.Name})");
                Debug.Log($"         Is this me? {player.PlayerId == MyPlayerId}");

                if (player.PlayerId == MyPlayerId)
                {
                    Debug.Log($"         ⏭️ SKIPPING (Self)");
                    continue;
                }

                Debug.Log($"         ✅ SPAWNING Other Player {player.PlayerId}");
                Spawn(player, false);
            }
        }
        else
        {
            Debug.Log("    ⚠️ pkt.Players is NULL!");
        }

        if (pkt.Monsters != null)
        {
            Debug.Log($"    🔍 Processing {pkt.Monsters.Count} monsters...");
            foreach (MonsterInfo monster in pkt.Monsters)
            {
                Debug.Log($"      🔍 Monster ID={monster.ObjectId}, TemplateId={monster.TemplateId}");

                if (MyPlayerId == 0)
                {
                    _pendingMonsters.Add(monster);
                    Debug.LogWarning("      ⏸️ Monster packet arrived before MyPlayer. Pending...");
                }
                else
                {
                    SpawnMonster(monster);
                }
            }
        }

        Debug.Log($"📥📥📥 [OnSpawn] ===== COMPLETE ===== ");
    }

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
        Debug.Log($"📥 [OnDespawn] Removing {pkt.ObjectIds.Count} objects");

        foreach (ulong id in pkt.ObjectIds)
        {
            if (_objects.ContainsKey(id))
            {
                Debug.Log($"    🗑️ Destroying object {id}");
                Destroy(_objects[id]);
                _objects.Remove(id);
            }
            else
            {
                Debug.LogWarning($"    ⚠️ Object {id} not found in dictionary");
            }
        }
    }


    void OnMapChangeBegin(S_MAP_CHANGE_BEGIN pkt)
    {
        Debug.Log($"[ObjectManager] OnMapChangeBegin token={pkt.Token} targetMapId={pkt.TargetMapId}");

        // 1) 내 플레이어는 유지 (END 패킷에 MyPlayerInfo가 없어서 재스폰 불가)
        // 2) 나머지 오브젝트만 전부 제거
        List<ulong> removeIds = new List<ulong>();

        foreach (var kv in _objects)
        {
            ulong id = kv.Key;
            if (id == MyPlayerId) continue;
            removeIds.Add(id);
        }

        foreach (ulong id in removeIds)
        {
            if (_objects.TryGetValue(id, out GameObject go))
                Destroy(go);
            _objects.Remove(id);
        }

        _pendingMonsters.Clear();

        Debug.Log($"[ObjectManager] Cleared world except MyPlayer. Remaining={_objects.Count}");
    }

    void OnMapChangeEnd(S_MAP_CHANGE_END pkt)
    {
        Debug.Log($"[ObjectManager] OnMapChangeEnd token={pkt.Token} mapId={pkt.MapId}");

        if (_objects.TryGetValue(MyPlayerId, out GameObject myGo) == false || myGo == null)
        {
            Debug.LogError($"[ObjectManager] MyPlayer object not found! MyPlayerId={MyPlayerId}. (BEGIN에서 MyPlayer까지 지웠으면 이거 터짐)");
            return;
        }

        Vector3 pos = new Vector3(pkt.Pos.X, pkt.Pos.Y, pkt.Pos.Z);
        myGo.transform.position = pos;

        Debug.Log($"[ObjectManager] MyPlayer warped to {pos}");
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
        if (_objects.ContainsKey(info.PlayerId))
        {
            Debug.LogWarning($"⚠️ [Spawn] Player {info.PlayerId} already exists! Skipping.");
            return;
        }

        GameObject go = null;
        Vector3 pos = new Vector3(info.PosInfo.X, info.PosInfo.Y, info.PosInfo.Z);

        if (isMine)
        {
            Debug.Log($"👤 [Spawn] Creating MY Player {info.PlayerId} at {pos}");
            go = Instantiate(MyPlayerPrefab, pos, Quaternion.identity);
            go.AddComponent<MyPlayerController>();

            go.tag = "MyPlayer";

            FollowCamera cam = Camera.main.GetComponent<FollowCamera>();
            if (cam != null)
                cam.target = go.transform;
        }
        else
        {
            Debug.Log($"👥 [Spawn] Creating OTHER Player {info.PlayerId} at {pos}");
            go = Instantiate(OtherPlayerPrefab, pos, Quaternion.identity);
            go.AddComponent<PlayerController>();
        }

        go.name = $"Player_{info.PlayerId}_{info.Name}";
        _objects.Add(info.PlayerId, go);

        Debug.Log($"✅ [Spawn] Player {info.PlayerId} spawned successfully. Total objects: {_objects.Count}");
    }

    void SpawnMonster(MonsterInfo info)
    {
        if (_objects.ContainsKey(info.ObjectId))
        {
            Debug.LogError($"❌ [Spawn Fail] Monster ID {info.ObjectId} already exists! Skipping.");
            return;
        }

        Vector3 pos = new Vector3(info.PosInfo.X, info.PosInfo.Y, info.PosInfo.Z);

        if (MonsterPrefab == null)
        {
            Debug.LogError("❌ [Spawn Fail] MonsterPrefab is NULL! Assign Prefab in Inspector.");
            return;
        }

        GameObject go = Instantiate(MonsterPrefab, pos, Quaternion.identity);

        if (go == null)
        {
            Debug.LogError("❌ [Spawn Fail] Instantiate failed! Prefab might be corrupted.");
            return;
        }

        if (go.GetComponent<PlayerController>() == null)
            go.AddComponent<PlayerController>();

        go.name = $"Monster_{info.TemplateId}_{info.ObjectId}";
        _objects.Add(info.ObjectId, go);

        Debug.Log($"👾 [SUCCESS] Monster Spawned: {go.name} at {pos}");
    }
}