using UnityEngine;
using UnityEngine.UI;
using Protocol;
using TMPro;
public class UI_PartyMemberRow : MonoBehaviour
{
    public TMP_Text text;
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
        if (text != null) text.text = $"[{playerId}] (no status)";

        UpdateKickButton();
    }

    public void Bind(PartyMemberStatus m)
    {
        _playerId = m.PlayerId;

        if (text != null)
            text.text = $"{m.Name} ({m.PlayerId}) Lv{m.Level} HP {m.Hp}/{m.MaxHp}";

        UpdateKickButton();
    }

    void UpdateKickButton()
    {
        if (btnKick == null) return;

        bool leader = PartyClient.Instance != null && PartyClient.Instance.IsLeader;
        bool isMe = _playerId != 0 && _playerId == ObjectManager.MyPlayerId;

        // ✅ 리더만 Kick 보임 + 내 자신은 Kick 금지
        btnKick.gameObject.SetActive(leader && !isMe);
    }

    void OnKickClicked()
    {
        if (_playerId == 0) return;
        if (_playerId == ObjectManager.MyPlayerId) return;

        PartyApi.Kick(_playerId);
    }
}
