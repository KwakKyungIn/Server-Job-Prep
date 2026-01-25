using UnityEngine;
using TMPro;

public class UI_PanelToggle : MonoBehaviour
{
    public GameObject inventoryPanel; // RightPanel/InventoryPanel
    public GameObject partyPanel;     // RightPanel/PartyPanel

    // (선택) 채팅 입력 중엔 토글 안 되게
    public TMP_InputField chatInput;
    public TMP_InputField partyInviteInput;

    void Start()
    {
        if (inventoryPanel) inventoryPanel.SetActive(false);
        if (partyPanel) partyPanel.SetActive(false);
    }

    void Update()
    {
        if (chatInput != null && chatInput.isFocused)
            return;

        if (partyInviteInput != null && partyInviteInput.isFocused)
            return;

        if (Input.GetKeyDown(KeyCode.I))
            ToggleInventory();

        if (Input.GetKeyDown(KeyCode.K))
            ToggleParty();
    }

    // ✅ NEW: ESC에서 "하나씩 닫기" 용도로 사용
    public bool CloseOneByEsc()
    {
        // 우선순위: Party -> Inventory (원하면 반대로 바꿔도 됨)
        if (partyPanel != null && partyPanel.activeSelf)
        {
            partyPanel.SetActive(false);
            return true;
        }

        if (inventoryPanel != null && inventoryPanel.activeSelf)
        {
            inventoryPanel.SetActive(false);
            return true;
        }

        return false; // 닫을 게 없음
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

        if (next)
        {
            var ui = partyPanel.GetComponent<UI_PartyPanel>();
            if (ui != null)
                ui.OnOpen();
        }
    }
}
