using UnityEngine;
using UnityEngine.EventSystems;

public class UI_DragHandle : MonoBehaviour, IPointerDownHandler, IDragHandler
{
    [Header("Drag Target (the panel root)")]
    public RectTransform target;        // InventoryPanel / PartyPanel

    [Header("Drag Area (usually SafeRoot or Canvas root)")]
    public RectTransform dragArea;      // SafeRoot 권장

    private Vector2 _offset;

    void Awake()
    {
        if (target == null)
            target = transform as RectTransform;

        if (dragArea == null && target != null)
            dragArea = target.parent as RectTransform;
    }

    public void OnPointerDown(PointerEventData e)
    {
        if (target == null || dragArea == null) return;

        target.SetAsLastSibling(); // 클릭하면 맨 앞으로

        RectTransformUtility.ScreenPointToLocalPointInRectangle(
            dragArea, e.position, e.pressEventCamera, out var localPoint);

        _offset = target.anchoredPosition - localPoint;
    }

    public void OnDrag(PointerEventData e)
    {
        if (target == null || dragArea == null) return;

        RectTransformUtility.ScreenPointToLocalPointInRectangle(
            dragArea, e.position, e.pressEventCamera, out var localPoint);

        target.anchoredPosition = localPoint + _offset;
    }
}
