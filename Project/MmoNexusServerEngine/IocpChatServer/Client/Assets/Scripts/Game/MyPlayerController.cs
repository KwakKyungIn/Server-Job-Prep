using UnityEngine;
using Protocol;
using System.Collections;
using UnityEngine.AI;
using UnityEngine.EventSystems;
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
    // ✅ [ADD] 카메라 기준 이동을 위한 FollowCamera 참조
    public FollowCamera followCam;

    // ✅ [Trade RMB Click vs Drag]
    // - RMB 드래그는 카메라 회전(orbit)로 쓰고
    // - RMB 짧은 클릭만 거래 요청으로 처리
    const float TRADE_RMB_CLICK_MAX_DRAG_PX = 6f;
    bool _tradeRmbCandidate = false;
    Vector3 _tradeRmbDownPos;

    uint _moveSeq = 0;

    // 권장 튜닝값
    const float MOVE_SEND_HZ = 20f;         // 20Hz (0.05s)
    const float SELF_SNAP_DIST = 0.75f;     // 이 이상 차이면 스냅
    const float SELF_LERP_FACTOR = 0.50f;   // 작게 차이면 절반만 따라가기

    void Start()
    {
        _anim = GetComponent<CreatureAnimator>(); // ✅ 추가

        _lastSentPos = transform.position;
        _lastSentYaw = transform.eulerAngles.y;

        // ✅ [ADD] 자동 바인딩 (인스펙터에서 안 넣어도 동작)
        if (followCam == null && Camera.main != null)
            followCam = Camera.main.GetComponent<FollowCamera>();

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

        if (UI_ChatPanel.IsTyping)
        {
            // ✅ 채팅 중엔 게임 입력 전부 봉쇄 (이동/공격/테스트키 등)
            _curMoveState = MoveState.MoveIdle;
            _anim?.SetMoveState(_curMoveState);
            return;
        }

        // ============================================================
        // [RIGHT CLICK -> TRADE REQUEST]
        // ============================================================
        // - 우클릭 드래그는 카메라 회전(orbit)
        // - 우클릭 짧은 클릭만 거래 신청
        // - UI 위 클릭은 무시
        if (Input.GetMouseButtonDown(1))
        {
            bool overUI = (EventSystem.current != null) && EventSystem.current.IsPointerOverGameObject();
            if (overUI)
            {
                _tradeRmbCandidate = false;
            }
            else
            {
                _tradeRmbCandidate = true;
                _tradeRmbDownPos = Input.mousePosition;
            }
        }

        if (_tradeRmbCandidate && Input.GetMouseButton(1))
        {
            // 드래그로 판정되면 카메라 회전 의도로 보고 trade 후보를 버린다
            float drag = (Input.mousePosition - _tradeRmbDownPos).magnitude;
            if (drag >= TRADE_RMB_CLICK_MAX_DRAG_PX)
                _tradeRmbCandidate = false;
        }

        if (Input.GetMouseButtonUp(1))
        {
            bool overUI = (EventSystem.current != null) && EventSystem.current.IsPointerOverGameObject();
            if (_tradeRmbCandidate && !overUI)
            {
                float drag = (Input.mousePosition - _tradeRmbDownPos).magnitude;
                if (drag < TRADE_RMB_CLICK_MAX_DRAG_PX)
                    TryRequestTradeByRightClick();
            }
            _tradeRmbCandidate = false;
        }
        // ============================================================
        // [ATTACK]
        // ============================================================
        if (Input.GetKeyDown(KeyCode.Space))
        {
            TryCastSkill(1, includeYaw: false);
        }

        // ============================================================
        // [PROJECTILE TEST] Q -> SkillId=2
        // ============================================================
        if (Input.GetKeyDown(KeyCode.Q))
        {
            TryCastSkill(2, includeYaw: true);
        }

        // ============================================================
        // [Item Test] E
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
        // [Movement]
        // ============================================================
        float h = Input.GetAxis("Horizontal");
        float v = Input.GetAxis("Vertical");

        bool moving = (Mathf.Abs(h) > 0.001f || Mathf.Abs(v) > 0.001f);
        _curMoveState = moving ? MoveState.MoveRun : MoveState.MoveIdle;
        _anim?.SetMoveState(_curMoveState);

        if (!moving) return;

        // ============================================================
        // [Direction] camera-based input direction
        // ============================================================
        Vector3 inputDir;
        if (followCam != null)
            inputDir = (followCam.GetPlanarForward() * v + followCam.GetPlanarRight() * h);
        else if (Camera.main != null)
        {
            Vector3 f = Camera.main.transform.forward; f.y = 0f; f.Normalize();
            Vector3 r = Camera.main.transform.right; r.y = 0f; r.Normalize();
            inputDir = (f * v + r * h);
        }
        else
            inputDir = new Vector3(h, 0, v);

        if (inputDir.sqrMagnitude < 0.0001f) return;
        inputDir.Normalize();

        // ✅ 회전은 "입력 방향"으로만 고정 (슬라이드 방향으로 안 돌림)
        transform.rotation = Quaternion.LookRotation(inputDir);

        // ============================================================
        // [Slide Move] (감속 없음)
        // ============================================================
        Vector3 curPos = transform.position;

        Vector3 moveDelta = inputDir * _speed * Time.deltaTime;
        Vector3 nextPos = curPos + moveDelta;

        NavMeshHit hit;
        if (NavMesh.Raycast(curPos, nextPos, out hit, NavMesh.AllAreas))
        {
            // ✅ 접선 방향(슬라이딩 방향)
            Vector3 slideDir = Vector3.ProjectOnPlane(inputDir, hit.normal);
            slideDir.y = 0f;

            if (slideDir.sqrMagnitude < 0.0001f)
            {
                // 정면충돌이면 hit 지점까지만
                transform.position = hit.position;
                return;
            }

            slideDir.Normalize();

            // ✅ 같은 거리 그대로 옆으로 이동 (감속 X)
            Vector3 slidePos = curPos + slideDir * moveDelta.magnitude;

            // (선택) NavMesh 스냅
            NavMeshHit snap;
            if (NavMesh.SamplePosition(slidePos, out snap, 0.5f, NavMesh.AllAreas))
                transform.position = snap.position;
            else
                transform.position = slidePos;

            return;
        }

        transform.position = nextPos;
    }

    void TryRequestTradeByRightClick()
    {
        if (TradeManager.Instance != null)
            TradeManager.Instance.Init();

        if (TradeManager.Instance != null && TradeManager.Instance.InTrade)
            return;

        var cam = Camera.main;
        if (cam == null) return;

        Ray ray = cam.ScreenPointToRay(Input.mousePosition);
        if (!Physics.Raycast(ray, out RaycastHit hit, 200f))
            return;

        var ident = hit.collider != null ? hit.collider.GetComponentInParent<PlayerIdentity>() : null;
        if (ident == null) return;
        if (ident.IsMine) return;
        if (ident.PlayerId == 0) return;

        Debug.Log($"🤝 [Trade] RightClick -> RequestTrade target={ident.PlayerName}({ident.PlayerId})");
        TradeManager.Instance.RequestTrade(ident.PlayerId);
    }

    IEnumerator CoSendPacket()
    {
        WaitForSecondsRealtime tick = new WaitForSecondsRealtime(1f / MOVE_SEND_HZ);

        while (true)
        {
            yield return tick;

            if (NetworkManager.Instance != null && NetworkManager.Instance.IsMapChanging)
                continue;

            if (_isDead) continue;

            bool posChanged = Vector3.Distance(transform.position, _lastSentPos) > 0.05f; // 0.1 -> 0.05 권장
            bool stateChanged = (_curMoveState != _lastSentMoveState);

            float curYaw = transform.eulerAngles.y;
            bool yawChanged = Mathf.Abs(Mathf.DeltaAngle(curYaw, _lastSentYaw)) > 2.0f;

            if (posChanged || stateChanged || yawChanged)
            {
                // ✅ move_seq: 보내는 순간에만 +1
                unchecked { _moveSeq++; }

                // ✅ client_time_ms: 단조 증가 ms
                uint nowMs = (uint)(Time.realtimeSinceStartupAsDouble * 1000.0);

                C_MOVE movePkt = new C_MOVE();
                movePkt.PosInfo = new PositionInfo();
                movePkt.PosInfo.X = transform.position.x;
                movePkt.PosInfo.Y = transform.position.y;
                movePkt.PosInfo.Z = transform.position.z;
                movePkt.PosInfo.Yaw = curYaw;
                movePkt.PosInfo.State = _curMoveState;

                // ✅ v2 필드 주입
                movePkt.MoveSeq = _moveSeq;
                movePkt.ClientTimeMs = nowMs;

                NetworkManager.Instance.Send(movePkt, (ushort)PacketManager.MsgId.C_MOVE);

                _lastSentPos = transform.position;
                _lastSentMoveState = _curMoveState;
                _lastSentYaw = curYaw;
            }
        }
    }

    public void ApplyServerMove(Vector3 serverPos, float serverYaw, MoveState serverState)
    {
        // 내 클라 예측 위치 vs 서버 권위
        float dist = Vector3.Distance(transform.position, serverPos);

        if (dist > SELF_SNAP_DIST)
        {
            // 큰 차이 = 스냅
            transform.position = serverPos;
        }
        else if (dist > 0.01f)
        {
            // 작은 차이 = 절반만 따라가기 (간단 스무딩)
            transform.position = Vector3.Lerp(transform.position, serverPos, SELF_LERP_FACTOR);
        }

        // 회전도 권위 반영(간단 스냅)
        transform.rotation = Quaternion.Euler(0f, serverYaw, 0f);

        // (선택) 애니도 서버 state로 맞추고 싶으면
        // _anim?.SetMoveState(serverState);

        // ✅ 중요: 서버가 텔레포트/보정한 뒤에, 다음 C_MOVE가 “대이동”으로 나가지 않게 기준점 갱신
        _lastSentPos = transform.position;
        _lastSentYaw = serverYaw;
        _lastSentMoveState = serverState;
    }

    public void ResetSendBaseline()
    {
        _lastSentPos = transform.position;
        _lastSentYaw = transform.eulerAngles.y;
        _lastSentMoveState = _curMoveState;
    }



    // ============================================================
    // [SKILL CAST] Client-side cooldown gate
    // - Client should NOT play attack animation or send C_SKILL while cooldown is active.
    // - Animation is played when S_SKILL arrives (server accepted).
    // ============================================================
    void TryCastSkill(int skillId, bool includeYaw)
    {
        if (NetworkManager.Instance != null && NetworkManager.Instance.IsMapChanging)
            return;

        if (SkillCooldownManager.Instance != null && SkillCooldownManager.Instance.IsOnCooldown(skillId))
        {
            Debug.Log($"[Skill] Blocked by cooldown. skillId={skillId}");
            return;
        }

        C_SKILL pkt = new C_SKILL();
        pkt.SkillId = skillId;
        if (includeYaw)
            pkt.CastYaw = transform.eulerAngles.y;
        pkt.ClientTimeMs = (uint)(Time.realtimeSinceStartupAsDouble * 1000.0);

        NetworkManager.Instance.Send(pkt, (ushort)PacketManager.MsgId.C_SKILL);
    }
}
