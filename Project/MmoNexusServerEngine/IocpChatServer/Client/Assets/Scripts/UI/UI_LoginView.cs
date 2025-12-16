using UnityEngine;
using UnityEngine.UI;
using TMPro;

public class UI_LoginView : MonoBehaviour
{
    public TMP_InputField idInput;
    public TMP_InputField pwInput;
    public Button loginButton;
    public TMP_Text statusText;
    public UI_LoadingOverlay loading;

    void Start()
    {
        statusText.text = "";
        loginButton.onClick.AddListener(OnClickLogin);
        loading.Hide();
    }

    void OnClickLogin()
    {
        string id = idInput.text.Trim();
        string pw = pwInput.text;

        if (string.IsNullOrEmpty(id) || string.IsNullOrEmpty(pw))
        {
            statusText.text = "ID/PW를 입력해라.";
            return;
        }

        statusText.text = "";
        loginButton.interactable = false;
        loading.Show("Logging in...");

        NexusClient.Instance.RequestLogin(id, pw);

        // 로그인 성공/실패 결과는 NexusClient.HandleLogin에서 처리됨
        // 실패까지 UI에 반영하려면 “이벤트”를 붙이는 게 가장 깔끔하지만,
        // 당장 초보 단계에서는 타임아웃만 하나 넣어도 됨.
        Invoke(nameof(LoginTimeout), 5f);
    }

    void LoginTimeout()
    {
        // 이미 씬 넘어갔으면 이 오브젝트는 파괴됐을 거라 사실상 호출 안 됨
        loginButton.interactable = true;
        loading.Hide();
        statusText.text = "로그인 응답이 없다. 서버/포트 확인.";
    }

    void OnDestroy()
    {
        CancelInvoke();
    }
}
