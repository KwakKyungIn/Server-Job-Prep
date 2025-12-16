using UnityEngine;
using UnityEngine.UI;
using TMPro;
using Protocol;

public class UI_ChannelView : MonoBehaviour
{
    [Header("3 fixed buttons")]
    public Button[] channelButtons;   // 크기 3으로 넣어라 (Btn1, Btn2, Btn3)

    [Header("UI")]
    public UI_LoadingOverlay loading;
    public TMP_Text statusText;

    void Start()
    {
        statusText.text = "";
        loading.Hide();

        PopulateFixed3();
    }

    void PopulateFixed3()
    {
        if (NexusClient.Instance == null)
        {
            statusText.text = "Client not ready.";
            SetButtonsInteractable(false);
            return;
        }

        var list = NexusClient.Instance.GetServerList();
        if (list == null || list.Count == 0)
        {
            statusText.text = "서버 리스트가 비었다.";
            SetButtonsInteractable(false);
            return;
        }

        // 버튼 3개 세팅
        for (int i = 0; i < channelButtons.Length; i++)
        {
            var btn = channelButtons[i];
            if (btn == null) continue;

            btn.onClick.RemoveAllListeners();

            if (i < list.Count)
            {
                int idx = i;
                ServerInfo s = list[i];

                // 라벨 세팅 (TMP 기준)
                SetCardText(btn.transform, idx, s);

                btn.interactable = true;
                btn.onClick.AddListener(() =>
                {
                    loading.Show("Connecting...");
                    SetButtonsInteractable(false);
                    NexusClient.Instance.RequestEnterGameByIndex(idx);
                });
            }
            else
            {
                // 서버 리스트가 3개보다 적으면 남는 버튼 숨김/비활성
                btn.interactable = false;
                btn.gameObject.SetActive(false);
            }
        }

        statusText.text = "채널을 선택해라.";
    }

    void SetButtonsInteractable(bool v)
    {
        for (int i = 0; i < channelButtons.Length; i++)
            if (channelButtons[i] != null)
                channelButtons[i].interactable = v;
    }

    void SetCardText(Transform btnRoot, int idx, ServerInfo s)
    {
        // 오브젝트 이름으로 찾는게 제일 깔끔함
        var nameText = btnRoot.Find("ServerName")?.GetComponent<TMP_Text>();
        var endpointText = btnRoot.Find("Endpoint")?.GetComponent<TMP_Text>();
        var loadText = btnRoot.Find("Load")?.GetComponent<TMP_Text>();

        if (nameText != null) nameText.text = $"{idx + 1}. {s.Name}";
        if (endpointText != null) endpointText.text = $"{s.Ip}:{s.Port}";
        if (loadText != null) loadText.text = $"Load: {s.Congestion}";
    }
}
