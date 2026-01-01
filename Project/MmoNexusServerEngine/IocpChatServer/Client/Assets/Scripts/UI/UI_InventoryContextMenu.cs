using System;
using UnityEngine;
using UnityEngine.UI;
using TMPro;
using Protocol;

public class UI_InventoryContextMenu : MonoBehaviour
{
    [Header("Wiring")]
    public RectTransform panel;       // 보통 이 스크립트가 붙은 오브젝트 RectTransform 넣어도 됨
    public Button equipButton;
    public TMP_Text equipLabel;
    public Button detailsButton;

    public bool IsOpen => gameObject.activeSelf;
    public int SlotIndex { get; private set; } = -1;

    Canvas _canvas;
    RectTransform _canvasRect;
    RectTransform _parentRect;
    int _openFrame = -1;

    void Awake()
    {
        if (!panel) panel = (RectTransform)transform;

        _canvas = GetComponentInParent<Canvas>(true); // includeInactive
        _canvasRect = _canvas ? _canvas.transform as RectTransform : null;

        _parentRect = panel.parent as RectTransform;

        Hide();
    }

    // 비활성 시작/재부모 대비: Show에서 늦게라도 잡기
    bool EnsureRects()
    {
        if (_canvasRect == null)
        {
            _canvas = GetComponentInParent<Canvas>(true);
            _canvasRect = _canvas ? _canvas.transform as RectTransform : null;
        }

        if (_parentRect == null && panel != null)
            _parentRect = panel.parent as RectTransform;

        if (_canvasRect == null || _parentRect == null)
        {
            Debug.LogWarning("[UI_InventoryContextMenu] Canvas/Parent Rect not found.");
            return false;
        }

        return true;
    }

    void Update()
    {
        if (!IsOpen) return;
        if (Time.frameCount == _openFrame) return;

        if (Input.GetKeyDown(KeyCode.Escape))
        {
            Hide();
            return;
        }

        // 바깥 클릭하면 닫기
        if (Input.GetMouseButtonDown(0) || Input.GetMouseButtonDown(1))
        {
            var cam = (_canvas != null && _canvas.renderMode != RenderMode.ScreenSpaceOverlay) ? _canvas.worldCamera : null;
            if (!RectTransformUtility.RectangleContainsScreenPoint(panel, Input.mousePosition, cam))
                Hide();
        }
    }

    public void Show(int slotIndex, ItemInfo item, Vector2 screenPos, Action onEquip, Action onDetails)
    {
        if (!EnsureRects())
            return;

        SlotIndex = slotIndex;

        // 라벨
        if (equipLabel)
            equipLabel.text = item.IsEquipped ? "Unequip" : "Equip";

        // 버튼 리스너
        if (equipButton)
        {
            equipButton.onClick.RemoveAllListeners();
            equipButton.onClick.AddListener(() => onEquip?.Invoke());
        }

        if (detailsButton)
        {
            detailsButton.onClick.RemoveAllListeners();
            detailsButton.onClick.AddListener(() => onDetails?.Invoke());
        }

        gameObject.SetActive(true);

        // ✅ 핵심 FIX:
        // ScreenPos -> "panel의 부모 RectTransform" 기준 localPos로 변환해야 anchoredPosition이 안 튄다.
        var cam = (_canvas != null && _canvas.renderMode != RenderMode.ScreenSpaceOverlay) ? _canvas.worldCamera : null;

        if (RectTransformUtility.ScreenPointToLocalPointInRectangle(_parentRect, screenPos, cam, out Vector2 localPos))
        {
            panel.anchoredPosition = ClampToParent(localPos);
        }

        _openFrame = Time.frameCount; // 열린 프레임엔 즉시 닫히지 않게
    }

    Vector2 ClampToParent(Vector2 anchoredPos)
    {
        if (_parentRect == null) return anchoredPos;

        Vector2 parentSize = _parentRect.rect.size;
        Vector2 panelSize = panel.rect.size;
        Vector2 pivot = panel.pivot;

        // parent pivot이 (0.5,0.5)라고 가정(대부분 UI 패널은 이거임)
        // pivot 고려해서 "메뉴가 화면 밖으로 나가지 않게" 클램프
        float minX = -parentSize.x * 0.5f + panelSize.x * pivot.x;
        float maxX = parentSize.x * 0.5f - panelSize.x * (1f - pivot.x);

        float minY = -parentSize.y * 0.5f + panelSize.y * pivot.y;
        float maxY = parentSize.y * 0.5f - panelSize.y * (1f - pivot.y);

        anchoredPos.x = Mathf.Clamp(anchoredPos.x, minX, maxX);
        anchoredPos.y = Mathf.Clamp(anchoredPos.y, minY, maxY);
        return anchoredPos;
    }

    public void Hide()
    {
        SlotIndex = -1;

        if (equipButton) equipButton.onClick.RemoveAllListeners();
        if (detailsButton) detailsButton.onClick.RemoveAllListeners();

        gameObject.SetActive(false);
    }
}
