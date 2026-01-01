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
    int _openFrame = -1;

    void Awake()
    {
        if (!panel) panel = (RectTransform)transform;
        _canvas = GetComponentInParent<Canvas>(true); // [FIX] includeInactive
        _canvasRect = _canvas ? _canvas.transform as RectTransform : null;

        Hide();
    }

    // [FIX] ContextMenu가 비활성으로 시작하면 Awake가 늦게/안 불릴 수 있으니 Show에서 보강
    bool EnsureCanvas()
    {
        if (_canvasRect != null) return true;

        _canvas = GetComponentInParent<Canvas>(true); // includeInactive
        _canvasRect = _canvas ? _canvas.transform as RectTransform : null;

        if (_canvasRect == null)
        {
            Debug.LogWarning("[UI_InventoryContextMenu] Canvas not found in parents.");
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
        // [FIX] Awake 캐시 실패/지연 대비
        if (!EnsureCanvas())
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

        // 위치 잡기 (Screen -> Canvas Local)
        var cam = (_canvas.renderMode != RenderMode.ScreenSpaceOverlay) ? _canvas.worldCamera : null;

        if (RectTransformUtility.ScreenPointToLocalPointInRectangle(_canvasRect, screenPos, cam, out Vector2 localPos))
        {
            panel.anchoredPosition = ClampToCanvas(localPos);
        }

        _openFrame = Time.frameCount; // 열린 프레임엔 즉시 닫히지 않게
    }

    Vector2 ClampToCanvas(Vector2 anchoredPos)
    {
        // Canvas pivot이 (0.5,0.5)인 일반적인 케이스 기준 클램프
        // 너 Canvas/Panel pivot이 다르면 여기만 살짝 수정하면 됨.
        if (!_canvasRect) return anchoredPos;

        Vector2 canvasSize = _canvasRect.rect.size;
        Vector2 panelSize = panel.rect.size;

        float minX = -canvasSize.x * 0.5f + panelSize.x * 0.5f;
        float maxX = canvasSize.x * 0.5f - panelSize.x * 0.5f;
        float minY = -canvasSize.y * 0.5f + panelSize.y * 0.5f;
        float maxY = canvasSize.y * 0.5f - panelSize.y * 0.5f;

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
