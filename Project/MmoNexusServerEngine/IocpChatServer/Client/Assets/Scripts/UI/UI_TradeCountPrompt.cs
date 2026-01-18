using System;
using TMPro;
using UnityEngine;
using UnityEngine.EventSystems;
using UnityEngine.UI;

/// <summary>
/// Runtime-created simple popup for choosing a trade offer count.
/// - No prefab required.
/// - Creates UI under the first active Canvas it can find.
/// </summary>
public class UI_TradeCountPrompt : MonoBehaviour
{
    static UI_TradeCountPrompt _instance;

    CanvasGroup _cg;
    TMP_Text _title;
    TMP_InputField _input;
    Button _btnOk;
    Button _btnCancel;

    int _max = 1;
    Action<int> _onConfirm;

    public static void Show(int max, Action<int> onConfirm)
    {
        if (max <= 1)
        {
            onConfirm?.Invoke(1);
            return;
        }

        EnsureInstance();
        _instance.InternalShow(max, onConfirm);
    }

    static void EnsureInstance()
    {
        if (_instance != null) return;

        Canvas canvas = GameObject.FindObjectOfType<Canvas>();
        if (canvas == null)
        {
            // As a last resort, create a canvas so the popup still works.
            var cgo = new GameObject("UI_RuntimeCanvas", typeof(Canvas), typeof(CanvasScaler), typeof(GraphicRaycaster));
            canvas = cgo.GetComponent<Canvas>();
            canvas.renderMode = RenderMode.ScreenSpaceOverlay;
            var scaler = cgo.GetComponent<CanvasScaler>();
            scaler.uiScaleMode = CanvasScaler.ScaleMode.ScaleWithScreenSize;
            scaler.referenceResolution = new Vector2(1920, 1080);
        }

        var go = new GameObject("UI_TradeCountPrompt", typeof(RectTransform));
        go.transform.SetParent(canvas.transform, false);
        _instance = go.AddComponent<UI_TradeCountPrompt>();
        _instance.BuildUi(go.GetComponent<RectTransform>());
        _instance.HideImmediate();
    }

    void BuildUi(RectTransform root)
    {
        root.anchorMin = Vector2.zero;
        root.anchorMax = Vector2.one;
        root.offsetMin = Vector2.zero;
        root.offsetMax = Vector2.zero;

        _cg = gameObject.AddComponent<CanvasGroup>();

        // Dim background
        var bg = new GameObject("Bg", typeof(RectTransform), typeof(Image));
        bg.transform.SetParent(root, false);
        var bgRt = bg.GetComponent<RectTransform>();
        bgRt.anchorMin = Vector2.zero;
        bgRt.anchorMax = Vector2.one;
        bgRt.offsetMin = Vector2.zero;
        bgRt.offsetMax = Vector2.zero;
        bg.GetComponent<Image>().color = new Color(0, 0, 0, 0.6f);

        // Panel
        var panel = new GameObject("Panel", typeof(RectTransform), typeof(Image));
        panel.transform.SetParent(root, false);
        var panelRt = panel.GetComponent<RectTransform>();
        panelRt.anchorMin = new Vector2(0.5f, 0.5f);
        panelRt.anchorMax = new Vector2(0.5f, 0.5f);
        panelRt.sizeDelta = new Vector2(420, 220);
        panelRt.anchoredPosition = Vector2.zero;
        panel.GetComponent<Image>().color = new Color(0.12f, 0.12f, 0.12f, 0.95f);

        var v = panel.AddComponent<VerticalLayoutGroup>();
        v.padding = new RectOffset(18, 18, 18, 18);
        v.spacing = 12;
        v.childAlignment = TextAnchor.UpperCenter;
        v.childControlHeight = true;
        v.childControlWidth = true;
        v.childForceExpandHeight = false;
        v.childForceExpandWidth = true;

        panel.AddComponent<ContentSizeFitter>().verticalFit = ContentSizeFitter.FitMode.PreferredSize;

        _title = CreateText(panel.transform, "How many?", 20);

        // Input row
        var inputGo = new GameObject("Input", typeof(RectTransform), typeof(Image), typeof(TMP_InputField));
        inputGo.transform.SetParent(panel.transform, false);
        var inputImg = inputGo.GetComponent<Image>();
        inputImg.color = new Color(0.2f, 0.2f, 0.2f, 1f);
        var inputRt = inputGo.GetComponent<RectTransform>();
        inputRt.sizeDelta = new Vector2(0, 48);

        var textArea = new GameObject("TextArea", typeof(RectTransform));
        textArea.transform.SetParent(inputGo.transform, false);
        var taRt = textArea.GetComponent<RectTransform>();
        taRt.anchorMin = Vector2.zero;
        taRt.anchorMax = Vector2.one;
        taRt.offsetMin = new Vector2(12, 8);
        taRt.offsetMax = new Vector2(-12, -8);

        var placeholder = CreateText(textArea.transform, "1", 18);
        placeholder.color = new Color(0.7f, 0.7f, 0.7f, 0.7f);

        var text = CreateText(textArea.transform, "", 18);
        text.color = Color.white;

        _input = inputGo.GetComponent<TMP_InputField>();
        _input.textViewport = taRt;
        _input.textComponent = text;
        _input.placeholder = placeholder;
        _input.contentType = TMP_InputField.ContentType.IntegerNumber;
        _input.characterLimit = 9;

        // Buttons row
        var btnRow = new GameObject("Buttons", typeof(RectTransform));
        btnRow.transform.SetParent(panel.transform, false);
        var h = btnRow.AddComponent<HorizontalLayoutGroup>();
        h.spacing = 12;
        h.childAlignment = TextAnchor.MiddleCenter;
        h.childControlWidth = true;
        h.childControlHeight = true;
        h.childForceExpandWidth = true;
        h.childForceExpandHeight = false;
        btnRow.GetComponent<RectTransform>().sizeDelta = new Vector2(0, 48);

        _btnOk = CreateButton(btnRow.transform, "OK");
        _btnCancel = CreateButton(btnRow.transform, "Cancel");

        _btnOk.onClick.AddListener(OnClickOk);
        _btnCancel.onClick.AddListener(OnClickCancel);
    }

    TMP_Text CreateText(Transform parent, string txt, int fontSize)
    {
        var go = new GameObject("Text", typeof(RectTransform), typeof(TMP_Text));
        go.transform.SetParent(parent, false);
        var t = go.GetComponent<TMP_Text>();
        t.text = txt;
        t.fontSize = fontSize;
        t.alignment = TextAlignmentOptions.Center;
        t.color = Color.white;
        t.enableWordWrapping = false;
        var rt = go.GetComponent<RectTransform>();
        rt.sizeDelta = new Vector2(0, fontSize + 12);
        return t;
    }

    Button CreateButton(Transform parent, string label)
    {
        var go = new GameObject($"Btn_{label}", typeof(RectTransform), typeof(Image), typeof(Button));
        go.transform.SetParent(parent, false);
        var img = go.GetComponent<Image>();
        img.color = new Color(0.25f, 0.25f, 0.25f, 1f);
        var btn = go.GetComponent<Button>();

        var t = CreateText(go.transform, label, 18);
        t.rectTransform.anchorMin = Vector2.zero;
        t.rectTransform.anchorMax = Vector2.one;
        t.rectTransform.offsetMin = Vector2.zero;
        t.rectTransform.offsetMax = Vector2.zero;

        go.GetComponent<RectTransform>().sizeDelta = new Vector2(0, 44);
        return btn;
    }

    void InternalShow(int max, Action<int> onConfirm)
    {
        _max = Mathf.Max(1, max);
        _onConfirm = onConfirm;

        _title.text = $"Offer count (1 ~ {_max})";
        _input.text = _max.ToString();

        _cg.alpha = 1f;
        _cg.blocksRaycasts = true;
        _cg.interactable = true;

        // Ensure an EventSystem exists.
        if (EventSystem.current == null)
            new GameObject("EventSystem", typeof(EventSystem), typeof(StandaloneInputModule));

        _input.Select();
        _input.ActivateInputField();
    }

    void HideImmediate()
    {
        if (_cg == null) return;
        _cg.alpha = 0f;
        _cg.blocksRaycasts = false;
        _cg.interactable = false;
    }

    void Hide()
    {
        _onConfirm = null;
        HideImmediate();
    }

    void OnClickOk()
    {
        int v = 1;
        if (!int.TryParse(_input.text, out v)) v = 1;
        v = Mathf.Clamp(v, 1, _max);

        var cb = _onConfirm;
        Hide();
        cb?.Invoke(v);
    }

    void OnClickCancel()
    {
        Hide();
    }
}
