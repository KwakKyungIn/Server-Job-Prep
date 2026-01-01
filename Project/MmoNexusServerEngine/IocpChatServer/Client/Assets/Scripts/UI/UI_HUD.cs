using UnityEngine;
using UnityEngine.UI;
using TMPro;
using Protocol;

public class UI_HUD : MonoBehaviour
{
    [Header("HP")]
    public Slider hpBar;       // HPBar
    public TMP_Text hpText;    // HPText

    [Header("EXP")]
    public Slider expBar;      // (너의 EXPGroup/Slider)
    public TMP_Text levelText; // LevelText
    public TMP_Text expText;   // ExpText

    int _hp, _maxHp = 1, _level;
    long _totalExp;

    void OnEnable()
    {
        PacketHandler.OnChangeStat += OnChangeStat;
        PacketHandler.OnChangeHp += OnChangeHp;
    }

    void OnDisable()
    {
        PacketHandler.OnChangeStat -= OnChangeStat;
        PacketHandler.OnChangeHp -= OnChangeHp;
    }

    void OnChangeStat(StatInfo stat)
    {
        _level = stat.Level;
        _hp = stat.Hp;
        _maxHp = Mathf.Max(1, stat.MaxHp);
        _totalExp = stat.TotalExp;
        Refresh();
    }

    void OnChangeHp(S_CHANGE_HP pkt)
    {
        if (pkt.ObjectId != ObjectManager.MyPlayerId) return;
        _hp = pkt.CurrentHp;
        Refresh();
    }

    void Refresh()
    {
        if (hpBar) hpBar.value = (float)_hp / _maxHp;
        if (hpText) hpText.text = $"{_hp} / {_maxHp}";

        if (levelText) levelText.text = $"{_level}";
        if (expText) expText.text = $"EXP {_totalExp}";

        // exp %는 서버가 "다음 레벨 필요 exp"를 안 보내면 정확히 못 만듦.
        // 일단 감각용(임시): 0~1로 대충 매핑
        if (expBar) expBar.value = (float)(_totalExp % 1000) / 1000f;
    }
}
