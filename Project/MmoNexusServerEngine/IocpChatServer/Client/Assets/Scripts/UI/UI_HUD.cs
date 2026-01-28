using UnityEngine;
using UnityEngine.UI;
using TMPro;

public class UI_HUD : MonoBehaviour
{
    [Header("HP")]
    public Slider hpBar;
    public TMP_Text hpText;

    [Header("EXP")]
    public Slider expBar;
    public TMP_Text levelText;
    public TMP_Text expText;

    [Header("Gold")]
    public TMP_Text goldText;

    int _hp, _maxHp = 1, _level;
    long _totalExp;
    long _gold;

    void OnEnable()
    {
        StatManager.Instance.OnUpdated += RefreshFromCache;
        GoldManager.Instance.OnUpdated += RefreshFromCache;
        RefreshFromCache(); // ✅ 씬 전환 직후에도 캐시로 즉시 복구
    }

    void OnDisable()
    {
        StatManager.Instance.OnUpdated -= RefreshFromCache;
        GoldManager.Instance.OnUpdated -= RefreshFromCache;
    }

    void RefreshFromCache()
    {
        if (!StatManager.Instance.HasStat)
        {
            // 아직 스탯을 못 받은 상태 (EnterGame 전 등)
            _hp = 0;
            _maxHp = 1;
            _level = 0;
            _totalExp = 0;
            _gold = GoldManager.Instance.HasGold ? GoldManager.Instance.GetGold() : 0;
            Refresh();
            return;
        }

        var stat = StatManager.Instance.GetSnapshot();
        if (stat == null)
            return;

        _level = stat.Level;
        _maxHp = Mathf.Max(1, stat.MaxHp);
        _totalExp = stat.TotalExp;

        int hp = StatManager.Instance.GetHp();
        _hp = Mathf.Clamp(hp, 0, _maxHp);

        if (GoldManager.Instance.HasGold)
            _gold = GoldManager.Instance.GetGold();
        else
            _gold = 0;

        Refresh();
    }

    void Refresh()
    {
        if (hpBar) hpBar.value = (float)_hp / _maxHp;
        if (hpText) hpText.text = $"{_hp} / {_maxHp}";

        if (levelText) levelText.text = $"{_level}";
        if (expText) expText.text = $"EXP {_totalExp}";

        if (expBar) expBar.value = (float)(_totalExp % 1000) / 1000f;

        if (goldText) goldText.text = $"GOLD {_gold}";
    }
}
