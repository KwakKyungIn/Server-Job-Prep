## **Section 2: 핵심 차별화 기능 딥다이브** (16페이지)

### **2-1. 서버 권위 이동 검증** (3페이지)

**Page 6 - 문제 정의: 서버 권위 이동이 필요한 이유**
```text
핵심 메시지
- "이동은 클라이언트 입력일 뿐, 위치 확정은 서버 책임"이어야 한다.

클라이언트 신뢰 시 실제로 발생하는 문제
1) 속도핵/텔레포트
- 클라이언트가 과도한 delta를 보내면 짧은 시간에 비정상 거리 이동 가능
2) 벽 통과/비정상 지형 진입
- 충돌 지오메트리 무시 좌표를 보내도 신뢰하면 바로 반영됨
3) 시퀀스 역전/재전송 패킷
- 네트워크 지연/재정렬 상황에서 과거 위치가 현재 상태를 덮어씀

설계 목표
- 서버가 seq, dt, 속도, 지형 충돌을 검증한 뒤 최종 좌표만 커밋
- 비정상 요청은 drop 또는 보정(slide/clamp) 처리
- 확정된 결과만 AOI 대상에게 S_MOVE로 전파

제출본에 첨부할 근거(페이지 6 하단 박스)
- `Handle_C_MOVE` 진입점과 RoomActor 직렬 처리 구조
- `MoveValidate::IsSeqNewer`, `ComputeDtSec`, `CheckSpeed2D` 호출 흐름
- `GameMap::ValidateMove`를 통한 NavMesh 기반 충돌/보정 분기
```

**Page 7 - 이동 검증 파이프라인**
- Mermaid 다이어그램 필요
```text
파이프라인 개요(코드 순서 기준)
1) `Handle_C_MOVE` 수신
- 클라이언트 패킷을 RoomActor JobQueue에 enqueue (동일 룸 직렬화 보장)

2) 입력 안정성 검사
- NaN/Inf, 비정상 좌표 범위(crazy position) 즉시 거부

3) 시간/순서 검증
- 마지막 처리 seq보다 과거면 drop (`IsSeqNewer`)
- 서버 기준 dt 계산 (`ComputeDtSec`) 후 과대 dt/음수 dt 방어

4) 속도 검증
- `CheckSpeed2D`로 최대 이동 속도 초과 여부 판단
- 초과 시 drop 또는 허용 반경으로 clamp

5) 지형/충돌 검증
- `GameMap::ValidateMove`에서 NavMesh 경로, raycast, 슬라이딩 보정 수행
- 통과 불가 지점은 직전 유효 위치 또는 보정 좌표로 대체

6) 상태 확정 및 전파
- 서버 좌표 commit + zone/AOI 갱신
- 확정 결과만 주변 클라이언트에 `S_MOVE` broadcast
- 마지막 move stamp(seq/time/pos) 갱신

--> 이거를 2페이지로 나누고, 한 페이지에 검증 흐름 절반과 파이프라인 개요 넣고, 아래 공간에 코드 스니펫 추가
```

**Page 8 - 검증 결과(로그/좌표 보정 사례)**
```text
[작성 제외: 사용자 직접 제작]
- 실제 운영/테스트 로그 캡처
- 요청 좌표 vs 서버 확정 좌표 before/after 비교
- "비정상 이동 정규화" 메시지 1줄
```

### **2-2. AOI 최적화 + 스냅샷 배칭** (3페이지)

**Page 9 - 문제 정의: 전체 브로드캐스트 한계**
```text
문제
- 엔티티 증가 시 패킷량 급증
- CPU/네트워크 낭비

해결 방향
- 관심 영역(AOI) 기반 전송
```

**Page 10 - SpatialGrid + Connectivity 필터**
- Mermaid 다이어그램 필요
```text
맵 Grid 분할 → 주변 Zone 후보 수집
  ↓
거리 + Connectivity 필터
  ↓
가시 목록 확정

핵심
- 전송 대상을 구조적으로 축소
```

**Page 11 - 스냅샷 배칭 전송 결과**
```text
입장 시 대량 스폰 동기화
- S_SPAWN(snapshot_begin)
- S_SPAWN(entities batch ...)
- S_SPAWN(snapshot_end)

효과
- 소켓 버스트 완화
- 초기 입장 안정성 향상
```

### **2-3. 전투/투사체 서버 판정** (4페이지)

**Page 12 - 스킬 판정 구조 (즉발/범위/부채꼴 분기)**
- Mermaid 다이어그램 필요
```text
BattleSystem::ResolveSkill 분기
- 즉발(SKILL_AUTO): CheckCircle + 단일 타겟 처리
- 원형 범위(SKILL_AREA_CIRCLE): CheckCircle
- 부채꼴(SKILL_AREA_CONE): CheckFan(range/angle)

결과 전파
- S_SKILL, S_CHANGE_HP

코드 근거
- GameServer/BattleSystem.cpp
- GameServer/GameRoom.Combat.cpp
```

**Page 13 - 투사체 시뮬레이션 (충돌/NavMesh 레이캐스트)**
- Mermaid 다이어그램 필요
```text
서버 Tick 기반 업데이트
- oldPos→newPos 이동 + RaycastNav 벽 충돌 보정
- SegmentCircleHitXZ 선분-원 충돌 판정
- 충돌 시점(t) 정렬 후 순차 타격
- stopOnHit/maxHits로 관통/비관통 분기

코드 근거
- GameServer/GameRoom.Projectile.cpp
```

**Page 14 - 쿨타임 서버 검증 + 중복 타격 방지**
```text
쿨타임 검증 경로
C_SKILL → GameRoom::HandleSkill
  1) attacker->CanUseSkill(skillId) 실패 시 드롭
  2) 판정 성공/투사체 생성 시 StartSkillCooldown(skillId, cooldown)

중복 타격 방지 (hitTargets Set 개념)
- 구현체: Projectile::_hitVictims(HashSet)
- HasAlreadyHit(victimNetId)로 중복 피격 차단
- MarkHit() 기록 후 StopOnHit/MaxHits 조건으로 소멸 분기

코드 근거
- GameServer/Creature.cpp
- GameServer/GameRoom.Combat.cpp
- GameServer/Projectile.h
- GameServer/GameRoom.Projectile.cpp
```

**Page 15 - 전투 판정 + 몬스터 AI + 드랍 처리 흐름**
- Mermaid 다이어그램 필요
```text
End-to-End 전투 흐름
1) 피격: BattleSystem/Projectile에서 OnDamaged 호출
2) AI 반응: Monster FSM(Idle→Chase→Attack→Return) 전이
3) 사망: Monster::OnDead → GameRoom::HandleMonsterDead
4) 보상: EXP 지급 + DropTable 롤링 + AutoLoot
5) 정리: LeaveMonster + Spawn 타이머 재가동

코드 근거
- GameServer/Monster.cpp
- GameServer/GameRoom.Items.cpp
- GameServer/GameRoom.LifeTime.cpp
```

### **2-4. 파티/인스턴스/맵 전이** (3페이지)

**Page 16 - 맵 변경 핸드셰이크**
- Mermaid 다이어그램 필요
```text
C_MAP_CHANGE_REQ
  ↓
MapChangeToken 발급 + MapChanging 상태 전이
  ↓
S_MAP_CHANGE_BEGIN
  ↓
C_MAP_CHANGE_ACK(token) 검증
  ↓
룸 이동 + S_MAP_CHANGE_END
```

**Page 17 - 파티 구조와 정합성 제어**
```text
PartyActor + JobQueue 직렬화
- 생성/초대/수락/강퇴/해산
- 이름 인덱스 기반 초대 대상 해석
- 초대 TTL(60초) 만료 제어
- 파티 채팅/상태 스냅샷 전파
```

**Page 18 - 인스턴스 던전 생명주기**
- Mermaid 다이어그램 필요
```text
RoomKey(channelId, mapId, instanceId)
- 생성 → 입장 → 플레이 → 종료 → Purge
- 파티 메타(instanceId/dungeonState) 연동
- 강퇴/해산/종료 시 안전 귀환 처리
```

### **2-5. 거래 원자성 보장** (3페이지)

**Page 19 - 거래 상태머신**
- Mermaid 다이어그램 필요
```text
REQ → INVITE_RESP → OFFER_SET
→ READY(양측) → LOCKED → CONFIRM(양측)

검증
- 거리/대상/중복 거래/아이템 상태
- 맵 이동/접속 종료 시 자동 취소
```

**Page 20 - 2-Phase Commit**
- Mermaid 다이어그램 필요
```text
Phase 1 (GameServer)
- 인벤/골드 시뮬레이션
- 최종 스냅샷 생성

Phase 2 (DBAgent)
- BEGIN TRAN
- DELETE/UPSERT/UPDATE
- COMMIT

성공 시 메모리 + Redis 반영
```

**Page 21 - 거래 검증 결과**
```text
테스트 케이스
- 인벤 부족: Phase1 차단
- DB 실패: 트랜잭션 롤백
- 동시 요청: JobQueue 직렬화로 정합성 유지
- 골드 음수/오버플로우: 사전 차단
```

---

