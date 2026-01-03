using UnityEngine;
using UnityEngine.UI;
using Protocol;
using TMPro;

public class UI_PartyMemberRow : MonoBehaviour
{
    [Header("Texts")]
    public TMP_Text nameText;
    public TMP_Text levelText;
    public TMP_Text hpText;

    [Header("Controls")]
    public Button btnKick;

    ulong _playerId;

    void Start()
    {
        if (btnKick != null)
            btnKick.onClick.AddListener(OnKickClicked);
    }

    public void BindFallback(ulong playerId)
    {
        _playerId = playerId;

        if (nameText != null) nameText.text = $"[{playerId}]";
        if (levelText != null) levelText.text = "-";
        if (hpText != null) hpText.text = "(no status)";

        UpdateKickButton();
    }

    public void Bind(PartyMemberStatus m)
    {
        _playerId = m.PlayerId;

        if (nameText != null)
            nameText.text = string.IsNullOrEmpty(m.Name) ? $"[{m.PlayerId}]" : $"{m.Name} ({m.PlayerId})";

        if (levelText != null)
            levelText.text = $"Lv{m.Level}";

        if (hpText != null)
            hpText.text = $"{m.Hp}/{m.MaxHp}";

        UpdateKickButton();
    }

    void UpdateKickButton()
    {
        if (btnKick == null) return;

        bool leader = PartyClient.Instance != null && PartyClient.Instance.IsLeader;
        bool isMe = _playerId != 0 && _playerId == ObjectManager.MyPlayerId;

        // 리더만 Kick 보임 + 내 자신은 Kick 금지
        btnKick.gameObject.SetActive(leader && !isMe);
    }

    void OnKickClicked()
    {
        if (_playerId == 0) return;
        if (_playerId == ObjectManager.MyPlayerId) return;

        PartyApi.Kick(_playerId);
    }
}
