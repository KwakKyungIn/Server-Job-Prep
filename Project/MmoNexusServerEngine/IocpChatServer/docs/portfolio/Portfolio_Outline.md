# 📘 포트폴리오 제출본 구성 (37페이지)

> 본 문서는 발표용이 아닌 제출용 포트폴리오입니다.
> 목적: 문서만 읽어도 구조/구현/검증이 전달되는 제출용 포트폴리오
> 범위: GameServer / LoginServer / DBAgent / ServerCore / Common(Protobuf) / Tools(PacketGenerator)
> 제외: ChatServer (포트폴리오 범위에서 미사용)
> 기준 문서: `docs/portfolio/Portfolio_FeatureList.md`, `docs/portfolio/Portfolio_Techniques.md`
> 구성 원칙: 구현 완료 항목만 반영
> 핵심 축: Server Authority / Concurrency / Data Integrity / Observability / Load Validation

---

## 제출본 작성 기준 (페이지 공통)

- 각 페이지는 `문제 → 구현 → 근거 코드 → 검증 결과` 순서로 읽히게 구성
- 현장 설명 전제 표현(구어체 문장) 제거
- 수치/로그/지표는 캡처 또는 표 형태로 첨부 가능한 항목 위주로 배치
- 모든 핵심 페이지에 코드 근거 경로를 하단 캡션으로 표기

---

## 배치 기준 (제출 심사 관점)

- **Section 2(핵심 차별화)**: 구현 임팩트가 큰 기능(이동/AOI/전투/인스턴스/거래)을 앞에 배치
- **Section 3(엔진 설계)**: 기능이 안정적으로 동작하는 이유(IOCP/JobQueue/패킷/세션)를 구조적으로 배치
- **Section 4(데이터 정합성)**: 실무형 역량(영속화/트랜잭션/UID 충돌 방지)을 독립 섹션으로 배치
- **Section 5(운영/검증)**: 운영 가능성(메트릭/대시보드)과 재현 가능 검증(더미 부하)을 후반 배치
- **Section 6(신뢰성/마무리)**: 실제 장애 대응 사례와 결과 요약으로 마무리

---

## **Preview: 면접관용 1페이지 요약본** (1페이지)

**Page 1 - 표지 + 프로젝트 30초 요약**
```text
MMORPG 서버 포트폴리오
- IOCP 기반 멀티 서버 구조
- 서버 권위 게임 서버 (C++/Redis/ODBC)

한 줄 요약
"서버 권위 전투/이동/거래를 중심으로 MMORPG 핵심 경로를 직접 구현한 IOCP 멀티 서버"

핵심 메시지
- 클라이언트 신뢰를 배제하고 서버가 전투/이동/거래 결과를 최종 확정
- 정합성, 영속화, 모니터링, 부하검증까지 실서비스 관점으로 완결

[이름 / 연락처 / GitHub]
```

---

**Page 2 - 목차**
```text
Preview
- 표지 + 프로젝트 30초 요약

Section 1. 개요 & 범위 정의
- 프로젝트 한 페이지 요약 (차별화/증거 매핑)
- 시스템 아키텍처
- End-to-End 데이터 흐름

Section 2. 핵심 차별화 기능 딥다이브
- 서버 권위 이동 검증
- AOI SpatialGrid + 스냅샷 배칭
- 전투/투사체 서버 판정 + 몬스터 AI/드랍 흐름
- 파티/인스턴스/맵 전이
- 거래 상태머신 + 2-Phase Commit

Section 3. 서버 엔진 설계
- Actor + JobQueue 직렬화 모델
- Hybrid 스레드 모델 + IOCP 네트워크
- 패킷 파이프라인
- 로비 데이터 로딩 게이트 + 세션 생명주기 관리

Section 4. 데이터 정합성 & 영속화
- Redis 토큰 인증 + 세션 바인딩
- Redis Write-Back + AutoCommit
- DBAgent 트랜잭션 경로
- 부팅 데이터 동기화 + GameItem UID 시드 부트스트랩

Section 5. 운영 관측 & 성능 검증
- 메모리/송신 성능 가드레일
- Prometheus + Grafana 관측 체계
- DummyClient 부하 검증 보고

Section 6. 신뢰성 사례 & 마무리
- 안정성 도구 + 트러블슈팅
- 역량 요약 & 참고 링크
```

---

## **Section 1: 개요 & 범위 정의** (3페이지)

**Page 3 - 프로젝트 한 페이지 요약 (정리본)**
```text
프로젝트 한 줄 요약
"실서비스형 MMORPG 서버 핵심 경로를 직접 구현"

핵심 차별화 5개
1) 서버 권위 이동 검증 + NavMesh 슬라이딩
2) AOI SpatialGrid + 스냅샷 배칭
3) 1:1 거래 2-Phase Commit (아이템/골드 원자성)
4) Redis Write-Back + AutoCommit + 종료 Flush
5) Prometheus/Grafana + DummyClient 부하 검증
```

**Page 4 - 시스템 아키텍처**
- Mermaid 다이어그램 필요
```text
[다이어그램]
Client ↔ LoginServer
        ↘ Redis (Token/Cache)
Client ↔ GameServer ↔ DBAgent ↔ RDBMS(ODBC)

컴포넌트 책임
- LoginServer: C_LOGIN 처리 + Redis 토큰 발급(TTL 300초)
- GameServer: 입장/플레이 서버 권위 판정 + Redis Dirty 반영
- DBAgent: ODBC Connection Pool + BEGIN/COMMIT 트랜잭션 전담

핵심 경로 요약
- 인증/입장: C_LOGIN → S_LOGIN(token) → C_ENTER_GAME(token) → 월드 입장
- 저장 경로: S2S_REQ_SAVE_* → DBAgent 트랜잭션 커밋

코드 근거
- LoginServer/LoginServer.cpp
- LoginServer/S2SPacketHandler.cpp
- GameServer/GameServer.cpp
- GameServer/AutoCommitService.cpp
- DBAgent/DBAgentPacketHandler.cpp
```

**Page 5 - End-to-End 데이터 흐름**
- Mermaid 다이어그램 필요
```text
Login: C_LOGIN → S2S_REQ_LOGIN → S_LOGIN(token)
Enter: C_ENTER_GAME → 토큰 검증 → 로비 로딩 게이트
Play : 이동/전투/거래/파티/던전
Save : Redis Dirty 누적 → AutoCommit → DB 저장
Exit : Disconnect 시 Flush + 인스턴스 정리

주요 저장 이벤트
- S2S_REQ_SAVE_PLAYER_CORE
- S2S_REQ_SAVE_INVENTORY
- S2S_REQ_SAVE_QUICKSLOT
- S2S_REQ_TRADE_COMMIT / S2S_RES_TRADE_COMMIT

실패 처리 원칙
- DB 실패 시 ROLLBACK으로 원자성 유지
- 종료 시 Flush로 미커밋 데이터 손실 최소화
- 로딩 게이트 실패 시 입장 차단 + 세션 정리

코드 근거
- GameServer/ClientPacketHandler.EnterGame.cpp
- GameServer/AutoCommitService.cpp
- GameServer/PersistenceService.cpp
- DBAgent/DBAgentPacketHandler.cpp
```

---

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

## **Section 3: 서버 엔진 설계** (6페이지)

**Page 22 - Actor + JobQueue 직렬화 모델**
- Mermaid 다이어그램 필요
```text
핵심 아이디어
- 액터 상태를 단일 실행 경로로 직렬화
- 세션은 PostRoom()으로 액터에 작업 위임

효과
- 데이터 레이스 감소
- 액터 로직 복잡도 제어
```

**Page 23 - Hybrid 스레드 모델**
- Mermaid 다이어그램 필요
```text
IOCP Dispatch 스레드 + 로직 워커 분리
- 네트워크 I/O와 게임 로직 경합 분리
- GlobalQueue가 JobQueue 실행 분배
```

**Page 24 - IOCP 네트워크 레이어**
- Mermaid 다이어그램 필요
```text
Windows IOCP + Overlapped 소켓
- Accept/Recv/Send 비동기 처리
- Session 기반 Dispatch
- 송신 백로그 임계치로 폭주 완화
```

**Page 25 - 패킷 파이프라인**
- Mermaid 다이어그램 필요
```text
PacketHeader: size/id/crc/seq
  ↓
핸들러 테이블 디스패치
  ↓
Protobuf 직렬화

부가 도구
- PacketGenerator(Python+Jinja2)
```

**Page 26 - 로비 데이터 로딩 게이트**
```text
입장 게이트
- Stat 로딩
- Items 로딩
- QuickSlot 로딩
모두 완료 시 월드 입장 허용

실패 시
- 입장 실패 응답 + 세션 정리
```

**Page 27 - 세션 생명주기 관리**
```text
sessionId ↔ playerId 바인딩
- MapChanging FSM 관리
- C_MAP_CHANGE_ACK 토큰 검증
- Disconnect 시 Dirty 마킹/Flush/인스턴스 정리
```

---

## **Section 4: 데이터 정합성 & 영속화** (5페이지)

**Page 28 - Redis 토큰 인증 + 세션 바인딩**
```text
LoginServer
- 토큰 생성 + Redis 저장(TTL)

GameServer
- 토큰 검증 후 playerId 확정
- 이름/세션 컨텍스트 복구
```

**Page 29 - Redis Write-Back + AutoCommit**
- Mermaid 다이어그램 필요
```text
런타임 변경
- Redis Hash 반영
- Dirty Set 기록

주기 저장
- AutoCommit이 Dirty 수집
- S2S_REQ_SAVE_*로 DBAgent 저장
- 성공 시 Dirty 제거
```

**Page 30 - DBAgent 트랜잭션 경로**
```text
ODBC + Connection Pool
- Prepared Statement
- BEGIN/COMMIT 트랜잭션
- Save/Load/Trade 커밋 전담

효과
- 게임 로직과 DB I/O 분리
```

**Page 31 - 부팅 데이터 동기화**
```text
S2S_REQ_LOAD_GAME_DATA
- Stat/Item/Skill Template 로드
- DataManager 메모리 적재

JSON 로드
- Maps/Monster/Spawn/Drop 테이블 반영
```

**Page 32 - GameItem UID 시드 부트스트랩**
```text
문제
- 재시작 시 UID 충돌 위험

해결
- DBAgent가 MAX(game_item_uid)+1 계산
- GameItemUidGen::Init(next_uid)
- 런타임 Alloc()로 단조 증가 발급
```

---

## **Section 5: 운영 관측 & 성능 검증** (3페이지)

**Page 33 - 메모리/송신 성능 가드레일**
```text
ObjectPool + MemoryPool
- 빈번 객체 할당/해제 비용 감소

SendBufferChunk 풀링 + Backpressure
- 송신 버퍼 재사용
- 백로그 임계치 기반 과부하 제어
```

**Page 34 - 관측 체계 (Prometheus + Grafana)**
```text
공용 계층
- /metrics Exporter
- MetricsSystem 초기화

도메인 계측
- GameServer: packet/lobby/s2s/CCU
- DBAgent: req/query/pool wait

대시보드
- Grafana JSON 자산으로 재현 가능
```

**Page 35 - DummyClient 부하 검증 보고**
```text
시나리오
- idle / move / combat / mix
- CCU 램프업 + heartbeat + RTT 수집

산출물
- CSV(평균/P95/전송량)
- 기능 변경 전/후 비교표
```

---

## **Section 6: 신뢰성 사례 & 마무리** (2페이지)

**Page 36 - 안정성 도구 + 트러블슈팅**
```text
운영 안정화 도구
- DeadLockProfiler
- ASSERT_CRASH
- StompAllocator

사례 1) 투사체 중복 히트
- 원인: hitTargets 갱신 타이밍
- 해결: 판정 직후 갱신

사례 2) 거래 중 아이템 중복
- 원인: 거래 중 인벤 조작 허용
- 해결: 거래 상태머신에서 인벤 조작 차단
```

**Page 37 - 역량 요약 & 참고 링크**
```text
핵심 역량
1) 서버 권위/정합성 중심 설계
2) IOCP + JobQueue 기반 동시성 제어
3) 운영 관측/부하 검증까지 포함한 개발

참고 링크
- GitHub: github.com/yourname
- 문서: 아키텍처/모니터링/부하테스트 보고서
- 선택: 동작 영상(보조 자료)
```
