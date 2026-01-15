using System;
using UnityEngine;
using UnityEngine.UI;
using TMPro;
using Protocol;

public class UI_InventoryContextMenu : MonoBehaviour
{
    [Header("Wiring")]
    public RectTransform panel; // 보통 이 스크립트가 붙은 오브젝트 RectTransform 넣어도 됨

    [Header("Buttons")]
    public Button equipButton;
    public Button unequipButton;
    public Button useButton;
    public Button detailsButton;

    [Header("Labels (Optional)")]
    public TMP_Text equipLabel;
    public TMP_Text unequipLabel;
    public TMP_Text useLabel;

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

    static bool IsConsumable(ItemInfo item)
    {
        if (item == null) return false;
        int tid = item.TemplateId;
        return (tid >= 3000 && tid <= 3999); // "3000대는 전부 사용아이템"
    }

    static bool IsEquipable(ItemInfo item)
    {
        if (item == null) return false;
        int tid = item.TemplateId;
        // 1000~1999 Weapon, 2000~2999 Body, 4000~4999 Head
        return (tid >= 1000 && tid < 2000) || (tid >= 2000 && tid < 3000) || (tid >= 4000 && tid < 5000);
    }

    public void Show(
        int slotIndex,
        ItemInfo item,
        Vector2 screenPos,
        Action onEquip,
        Action onUnequip,
        Action onUse,
        Action onDetails)
    {
        if (!EnsureRects())
            return;

        SlotIndex = slotIndex;

        bool consumable = IsConsumable(item);
        bool equipable = IsEquipable(item);
        bool equipped = item != null && item.IsEquipped;

        // 라벨(선택)
        if (equipLabel) equipLabel.text = "Equip";
        if (unequipLabel) unequipLabel.text = "Unequip";
        if (useLabel) useLabel.text = "Use";

        // 버튼 노출 규칙
        // - 사용아이템(3000대): Use만
        // - 장비아이템(그 외): Equip/Unequip 중 하나만
        if (equipButton) equipButton.gameObject.SetActive(!consumable && equipable && !equipped);
        if (unequipButton) unequipButton.gameObject.SetActive(!consumable && equipable && equipped);
        if (useButton) useButton.gameObject.SetActive(consumable);
        if (detailsButton) detailsButton.gameObject.SetActive(true);

        // 리스너 세팅
        if (equipButton)
        {
            equipButton.onClick.RemoveAllListeners();
            equipButton.onClick.AddListener(() =>
            {
                onEquip?.Invoke();
                Hide();
            });
        }

        if (unequipButton)
        {
            unequipButton.onClick.RemoveAllListeners();
            unequipButton.onClick.AddListener(() =>
            {
                onUnequip?.Invoke();
                Hide();
            });
        }

        if (useButton)
        {
            useButton.onClick.RemoveAllListeners();
            useButton.onClick.AddListener(() =>
            {
                onUse?.Invoke();
                Hide();
            });
        }

        if (detailsButton)
        {
            detailsButton.onClick.RemoveAllListeners();
            detailsButton.onClick.AddListener(() =>
            {
                onDetails?.Invoke();
                // 디테일 열면 메뉴는 닫는 게 보통 UX 좋음 (원하면 주석)
                Hide();
            });
        }

        gameObject.SetActive(true);

        // ScreenPos -> 부모 Rect 기준 localPos로 변환
        var cam = (_canvas != null && _canvas.renderMode != RenderMode.ScreenSpaceOverlay) ? _canvas.worldCamera : null;

        if (RectTransformUtility.ScreenPointToLocalPointInRectangle(_parentRect, screenPos, cam, out Vector2 localPos))
            panel.anchoredPosition = ClampToParent(localPos);

        _openFrame = Time.frameCount;
    }

    Vector2 ClampToParent(Vector2 anchoredPos)
    {
        if (_parentRect == null) return anchoredPos;

        Vector2 parentSize = _parentRect.rect.size;
        Vector2 panelSize = panel.rect.size;
        Vector2 pivot = panel.pivot;

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
        if (unequipButton) unequipButton.onClick.RemoveAllListeners();
        if (useButton) useButton.onClick.RemoveAllListeners();
        if (detailsButton) detailsButton.onClick.RemoveAllListeners();

        gameObject.SetActive(false);
    }
}
