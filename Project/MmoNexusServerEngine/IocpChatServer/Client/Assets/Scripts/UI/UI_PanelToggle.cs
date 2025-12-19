using UnityEngine;
using TMPro;

public class UI_PanelToggle : MonoBehaviour
{
    public GameObject inventoryPanel; // RightPanel/InventoryPanel
    public GameObject partyPanel;     // RightPanel/PartyPanel

    // (선택) 채팅 입력 중엔 토글 안 되게
    public TMP_InputField chatInput;

    void Start()
    {
        if (inventoryPanel) inventoryPanel.SetActive(false);
        if (partyPanel) partyPanel.SetActive(false);
    }

    void Update()
    {
        if (chatInput != null && chatInput.isFocused)
            return;

        if (Input.GetKeyDown(KeyCode.I))
            ToggleInventory();

        if (Input.GetKeyDown(KeyCode.K))
            ToggleParty();
    }

    void ToggleInventory()
    {
        if (!inventoryPanel) return;

        bool next = !inventoryPanel.activeSelf;
        inventoryPanel.SetActive(next);

        if (next && partyPanel)
            partyPanel.SetActive(false);
    }

    void ToggleParty()
    {
        if (!partyPanel) return;

        bool next = !partyPanel.activeSelf;
        partyPanel.SetActive(next);

        if (next && inventoryPanel)
            inventoryPanel.SetActive(false);

        // 파티 패널 켰으면 상태 요청 + 즉시 갱신
        if (next)
        {
            var ui = partyPanel.GetComponent<UI_PartyPanel>();
            if (ui != null)
                ui.OnOpen(); // 아래에서 추가할 함수
        }
    }
}
