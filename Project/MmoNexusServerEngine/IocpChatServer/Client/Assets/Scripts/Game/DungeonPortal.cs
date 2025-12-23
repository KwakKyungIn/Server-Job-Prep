using UnityEngine;
using Protocol;

[RequireComponent(typeof(Collider))]
public class DungeonPortal : MonoBehaviour
{
    [Header("Dungeon Target")]
    public int dungeonMapId = 1001;

    [Header("Options")]
    public string myPlayerTag = "MyPlayer";
    public bool onlyMyPlayer = true;

    void Reset()
    {
        // Collider isTrigger 자동 세팅
        var col = GetComponent<Collider>();
        if (col != null) col.isTrigger = true;
    }

    void OnTriggerEnter(Collider other)
    {
        if (NetworkManager.Instance == null) return;

        // 맵 체인지 중이면 무시
        if (NetworkManager.Instance.IsMapChanging)
            return;

        if (onlyMyPlayer)
        {
            if (!other.CompareTag(myPlayerTag))
                return;
        }

        // 던전 입장 요청
        C_DUNGEON_ENTER_REQ req = new C_DUNGEON_ENTER_REQ();
        req.DungeonMapId = dungeonMapId;

        NetworkManager.Instance.Send(req, (ushort)PacketManager.MsgId.C_DUNGEON_ENTER_REQ);
        Debug.Log($"?? [Portal] Sent C_DUNGEON_ENTER_REQ dungeonMapId={dungeonMapId}");
    }
}
