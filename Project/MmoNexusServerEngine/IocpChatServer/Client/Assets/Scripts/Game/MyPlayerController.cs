using UnityEngine;
using Protocol;
using System.Collections;

public class MyPlayerController : MonoBehaviour
{
    float _speed = 5.0f;

    Vector3 _lastSentPos;
    float _lastSentYaw = 0f; // ✅ 마지막으로 보낸 Yaw

    MoveState _lastSentMoveState = MoveState.MoveIdle; // ✅ 마지막으로 보낸 이동 상태
    MoveState _curMoveState = MoveState.MoveIdle;      // ✅ 현재 입력 기반 이동 상태

    // ✅ 내 캐릭터 애니를 로컬에서 바로 갱신하기 위한 Animator 래퍼
    CreatureAnimator _anim;

    // [New] 상태 관리용
    bool _isDead = false;
    bool _isAttacking = false;

    void Start()
    {
        _anim = GetComponent<CreatureAnimator>(); // ✅ 추가

        _lastSentPos = transform.position;
        _lastSentYaw = transform.eulerAngles.y;

        StartCoroutine(CoSendPacket());

        // [New] 피격 이벤트 구독 (내가 죽었는지 확인)
        PacketHandler.OnChangeHp += OnChangeHp;
    }

    void OnDestroy()
    {
        PacketHandler.OnChangeHp -= OnChangeHp;
    }

    void OnChangeHp(S_CHANGE_HP pkt)
    {
        if (pkt.ObjectId == ObjectManager.MyPlayerId)
        {
            if (pkt.CurrentHp <= 0)
            {
                _isDead = true;
                Debug.Log("💀 [Die] You are dead!");
                _anim?.SetDead(); // ✅ 내 캐릭터도 즉시 죽음 애니 반영
            }
            else
            {
                // (선택) 맞을 때 히트 애니
                // _anim?.PlayHit();
            }
        }
    }

    void Update()
    {
        if (NetworkManager.Instance != null && NetworkManager.Instance.IsMapChanging)
            return;

        if (_isDead) return;

        // ============================================================
        // [ATTACK] 스페이스바 입력
        // ============================================================
        if (Input.GetKeyDown(KeyCode.Space) && _isAttacking == false)
        {
            Debug.Log("⚔️ [Input] Spacebar -> Attack!");

            // (선택) 로컬 선행 애니 (서버 승인 S_SKILL 도입 전까지는 체감용으로 OK)
            _anim?.PlayAttack(); // ✅ 추가

            C_SKILL skillPkt = new C_SKILL();
            skillPkt.SkillId = 1; // 1번 = 평타
            NetworkManager.Instance.Send(skillPkt, (ushort)PacketManager.MsgId.C_SKILL);
        }

        // ============================================================
        // [Item Test] E키
        // ============================================================
        if (Input.GetKeyDown(KeyCode.E))
        {
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

        // ============================================================
        // [Map Test] F1~F5
        // ============================================================
        if (Input.GetKeyDown(KeyCode.F1))
        {
            C_MAP_CHANGE_REQ req = new C_MAP_CHANGE_REQ();
            req.TargetMapId = 1;
            NetworkManager.Instance.Send(req, (ushort)PacketManager.MsgId.C_MAP_CHANGE_REQ);
            Debug.Log("📤 [TEST] Sent C_MAP_CHANGE_REQ targetMapId=1");
        }

        if (Input.GetKeyDown(KeyCode.F2))
        {
            C_MAP_CHANGE_REQ req = new C_MAP_CHANGE_REQ();
            req.TargetMapId = 2;
            NetworkManager.Instance.Send(req, (ushort)PacketManager.MsgId.C_MAP_CHANGE_REQ);
            Debug.Log("📤 [TEST] Sent C_MAP_CHANGE_REQ targetMapId=2");
        }

        if (Input.GetKeyDown(KeyCode.F3))
        {
            C_MAP_CHANGE_REQ req = new C_MAP_CHANGE_REQ();
            req.TargetMapId = 3;
            NetworkManager.Instance.Send(req, (ushort)PacketManager.MsgId.C_MAP_CHANGE_REQ);
            Debug.Log("📤 [TEST] Sent C_MAP_CHANGE_REQ targetMapId=3");
        }

        if (Input.GetKeyDown(KeyCode.F4))
        {
            C_MAP_CHANGE_REQ req = new C_MAP_CHANGE_REQ();
            req.TargetMapId = 4;
            NetworkManager.Instance.Send(req, (ushort)PacketManager.MsgId.C_MAP_CHANGE_REQ);
            Debug.Log("📤 [TEST] Sent C_MAP_CHANGE_REQ targetMapId=4");
        }

        if (Input.GetKeyDown(KeyCode.F5))
        {
            C_DUNGEON_EXIT_REQ req = new C_DUNGEON_EXIT_REQ();
            NetworkManager.Instance.Send(req, (ushort)PacketManager.MsgId.C_DUNGEON_EXIT_REQ);
            Debug.Log("📤 [TEST] Sent C_DUNGEON_EXIT_REQ");
        }

        // ============================================================
        // [Movement] 입력 기반으로 현재 MoveState 계산 (정지 상태도 계산해야 함)
        // ============================================================
        float h = Input.GetAxis("Horizontal");
        float v = Input.GetAxis("Vertical");

        bool moving = (Mathf.Abs(h) > 0.001f || Mathf.Abs(v) > 0.001f);
        _curMoveState = moving ? MoveState.MoveRun : MoveState.MoveIdle;

        // ✅ 내 캐릭터도 로컬에서 바로 애니 반영
        _anim?.SetMoveState(_curMoveState);

        // 실제 위치 이동은 moving일 때만
        if (!moving) return;

        Vector3 dir = new Vector3(h, 0, v).normalized;
        transform.position += dir * _speed * Time.deltaTime;

        if (dir != Vector3.zero)
            transform.rotation = Quaternion.LookRotation(dir);
    }

    IEnumerator CoSendPacket()
    {
        WaitForSeconds tick = new WaitForSeconds(0.2f);

        while (true)
        {
            yield return tick;

            if (NetworkManager.Instance != null && NetworkManager.Instance.IsMapChanging)
                continue;

            if (_isDead) continue;

            bool posChanged = Vector3.Distance(transform.position, _lastSentPos) > 0.1f;
            bool stateChanged = (_curMoveState != _lastSentMoveState);

            float curYaw = transform.eulerAngles.y;
            bool yawChanged = Mathf.Abs(Mathf.DeltaAngle(curYaw, _lastSentYaw)) > 2.0f; // ✅ 회전만 해도 전송

            // ✅ 위치/상태/방향 중 하나라도 바뀌면 전송
            if (posChanged || stateChanged || yawChanged)
            {
                C_MOVE movePkt = new C_MOVE();
                movePkt.PosInfo = new PositionInfo();
                movePkt.PosInfo.X = transform.position.x;
                movePkt.PosInfo.Y = transform.position.y;
                movePkt.PosInfo.Z = transform.position.z;
                movePkt.PosInfo.Yaw = curYaw;
                movePkt.PosInfo.State = _curMoveState;

                NetworkManager.Instance.Send(movePkt, (ushort)PacketManager.MsgId.C_MOVE);

                _lastSentPos = transform.position;
                _lastSentMoveState = _curMoveState;
                _lastSentYaw = curYaw; // ✅ 갱신
            }
        }
    }
}
