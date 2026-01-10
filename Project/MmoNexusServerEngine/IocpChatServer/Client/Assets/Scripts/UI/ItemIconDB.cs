using System.Collections.Generic;
using UnityEngine;

public static class ItemIconDB
{
    // templateId -> Sprite 캐시
    static readonly Dictionary<int, Sprite> _cache = new();

    // 없을 때 표시할 기본 아이콘(선택)
    static Sprite _fallback;

    public static void SetFallback(Sprite fallback) => _fallback = fallback;

    public static Sprite Get(int templateId)
    {
        if (_cache.TryGetValue(templateId, out var sp) && sp != null)
            return sp;

        // Resources/Icons/{templateId}.png 를 찾음
        sp = Resources.Load<Sprite>($"Icons/{templateId}");

        if (sp == null)
        {
            // 못 찾으면 fallback (또는 null)
            sp = _fallback;
        }

        _cache[templateId] = sp;
        return sp;
    }

    // 필요하면 씬 전환 등에 캐시 비우기
    public static void Clear()
    {
        _cache.Clear();
    }
}
