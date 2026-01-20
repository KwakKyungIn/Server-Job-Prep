using UnityEngine;
using UnityEngine.EventSystems;
using Protocol;

[RequireComponent(typeof(Collider))]
public class DungeonPortal : MonoBehaviour
{
    [Header("Dungeon Target")]
    public int dungeonMapId = 1001;

    [Header("Options")]
    public string myPlayerTag = "MyPlayer";
    public bool onlyMyPlayer = true;

    [Header("Right Click Trigger")]
    public float maxClickDistance = 200f;
    public bool requireMouseOverPortal = true;

    private bool _inRange = false;
    private Collider _portalCollider;

    void Awake()
    {
        _portalCollider = GetComponent<Collider>();
    }

    void Reset()
    {
        // Collider isTrigger auto setup
        var col = GetComponent<Collider>();
        if (col != null) col.isTrigger = true;
    }

    void OnTriggerEnter(Collider other)
    {
        if (onlyMyPlayer && !other.CompareTag(myPlayerTag))
            return;

        _inRange = true;
    }

    void OnTriggerExit(Collider other)
    {
        if (onlyMyPlayer && !other.CompareTag(myPlayerTag))
            return;

        _inRange = false;
    }

    void Update()
    {
        if (!_inRange)
            return;

        if (NetworkManager.Instance == null)
            return;

        // ignore while map changing
        if (NetworkManager.Instance.IsMapChanging)
            return;

        // right click
        if (!Input.GetMouseButtonDown(1))
            return;

        // ignore when cursor is on UI
        if (EventSystem.current != null && EventSystem.current.IsPointerOverGameObject())
            return;

        if (requireMouseOverPortal)
        {
            Camera cam = Camera.main;
            if (cam == null)
                return;

            Ray ray = cam.ScreenPointToRay(Input.mousePosition);
            if (!Physics.Raycast(ray, out RaycastHit hit, maxClickDistance, ~0, QueryTriggerInteraction.Collide))
                return;

            // accept self collider or children collider
            if (hit.collider != _portalCollider && !hit.collider.transform.IsChildOf(transform))
                return;
        }

        // send dungeon enter request
        C_DUNGEON_ENTER_REQ req = new C_DUNGEON_ENTER_REQ();
        req.DungeonMapId = dungeonMapId;

        NetworkManager.Instance.Send(req, (ushort)PacketManager.MsgId.C_DUNGEON_ENTER_REQ);
        Debug.Log($"[Portal] RightClick -> Sent C_DUNGEON_ENTER_REQ dungeonMapId={dungeonMapId}");
    }
}
