using Google.Protobuf;
using Protocol;
using System; // [Add] Action 사용을 위해 추가
using UnityEngine;

public class PacketHandler
{
    // [GIGACHAD] UI 쪽으로 이벤트를 토스하기 위한 Action
    public static Action<bool> OnLoginResult;
    public static Action<string> OnChatMsg;

    // [S_LOGIN_RES] 로그인 응답
    public static void S_LOGIN_RESHandler(ServerSession session, IMessage packet)
    {
        S_LOGIN_RES res = packet as S_LOGIN_RES;

        if (res.Success)
        {
            Debug.Log($"[Login Success] ID: {res.PlayerId}");
            // UI에게 "성공했다"고 알림
            if (OnLoginResult != null)
                OnLoginResult.Invoke(true);
        }
        else
        {
            Debug.Log($"[Login Failed] Check Server");
            if (OnLoginResult != null)
                OnLoginResult.Invoke(false);
        }
    }

    // [S_CHAT_RES] 내 채팅 전송 성공 여부
    public static void S_CHAT_RESHandler(ServerSession session, IMessage packet)
    {
        // 여기선 딱히 할 거 없음
    }

    // [S_CHAT_NTF] 남의 채팅 알림
    public static void S_CHAT_NTFHandler(ServerSession session, IMessage packet)
    {
        S_CHAT_NTF ntf = packet as S_CHAT_NTF;
        string finalMsg = $"[{ntf.Name}]: {ntf.Message}";

        Debug.Log(finalMsg);

        // UI 채팅창에 추가
        if (OnChatMsg != null)
            OnChatMsg.Invoke(finalMsg);
    }

    public static void S_HEART_BEAT_RESHandler(ServerSession session, IMessage packet)
    {
    }
}