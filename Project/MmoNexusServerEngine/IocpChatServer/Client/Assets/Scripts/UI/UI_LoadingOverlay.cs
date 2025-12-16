using UnityEngine;
using TMPro;

public class UI_LoadingOverlay : MonoBehaviour
{
    public GameObject root;
    public TMP_Text messageText;

    public void Show(string msg = "Loading...")
    {
        if (messageText != null) messageText.text = msg;
        root.SetActive(true);
    }

    public void Hide()
    {
        root.SetActive(false);
    }
}
