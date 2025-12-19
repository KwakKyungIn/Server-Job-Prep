using UnityEngine;
using Protocol;
using System.Collections;
using System.Collections.Generic;

public class MyPlayerController : MonoBehaviour
{
    float _speed = 5.0f;
    Vector3 _lastSentPos;

    // [New] 상태 관리용
    bool _isDead = false;
    bool _isAttacking = false;

    void Start()
    {
        StartCoroutine(CoSendPacket());

        // [New] 피격 이벤트 구독 (내가 죽었는지 확인)
        PacketHandler.OnChangeHp += OnChangeHp;
    }

    // [New] 이벤트 해제 (습관 들이기)
    void OnDestroy()
    {
        PacketHandler.OnChangeHp -= OnChangeHp;
    }

    // [New] 피격 핸들러
    void OnChangeHp(S_CHANGE_HP pkt)
    {
        // 내 아이디랑 맞는지 확인
        if (pkt.ObjectId == ObjectManager.MyPlayerId)
        {
            if (pkt.CurrentHp <= 0)
            {
                _isDead = true;
                Debug.Log("💀 [Die] You are dead!");
                // 여기서 애니메이션 처리 (GetComponent<Animator>().SetTrigger("Die");)
            }
        }
    }

    void Update()
    {

        if (NetworkManager.Instance != null && NetworkManager.Instance.IsMapChanging)
            return;


        // [New] 죽었으면 조작 불가
        if (_isDead) return;

        // ============================================================
        // [ATTACK] 스페이스바 입력
        // ============================================================
        if (Input.GetKeyDown(KeyCode.Space) && _isAttacking == false)
        {
            Debug.Log("⚔️ [Input] Spacebar -> Attack!");

            C_SKILL skillPkt = new C_SKILL();
            skillPkt.SkillId = 1; // 1번 = 평타

            // 패킷 전송
            NetworkManager.Instance.Send(skillPkt, (ushort)PacketManager.MsgId.C_SKILL);

            // (선택) 클라단 쿨타임 처리나 애니메이션 선행 실행
            // _isAttacking = true;
            // Invoke("ResetAttack", 0.5f); 
        }

        // ============================================================
        // [Item Test] E키
        // ============================================================
        if (Input.GetKeyDown(KeyCode.E))
        {
            // ... (기존 아이템 장착 로직 유지) ...
            var allItems = InventoryManager.Instance.GetAllItems();
            if (allItems.Count > 0)
            {
                var enumerator = allItems.GetEnumerator();
                enumerator.MoveNext();
                ItemInfo item = enumerator.Current.Value;
                C_EQUIP_ITEM pkt = new C_EQUIP_ITEM();
                pkt.ItemUid = item.ItemUid;
                pkt.SlotIndex = item.Slot;
                pkt.Equip = !item.IsEquipped;
                NetworkManager.Instance.Send(pkt, (ushort)PacketManager.MsgId.C_EQUIP_ITEM);
            }
        }

        //임시 테스트 코드
        if (Input.GetKeyDown(KeyCode.F1))
        {
            C_MAP_CHANGE_REQ req = new C_MAP_CHANGE_REQ();
            req.TargetMapId = 1; // 테스트용

            NetworkManager.Instance.Send(req, (ushort)PacketManager.MsgId.C_MAP_CHANGE_REQ);
            Debug.Log("📤 [TEST] Sent C_MAP_CHANGE_REQ targetMapId=1");
        }

        if (Input.GetKeyDown(KeyCode.F2))
        {
            C_MAP_CHANGE_REQ req = new C_MAP_CHANGE_REQ();
            req.TargetMapId = 2; // 테스트용

            NetworkManager.Instance.Send(req, (ushort)PacketManager.MsgId.C_MAP_CHANGE_REQ);
            Debug.Log("📤 [TEST] Sent C_MAP_CHANGE_REQ targetMapId=2");
        }
        if (Input.GetKeyDown(KeyCode.F3))
        {
            C_MAP_CHANGE_REQ req = new C_MAP_CHANGE_REQ();
            req.TargetMapId = 3; // 테스트용

            NetworkManager.Instance.Send(req, (ushort)PacketManager.MsgId.C_MAP_CHANGE_REQ);
            Debug.Log("📤 [TEST] Sent C_MAP_CHANGE_REQ targetMapId=3");
        }
        if (Input.GetKeyDown(KeyCode.F4))
        {
            C_MAP_CHANGE_REQ req = new C_MAP_CHANGE_REQ();
            req.TargetMapId = 4; // 테스트용

            NetworkManager.Instance.Send(req, (ushort)PacketManager.MsgId.C_MAP_CHANGE_REQ);
            Debug.Log("📤 [TEST] Sent C_MAP_CHANGE_REQ targetMapId=4");
        }


        // ============================================================
        // [Movement]
        // ============================================================
        float h = Input.GetAxis("Horizontal");
        float v = Input.GetAxis("Vertical");

        if (h == 0 && v == 0) return;

        Vector3 dir = new Vector3(h, 0, v).normalized;
        transform.position += dir * _speed * Time.deltaTime;

        if (dir != Vector3.zero)
        {
            transform.rotation = Quaternion.LookRotation(dir);
        }
    }

    IEnumerator CoSendPacket()
    {


        while (true)
        {
            if (NetworkManager.Instance != null && NetworkManager.Instance.IsMapChanging)
                continue;


            yield return new WaitForSeconds(0.2f);

            // 죽었으면 이동 패킷 안 보냄
            if (_isDead) continue;

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