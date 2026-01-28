using System;
using UnityEngine;

public class GoldManager
{
    static readonly GoldManager _instance = new GoldManager();
    public static GoldManager Instance => _instance;

    public event Action OnUpdated;

    bool _inited = false;
    long _gold = 0;
    bool _hasGold = false;

    public bool HasGold => _hasGold;

    public void Init()
    {
        if (_inited) return;
        _inited = true;

        PacketHandler.OnGoldUpdate += HandleGoldUpdate;
    }

    public void Shutdown()
    {
        if (!_inited) return;
        _inited = false;

        PacketHandler.OnGoldUpdate -= HandleGoldUpdate;
    }

    public void Reset()
    {
        _gold = 0;
        _hasGold = false;
        OnUpdated?.Invoke();
    }

    public long GetGold()
    {
        return _gold;
    }

    void HandleGoldUpdate(long gold)
    {
        if (gold < 0) gold = 0;
        _gold = gold;
        _hasGold = true;
        OnUpdated?.Invoke();
    }
}
