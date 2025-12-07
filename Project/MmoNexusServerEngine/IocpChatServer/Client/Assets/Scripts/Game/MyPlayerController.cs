using UnityEngine;
using Protocol;
using System.Collections;

public class MyPlayerController : MonoBehaviour
{
    float _speed = 5.0f;
    Vector3 _lastSentPos; // 마지막으로 서버에 보낸 위치

    void Start()
    {
        StartCoroutine(CoSendPacket());
    }

    void Update()
    {
        // [Input Handling]
        float h = Input.GetAxis("Horizontal");
        float v = Input.GetAxis("Vertical");

        if (h == 0 && v == 0) return;

        // [Movement]
        Vector3 dir = new Vector3(h, 0, v).normalized;
        transform.position += dir * _speed * Time.deltaTime;

        // [Rotation]
        if (dir != Vector3.zero)
        {
            transform.rotation = Quaternion.LookRotation(dir);
        }
    }

    // [Network] 0.2초마다 위치 패킷 전송 (너무 자주 보내면 대역폭 낭비)
    // [Network] 0.2초마다 위치 패킷 전송
    IEnumerator CoSendPacket()
    {
        while (true)
        {
            yield return new WaitForSeconds(0.2f);

            if (Vector3.Distance(transform.position, _lastSentPos) > 0.1f)
            {
                C_MOVE movePkt = new C_MOVE();
                movePkt.PosInfo = new PositionInfo();
                movePkt.PosInfo.X = transform.position.x;
                movePkt.PosInfo.Y = transform.position.y;
                movePkt.PosInfo.Z = transform.position.z;
                movePkt.PosInfo.Yaw = transform.eulerAngles.y;
                movePkt.PosInfo.State = MoveState.MoveRun;

                // [FIX] 패킷 ID ((ushort)PacketManager.MsgId.C_MOVE)를 두 번째 인자로 추가
                NetworkManager.Instance.Send(movePkt, (ushort)PacketManager.MsgId.C_MOVE);

                _lastSentPos = transform.position;
            }
        }
    }
}