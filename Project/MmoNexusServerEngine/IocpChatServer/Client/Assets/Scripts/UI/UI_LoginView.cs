using UnityEngine;
using UnityEngine.UI;
using UnityEngine.SceneManagement;
using TMPro;
public class UI_LoginView : MonoBehaviour
{
    public TMP_InputField idInput;
    public TMP_InputField pwInput;
    public Button loginButton;
    public TMP_Text statusText;

    void Start()
    {
        loginButton.onClick.AddListener(OnClickLogin);
        statusText.text = "";
    }

    void OnClickLogin()
    {
        statusText.text = "Logging in...";
        NexusClient.Instance.RequestLogin(idInput.text, pwInput.text);
    }
}
