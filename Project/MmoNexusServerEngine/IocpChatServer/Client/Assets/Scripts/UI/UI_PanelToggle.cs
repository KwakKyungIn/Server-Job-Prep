using UnityEngine;

public class UI_PanelToggle : MonoBehaviour
{
    public GameObject inventoryPanel; // RightPanel/InventoryPanel

    void Start()
    {
        if (inventoryPanel) inventoryPanel.SetActive(false);
    }

    void Update()
    {
        if (Input.GetKeyDown(KeyCode.I) && inventoryPanel != null)
            inventoryPanel.SetActive(!inventoryPanel.activeSelf);
    }
}
