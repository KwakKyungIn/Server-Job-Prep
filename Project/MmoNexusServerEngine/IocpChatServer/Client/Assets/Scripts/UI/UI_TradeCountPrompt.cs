using System;
using TMPro;
using UnityEngine;
using UnityEngine.EventSystems;
using UnityEngine.UI;

public class UI_TradeCountPrompt : MonoBehaviour
{
    public static UI_TradeCountPrompt Instance { get; private set; }

    [Header("Wiring (assign in Inspector)")]
    [SerializeField] private CanvasGroup cg;
    [SerializeField] private TextMeshProUGUI titleText;
    [SerializeField] private TMP_InputField input;
    [SerializeField] private Button btnOk;
    [SerializeField] private Button btnCancel;

    private int max = 1;
    private Action<int> onConfirm;

    void Awake()
    {
        // Singleton: 씬에 하나만 둔다
        if (Instance != null && Instance != this)
        {
            Destroy(gameObject);
            return;
        }
        Instance = this;

        // 버튼 연결
        if (btnOk != null) btnOk.onClick.AddListener(OnClickOk);
        if (btnCancel != null) btnCancel.onClick.AddListener(OnClickCancel);

        HideImmediate();
    }

    private bool ValidateWiring()
    {
        if (cg == null || titleText == null || input == null || btnOk == null || btnCancel == null)
        {
            Debug.LogError("[UI_TradeCountPrompt] Wiring missing. Assign cg/titleText/input/btnOk/btnCancel in Inspector.");
            return false;
        }
        return true;
    }

    public static void Show(int max, Action<int> onConfirm)
    {
        if (max <= 1)
        {
            onConfirm?.Invoke(1);
            return;
        }

        if (Instance == null)
        {
            Debug.LogError("[UI_TradeCountPrompt] Instance not found. Put UI_TradeCountPrompt object in scene and assign references.");
            return;
        }

        Instance.InternalShow(max, onConfirm);
    }

    private void InternalShow(int maxValue, Action<int> cb)
    {
        if (!ValidateWiring()) return;

        max = Mathf.Max(1, maxValue);
        onConfirm = cb;

        titleText.text = $"Offer count (1 ~ {max})";
        input.contentType = TMP_InputField.ContentType.IntegerNumber;
        input.characterLimit = 9;
        input.text = max.ToString();

        cg.alpha = 1f;
        cg.blocksRaycasts = true;
        cg.interactable = true;

        // EventSystem 보장
        if (EventSystem.current == null)
            new GameObject("EventSystem", typeof(EventSystem), typeof(StandaloneInputModule));

        input.Select();
        input.ActivateInputField();
    }

    private void HideImmediate()
    {
        if (cg == null) return;
        cg.alpha = 0f;
        cg.blocksRaycasts = false;
        cg.interactable = false;
    }

    private void Hide()
    {
        onConfirm = null;
        HideImmediate();
    }

    private void OnClickOk()
    {
        if (!ValidateWiring()) return;

        int v = 1;
        if (!int.TryParse(input.text, out v)) v = 1;
        v = Mathf.Clamp(v, 1, max);

        var cb = onConfirm;
        Hide();
        cb?.Invoke(v);
    }

    private void OnClickCancel()
    {
        Hide();
    }
}
