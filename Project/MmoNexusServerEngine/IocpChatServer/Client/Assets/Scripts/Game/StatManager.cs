using System;
using UnityEngine;
using Protocol;

public class StatManager
{
    static readonly StatManager _instance = new StatManager();
    public static StatManager Instance => _instance;

    public event Action OnUpdated;

    bool _inited = false;

    StatInfo _stat;                 // 마지막으로 받은 스탯 스냅샷
    int _hpOverride = -1;           // HP 패킷으로만 갱신될 때 대비

    public bool HasStat => _stat != null;

    public void Init()
    {
        if (_inited) return;
        _inited = true;

        PacketHandler.OnChangeStat += HandleChangeStat;
        PacketHandler.OnChangeHp += HandleChangeHp;

        Debug.Log("[StatManager] Initialized & Listening...");
    }

    public void Shutdown()
    {
        if (!_inited) return;
        _inited = false;

        PacketHandler.OnChangeStat -= HandleChangeStat;
        PacketHandler.OnChangeHp -= HandleChangeHp;
    }

    public void Reset()
    {
        _stat = null;
        _hpOverride = -1;
        OnUpdated?.Invoke();
    }

    // UI가 안전하게 쓰도록 복사본 제공
    public StatInfo GetSnapshot()
    {
        return _stat?.Clone();
    }

    public int GetHp()
    {
        if (_hpOverride >= 0) return _hpOverride;
        return _stat?.Hp ?? 0;
    }

    // ================== Handlers ==================

    void HandleChangeStat(StatInfo stat)
    {
        if (stat == null) return;

        // protobuf 메시지는 참조 공유 위험 있으니 Clone해서 캐시
        _stat = stat.Clone();
        _hpOverride = -1; // 전체 스탯 갱신 오면 HP는 stat 기준으로 초기화

        OnUpdated?.Invoke();
    }

    void HandleChangeHp(S_CHANGE_HP pkt)
    {
        if (pkt == null) return;

        // 내 플레이어만 반영
        if (ObjectManager.MyPlayerId == 0) return;
        if (pkt.ObjectId != ObjectManager.MyPlayerId) return;

        _hpOverride = pkt.CurrentHp;

        // 스냅샷도 일관성 유지
        if (_stat != null)
            _stat.Hp = pkt.CurrentHp;

        OnUpdated?.Invoke();
    }
}
