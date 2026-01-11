using System.Collections;
using UnityEngine;
using UnityEngine.SceneManagement;
using Protocol;

public class MapSceneRouter : MonoBehaviour
{
    UI_LoadingOverlay _overlay;

    void Awake()
    {
        DontDestroyOnLoad(gameObject);
    }

    void OnEnable()
    {
        PacketHandler.OnMapChangeBegin += OnMapChangeBegin;
        PacketHandler.OnMapChangeEnd += OnMapChangeEnd;

        SceneManager.sceneLoaded += OnSceneLoaded;
    }

    void OnDisable()
    {
        PacketHandler.OnMapChangeBegin -= OnMapChangeBegin;
        PacketHandler.OnMapChangeEnd -= OnMapChangeEnd;

        SceneManager.sceneLoaded -= OnSceneLoaded;
    }

    void OnMapChangeBegin(S_MAP_CHANGE_BEGIN pkt)
    {
        // 1) 상태 플래그
        NetworkManager.Instance.BeginMapChange(pkt.Token);

        // 2) 로딩 UI ON
        EnsureOverlay();
        _overlay?.Show($"Loading Map {pkt.TargetMapId} (Ch {pkt.TargetChannelId})...");


        // 3) 내 플레이어를 씬 전환에서 보호 (안 하면 씬 로드때 파괴됨)
        PreserveMyPlayer();

        // 4) targetMapId -> 씬 이름
        string sceneName = GetSceneNameByMapId(pkt.TargetMapId);

        // 5) 씬 로드 시작 (ACK는 로드 완료 후)
        StartCoroutine(CoLoadSceneAndAck(sceneName, pkt.Token));
    }

    IEnumerator CoLoadSceneAndAck(string sceneName, ulong token)
    {
        // 씬 로드 (Single로 갈아끼우는 전제)
        AsyncOperation op = SceneManager.LoadSceneAsync(sceneName, LoadSceneMode.Single);
        if (op == null)
        {
            Debug.LogError($"[MapSceneRouter] LoadSceneAsync failed. sceneName={sceneName}");
            yield break;
        }

        while (!op.isDone)
            yield return null;

        // 씬 오브젝트 Awake/Start 안정화 1프레임 양보
        yield return null;

        // ✅ 로딩 끝났으니 이제 ACK
        var ack = new C_MAP_CHANGE_ACK { Token = token };
        NetworkManager.Instance.Send(ack, (ushort)PacketManager.MsgId.C_MAP_CHANGE_ACK);
        Debug.Log($"✅ [MapChange ACK Sent AFTER SceneLoad] token={token}");
    }

    void OnMapChangeEnd(S_MAP_CHANGE_END pkt)
    {
        // END는 ObjectManager가 워프 처리하든, 여기서 하든 선택인데
        // 네 구조(씬에 ObjectManager) 기준으로는 ObjectManager가 처리하는게 자연스러움.
        // 여기서는 로딩 UI만 내린다.
        EnsureOverlay();
        _overlay?.Hide();

        NetworkManager.Instance.EndMapChange(pkt.Token);
    }

    void OnSceneLoaded(Scene scene, LoadSceneMode mode)
    {
        // 새 씬에서 overlay 다시 찾기
        EnsureOverlay();

        // 씬 바뀌면 카메라가 새로 생김 -> 내 플레이어 카메라 재타겟 필요
        // (ObjectManager가 Awake에서 잡아도 되고, 여기서 한번 더 보정)
        TryRetargetCameraToMyPlayer();
    }

    void PreserveMyPlayer()
    {
        var my = GameObject.FindWithTag("MyPlayer");
        if (my != null)
            DontDestroyOnLoad(my);
    }

    void TryRetargetCameraToMyPlayer()
    {
        var my = GameObject.FindWithTag("MyPlayer");
        if (my == null) return;

        var cam = Camera.main;
        if (cam == null) return;

        var follow = cam.GetComponent<FollowCamera>();
        if (follow != null)
            follow.target = my.transform;
    }

    void EnsureOverlay()
    {
        if (_overlay != null) return;
        _overlay = FindObjectOfType<UI_LoadingOverlay>(true);
    }

    string GetSceneNameByMapId(int mapId)
    {
        // 너가 원한 규칙: 03_Game_0001, 03_Game_0002 ...
        return $"03_Game_{mapId:D4}";
    }
}
