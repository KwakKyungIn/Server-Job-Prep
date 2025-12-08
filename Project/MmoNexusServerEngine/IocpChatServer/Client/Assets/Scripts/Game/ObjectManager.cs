using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using Protocol;

public class ObjectManager : MonoBehaviour
{
    public static ObjectManager Instance { get; private set; }

    public static ulong MyPlayerId { get; set; }

    // [GIGACHAD Refactoring] 플레이어만 담는 게 아니라 몬스터도 담아야 함.
    // 이름도 _objects로 변경. (PlayerId, MonsterId 모두 ulong으로 통합 관리)
    Dictionary<ulong, GameObject> _objects = new Dictionary<ulong, GameObject>();

    // [Resources]
    public GameObject MyPlayerPrefab;
    public GameObject OtherPlayerPrefab;
    public GameObject MonsterPrefab; // [New] 몬스터용 프리팹 연결 필요

    void Awake()
    {
        Instance = this;
    }

    void Start()
    {
        PacketHandler.OnEnterGame += OnEnterGame;
        PacketHandler.OnSpawn += OnSpawn;
        PacketHandler.OnDespawn += OnDespawn;
        PacketHandler.OnMove += OnMove;
    }

    // 1. 내 캐릭터 입장
    void OnEnterGame(S_ENTER_GAME_RES pkt)
    {
        if (pkt.Success == false) return;

        MyPlayerId = pkt.MyPlayer.PlayerId;

        // 나를 생성
        Spawn(pkt.MyPlayer, true);
    }

    // 2. 다른 객체(플레이어 + 몬스터) 출현
    void OnSpawn(S_SPAWN pkt)
    {
        // 2-1. 플레이어 처리
        if (pkt.Players != null)
        {
            foreach (PlayerInfo player in pkt.Players)
            {
                if (player.PlayerId == MyPlayerId) continue;
                Spawn(player, false);
            }
        }

        // 2-2. [New] 몬스터 처리
        if (pkt.Monsters != null)
        {
            foreach (MonsterInfo monster in pkt.Monsters)
            {
                SpawnMonster(monster);
            }
        }
    }

    // 3. 객체 사라짐 (S_DESPAWN)
    void OnDespawn(S_DESPAWN pkt)
    {
        // [Modify] PacketHandler 수정에 맞춰 ObjectIds 사용
        foreach (ulong id in pkt.ObjectIds)
        {
            if (_objects.ContainsKey(id))
            {
                Destroy(_objects[id]);
                _objects.Remove(id);
            }
        }
    }

    // 4. 이동 패킷 처리 (S_MOVE)
    void OnMove(S_MOVE pkt)
    {
        // [Modify] PlayerId -> ObjectId
        if (pkt.ObjectId == MyPlayerId) return;

        // Dictionary(_objects)에서 찾기
        if (_objects.TryGetValue(pkt.ObjectId, out GameObject go))
        {
            // 플레이어든 몬스터든 PlayerController(혹은 BaseController)를 가지고 있다고 가정
            // 움직임 동기화 로직은 동일하니까.
            PlayerController pc = go.GetComponent<PlayerController>();
            if (pc != null)
            {
                pc.SetTargetPosition(new Vector3(pkt.PosInfo.X, pkt.PosInfo.Y, pkt.PosInfo.Z));
            }
        }
        else
        {
            // 시야 문제 등으로 인해 스폰 패킷보다 이동 패킷이 먼저 올 수도 있음 (UDP라면)
            // 하지만 TCP라면 순서 보장되므로 로직 에러일 가능성 높음
            // Debug.LogError($"[S_MOVE] Object ID {pkt.ObjectId} not found!");
        }
    }

    // [Helper] 플레이어 생성
    void Spawn(PlayerInfo info, bool isMine)
    {
        if (_objects.ContainsKey(info.PlayerId)) return;

        GameObject go = null;
        Vector3 pos = new Vector3(info.PosInfo.X, info.PosInfo.Y, info.PosInfo.Z);

        if (isMine)
        {
            go = Instantiate(MyPlayerPrefab, pos, Quaternion.identity);
            go.AddComponent<MyPlayerController>(); // 내 컨트롤러
        }
        else
        {
            go = Instantiate(OtherPlayerPrefab, pos, Quaternion.identity);
            go.AddComponent<PlayerController>(); // 타인 컨트롤러 (보간 이동)
        }

        go.name = $"Player_{info.PlayerId}_{info.Name}";
        _objects.Add(info.PlayerId, go);
    }

    // [Helper] 몬스터 생성 (New)
    void SpawnMonster(MonsterInfo info)
    {
        // Object ID 중복 체크
        if (_objects.ContainsKey(info.ObjectId)) return; // MonsterInfo에도 ObjectId 필드가 있어야 함 (Struct.proto 확인)

        Vector3 pos = new Vector3(info.PosInfo.X, info.PosInfo.Y, info.PosInfo.Z);

        // 몬스터 프리팹 생성
        GameObject go = Instantiate(MonsterPrefab, pos, Quaternion.identity);

        // 몬스터도 이동해야 하므로 PlayerController(이동 담당)를 붙여준다.
        // 나중에는 MonsterController를 따로 만드는 게 정석.
        if (go.GetComponent<PlayerController>() == null)
            go.AddComponent<PlayerController>();

        go.name = $"Monster_{info.TemplateId}_{info.ObjectId}";
        _objects.Add(info.ObjectId, go);
    }
}