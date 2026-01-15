using UnityEngine;

/// <summary>
/// Keyboard hotkeys for quickslots.
/// Attach to MyPlayer (recommended) so we can pass transform yaw for skills.
/// </summary>
public class QuickSlotInput : MonoBehaviour
{
    void Update()
    {
        if (NetworkManager.Instance != null && NetworkManager.Instance.IsMapChanging)
            return;

        if (UI_ChatPanel.IsTyping)
            return;

        // 12-slot mapping: 1-9,0,-,=
        if (Input.GetKeyDown(KeyCode.Alpha1)) Use(0);
        else if (Input.GetKeyDown(KeyCode.Alpha2)) Use(1);
        else if (Input.GetKeyDown(KeyCode.Alpha3)) Use(2);
        else if (Input.GetKeyDown(KeyCode.Alpha4)) Use(3);
        else if (Input.GetKeyDown(KeyCode.Alpha5)) Use(4);
        else if (Input.GetKeyDown(KeyCode.Alpha6)) Use(5);
        else if (Input.GetKeyDown(KeyCode.Alpha7)) Use(6);
        else if (Input.GetKeyDown(KeyCode.Alpha8)) Use(7);
        else if (Input.GetKeyDown(KeyCode.Alpha9)) Use(8);
        else if (Input.GetKeyDown(KeyCode.Alpha0)) Use(9);
        else if (Input.GetKeyDown(KeyCode.Minus)) Use(10);
        else if (Input.GetKeyDown(KeyCode.Equals)) Use(11);
    }

    void Use(int idx)
    {
        QuickSlotManager.Instance.TryUse(idx, transform);
    }
}
