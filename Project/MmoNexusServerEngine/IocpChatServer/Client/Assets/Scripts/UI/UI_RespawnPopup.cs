using TMPro;
using UnityEngine;
using UnityEngine.UI;
using Protocol;

public class UI_RespawnPopup : MonoBehaviour
{
    public static UI_RespawnPopup Instance { get; private set; }

    [Header("Wiring (assign in Inspector)")]
    public GameObject root;
    public TMP_Text msgText;
    public Button btnConfirm;
    public Button btnCancel;

    void Awake()
    {
        if (Instance != null && Instance != this)
        {
            Destroy(gameObject);
            return;
        }
        Instance = this;

        if (root == null)
            root = gameObject;

        if (btnConfirm != null)
        {
            btnConfirm.onClick.RemoveAllListeners();
            btnConfirm.onClick.AddListener(OnConfirm);
        }

        if (btnCancel != null)
        {
            btnCancel.onClick.RemoveAllListeners();
            btnCancel.onClick.AddListener(OnCancel);
        }

        InternalHide();
    }

    void OnDestroy()
    {
        if (Instance == this)
            Instance = null;
    }

    public static void Show(string message = "근처 월드에서 부활하시겠습니까?")
    {
        if (Instance == null)
        {
            Debug.LogError("[UI_RespawnPopup] Instance not found. Put UI_RespawnPopup in scene and wire references.");
            return;
        }

        Instance.InternalShow(message);
    }

    public static void Hide()
    {
        if (Instance == null) return;
        Instance.InternalHide();
    }

    void InternalShow(string message)
    {
        if (msgText != null)
            msgText.text = message;

        if (root != null)
            root.SetActive(true);
    }

    void InternalHide()
    {
        if (root != null)
            root.SetActive(false);
    }

    void OnConfirm()
    {
        InternalHide();
        SendRespawn();
    }

    void OnCancel()
    {
        InternalHide();
    }

    void SendRespawn()
    {
        if (NetworkManager.Instance == null)
        {
            Debug.LogWarning("[UI_RespawnPopup] NetworkManager not ready.");
            return;
        }

        var pkt = new C_RESPAWN_REQ();
        NetworkManager.Instance.Send(pkt, (ushort)PacketManager.MsgId.C_RESPAWN_REQ);
    }
}
