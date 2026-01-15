using UnityEngine;
using UnityEngine.UI;

/// <summary>
/// Runtime-created drag icon that follows the pointer.
/// </summary>
public class UI_DragGhost : MonoBehaviour
{
    static UI_DragGhost _inst;

    RectTransform _rt;
    Image _img;
    CanvasGroup _cg;

    public static void Begin(Canvas canvas, Sprite sprite, Vector2 screenPos)
    {
        End();

        if (canvas == null || sprite == null)
            return;

        GameObject go = new GameObject("UI_DragGhost");
        go.transform.SetParent(canvas.transform, worldPositionStays: false);

        _inst = go.AddComponent<UI_DragGhost>();
        _inst._rt = go.AddComponent<RectTransform>();
        _inst._img = go.AddComponent<Image>();
        _inst._cg = go.AddComponent<CanvasGroup>();

        _inst._cg.blocksRaycasts = false;
        _inst._cg.interactable = false;

        _inst._img.sprite = sprite;
        _inst._img.raycastTarget = false;
        _inst._img.preserveAspect = true;

        // reasonable default size
        _inst._rt.sizeDelta = new Vector2(64f, 64f);

        _inst.SetPos(screenPos);
    }

    public static void UpdatePos(Vector2 screenPos)
    {
        if (_inst == null) return;
        _inst.SetPos(screenPos);
    }

    public static void End()
    {
        if (_inst == null) return;
        Destroy(_inst.gameObject);
        _inst = null;
    }

    void SetPos(Vector2 screenPos)
    {
        if (_rt == null) return;
        _rt.position = screenPos;
    }
}
