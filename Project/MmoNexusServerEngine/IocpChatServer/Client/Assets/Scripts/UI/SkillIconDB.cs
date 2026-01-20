using System.Collections.Generic;
using UnityEngine;

public static class SkillIconDB
{
    // skillTemplateId -> Sprite cache
    static readonly Dictionary<int, Sprite> _cache = new();
    static Sprite _fallback;

    // Put skill icons here:
    //   Assets/Resources/SkillIcons/{templateId}.png
    // Then load with Resources.Load<Sprite>("SkillIcons/{templateId}")
    const string ResourceRoot = "SkillIcons";

    public static void SetFallback(Sprite fallback) => _fallback = fallback;

    public static Sprite Get(int skillTemplateId)
    {
        if (_cache.TryGetValue(skillTemplateId, out var sp) && sp != null)
            return sp;

        sp = Resources.Load<Sprite>($"{ResourceRoot}/{skillTemplateId}");
        if (sp == null)
            sp = _fallback;

        _cache[skillTemplateId] = sp;
        return sp;
    }

    public static void Clear() => _cache.Clear();
}
