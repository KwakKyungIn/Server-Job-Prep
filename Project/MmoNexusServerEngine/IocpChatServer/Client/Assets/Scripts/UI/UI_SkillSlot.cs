using TMPro;
using UnityEngine;
using UnityEngine.UI;

// UI slot for a single skill icon + radial cooldown overlay.
//
// Suggested prefab hierarchy:
//   Slot (UI_SkillSlot)
//     - Icon (Image)
//     - CooldownOverlay (Image, on top of Icon)
//     - CooldownText (TMP_Text, optional)
//
// CooldownOverlay defaults:
//   Type=Filled, FillMethod=Radial360, Origin=Top, Clockwise=true
public class UI_SkillSlot : MonoBehaviour
{
    [Header("Skill")]
    public int skillTemplateId;

    [Header("Wiring")]
    public Image iconImage;
    public Image cooldownOverlay;
    public TMP_Text cooldownText;

    [Header("Options")]
    public bool hideWhenEmpty = false;
    public bool showTextSeconds = true;

    void Awake()
    {
        if (cooldownOverlay != null)
        {
            // Enforce radial fill settings.
            cooldownOverlay.type = Image.Type.Filled;
            cooldownOverlay.fillMethod = Image.FillMethod.Radial360;
            cooldownOverlay.fillOrigin = 2; // Top
            cooldownOverlay.fillClockwise = true;
            cooldownOverlay.fillAmount = 0f;
        }
    }

    void Start()
    {
        RefreshIcon();
        RefreshVisual(0f, 1f);
    }

    void Update()
    {
        if (skillTemplateId <= 0)
        {
            if (hideWhenEmpty)
                gameObject.SetActive(false);
            return;
        }

        if (SkillCooldownManager.Instance.TryGetCooldown(skillTemplateId, out float remain, out float dur))
        {
            RefreshVisual(remain, dur);
        }
        else
        {
            RefreshVisual(0f, 1f);
        }
    }

    public void SetSkill(int templateId)
    {
        skillTemplateId = templateId;
        gameObject.SetActive(true);
        RefreshIcon();
    }

    void RefreshIcon()
    {
        if (iconImage == null)
            return;

        if (skillTemplateId <= 0)
        {
            iconImage.sprite = null;
            iconImage.enabled = false;
            return;
        }

        iconImage.enabled = true;
        iconImage.sprite = SkillIconDB.Get(skillTemplateId);
    }

    void RefreshVisual(float remain, float duration)
    {
        bool on = remain > 0.001f;

        if (cooldownOverlay != null)
        {
            cooldownOverlay.enabled = on;
            if (on)
                cooldownOverlay.fillAmount = Mathf.Clamp01(remain / Mathf.Max(0.001f, duration));
            else
                cooldownOverlay.fillAmount = 0f;
        }

        if (cooldownText != null)
        {
            if (on && showTextSeconds)
            {
                int sec = Mathf.CeilToInt(remain);
                cooldownText.text = sec.ToString();
                cooldownText.enabled = true;
            }
            else
            {
                cooldownText.text = string.Empty;
                cooldownText.enabled = false;
            }
        }
    }
}
