using System;
using System.Collections.Generic;
using UnityEngine;
using Protocol;

// Tracks authoritative skill cooldowns for the local player.
// Cooldown starts when we receive S_SKILL for my player (server accepted the cast).
public class SkillCooldownManager
{
    public static SkillCooldownManager Instance { get; } = new SkillCooldownManager();

    struct CooldownState
    {
        public float EndTime;
        public float Duration;
    }

    readonly Dictionary<int, CooldownState> _cooldowns = new();
    bool _inited;

    // Optional: listen if you want to react to cooldown start/end.
    public event Action<int> OnCooldownStarted;
    public event Action<int> OnCooldownEnded;

    SkillCooldownManager() { }

    public void Init()
    {
        if (_inited) return;
        _inited = true;

        PacketHandler.OnSkill += HandleSkill;
        PacketHandler.OnEnterGame += _ => Clear();
        PacketHandler.OnMapChangeBegin += _ => Clear();
    }

    void HandleSkill(S_SKILL pkt)
    {
        // Only track for my player.
        if (pkt.ObjectId != ObjectManager.MyPlayerId)
            return;

        int skillId = pkt.SkillId;
        int cdMs = 0;

        // cooldownMs is added to S_SKILL in proto.
        // If server/client proto isn't updated yet, this will just be 0.
        try { cdMs = pkt.CooldownMs; } catch { cdMs = 0; }

        if (cdMs <= 0)
        {
            // Fallback so UI still moves even if server isn't sending cooldown yet.
            // Adjust per skill if you want: SkillCooldownManager.Instance.SetDefaultCooldown(skillId, ms)
            cdMs = GetDefaultCooldownMs(skillId);
        }

        StartCooldown(skillId, cdMs);
    }

    // -------- Public API --------

    public void StartCooldown(int skillId, int durationMs)
    {
        float dur = Mathf.Max(0f, durationMs / 1000f);
        if (dur <= 0f)
        {
            Clear(skillId);
            return;
        }

        float now = Time.realtimeSinceStartup;
        _cooldowns[skillId] = new CooldownState
        {
            Duration = dur,
            EndTime = now + dur
        };

        OnCooldownStarted?.Invoke(skillId);
    }

    public bool IsOnCooldown(int skillId)
    {
        PruneIfExpired(skillId);
        return _cooldowns.ContainsKey(skillId);
    }

    // Returns remaining seconds and total duration seconds.
    public bool TryGetCooldown(int skillId, out float remainSec, out float durationSec)
    {
        remainSec = 0f;
        durationSec = 0f;

        if (_cooldowns.TryGetValue(skillId, out var st) == false)
            return false;

        float now = Time.realtimeSinceStartup;
        float remain = st.EndTime - now;
        if (remain <= 0f)
        {
            Clear(skillId);
            return false;
        }

        remainSec = remain;
        durationSec = Mathf.Max(0.001f, st.Duration);
        return true;
    }

    public void Clear()
    {
        if (_cooldowns.Count == 0)
            return;

        // Fire end events for listeners.
        var keys = new List<int>(_cooldowns.Keys);
        _cooldowns.Clear();
        foreach (var k in keys)
            OnCooldownEnded?.Invoke(k);
    }

    public void Clear(int skillId)
    {
        if (_cooldowns.Remove(skillId))
            OnCooldownEnded?.Invoke(skillId);
    }

    // -------- Defaults / fallback --------

    readonly Dictionary<int, int> _defaultCooldownMs = new();
    const int DefaultFallbackMs = 1000;

    public void SetDefaultCooldown(int skillId, int cooldownMs)
    {
        _defaultCooldownMs[skillId] = Mathf.Max(0, cooldownMs);
    }

    int GetDefaultCooldownMs(int skillId)
    {
        if (_defaultCooldownMs.TryGetValue(skillId, out int ms))
            return ms;
        return DefaultFallbackMs;
    }

    void PruneIfExpired(int skillId)
    {
        if (_cooldowns.TryGetValue(skillId, out var st) == false)
            return;

        if (Time.realtimeSinceStartup >= st.EndTime)
            Clear(skillId);
    }
}
