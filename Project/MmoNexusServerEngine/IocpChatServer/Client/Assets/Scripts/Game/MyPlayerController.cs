using UnityEngine;
using Protocol;
using System.Collections;
using System.Collections.Generic; // [필수] Dictionary 사용을 위해 추가

public class MyPlayerController : MonoBehaviour
{
    float _speed = 5.0f;
    Vector3 _lastSentPos;

    void Start()
    {
        StartCoroutine(CoSendPacket());
    }

    void Update()
    {
        // ============================================================
        // [TEST] E키를 누르면 인벤토리의 첫 번째 아이템을 장착/해제한다.
        // ============================================================
        if (Input.GetKeyDown(KeyCode.E))
        {
            var allItems = InventoryManager.Instance.GetAllItems();

            if (allItems.Count > 0)
            {
                // 1. 첫 번째 아이템 찾기 (Dictionary라 순서는 보장 안 되지만 테스트용으론 충분)
                var enumerator = allItems.GetEnumerator();
                enumerator.MoveNext();
                ItemInfo item = enumerator.Current.Value;

                Debug.Log($"[Test] Request Equip/Unequip Item: {item.TemplateId} (UID: {item.ItemUid}) CurrentState: {item.IsEquipped}");

                // 2. 패킷 생성
                C_EQUIP_ITEM pkt = new C_EQUIP_ITEM();
                pkt.ItemUid = item.ItemUid;
                pkt.SlotIndex = item.Slot;
                pkt.Equip = !item.IsEquipped; // 토글 (장착 중이면 해제, 아니면 장착)

                // 3. 전송
                NetworkManager.Instance.Send(pkt, (ushort)PacketManager.MsgId.C_EQUIP_ITEM);
            }
            else
            {
                Debug.Log("[Test] 인벤토리가 비어있습니다. DB에 아이템을 넣었나요?");
            }
        }

        // ============================================================
        // [Input Handling] (기존 이동 로직)
        // ============================================================
        float h = Input.GetAxis("Horizontal");
        float v = Input.GetAxis("Vertical");

        // 입력이 없으면 패스 (E키 체크는 위에서 했으니 리턴해도 됨)
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

                NetworkManager.Instance.Send(movePkt, (ushort)PacketManager.MsgId.C_MOVE);

                _lastSentPos = transform.position;
            }
        }
    }
}