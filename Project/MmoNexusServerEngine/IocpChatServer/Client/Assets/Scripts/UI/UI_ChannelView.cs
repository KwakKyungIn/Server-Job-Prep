using UnityEngine;
using UnityEngine.UI;
using Protocol;
using TMPro; // ★ 추가

public class UI_ChannelView : MonoBehaviour
{
    public Transform contentRoot;
    public Button channelButtonPrefab;

    void Start()
    {
        // 0) 방어 체크 (초보 구간에서 반드시)
        if (contentRoot == null)
        {
            Debug.LogError("[UI_ChannelView] contentRoot is NULL. ScrollView/Viewport/Content 넣었는지 확인");
            return;
        }
        if (channelButtonPrefab == null)
        {
            Debug.LogError("[UI_ChannelView] channelButtonPrefab is NULL. 프리팹(Button) 연결 확인");
            return;
        }
        if (NexusClient.Instance == null)
        {
            Debug.LogError("[UI_ChannelView] NexusClient.Instance is NULL. NexusClient가 DontDestroy로 살아있나 확인");
            return;
        }

        var list = NexusClient.Instance.GetServerList();
        if (list == null)
        {
            Debug.LogError("[UI_ChannelView] ServerList is NULL. GetServerList()가 null 리턴하는지 확인");
            return;
        }

        Debug.Log($"[UI_ChannelView] serverList count = {list.Count}");

        for (int i = 0; i < list.Count; i++)
        {
            int idx = i;
            ServerInfo s = list[i];

            Button btn = Instantiate(channelButtonPrefab, contentRoot);

            // ★ TMP 우선
            TMP_Text tmp = btn.GetComponentInChildren<TMP_Text>();
            if (tmp != null)
            {
                tmp.text = $"{idx + 1}. {s.Name} ({s.Ip}:{s.Port}) Load:{s.Congestion}";
            }
            else
            {
                // 혹시 레거시 Text면 이것도 지원
                Text legacy = btn.GetComponentInChildren<Text>();
                if (legacy != null)
                    legacy.text = $"{idx + 1}. {s.Name} ({s.Ip}:{s.Port}) Load:{s.Congestion}";
                else
                    Debug.LogError("[UI_ChannelView] Button prefab에 TMP_Text도 Text도 없음. 라벨 오브젝트 확인해라.");
            }

            btn.onClick.AddListener(() =>
            {
                NexusClient.Instance.RequestEnterGameByIndex(idx);
            });
        }
    }
}
