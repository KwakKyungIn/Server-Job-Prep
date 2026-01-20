using UnityEngine;

// Convenience wrapper: auto-binds child UI_SkillSlot components in order.
// Drop this on a parent GameObject and assign initialSkillTemplateIds.
public class UI_SkillBar : MonoBehaviour
{
    [Header("Bind")]
    public int[] initialSkillTemplateIds;

    void Start()
    {
        var slots = GetComponentsInChildren<UI_SkillSlot>(true);
        if (slots == null || slots.Length == 0)
            return;

        if (initialSkillTemplateIds == null)
            return;

        int n = Mathf.Min(slots.Length, initialSkillTemplateIds.Length);
        for (int i = 0; i < n; i++)
            slots[i].SetSkill(initialSkillTemplateIds[i]);
    }
}
