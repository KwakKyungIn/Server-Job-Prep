# 📊 포트폴리오 PPT 구성 (30장)

> 범위: GameServer / LoginServer / DBAgent / ServerCore / Common(Protobuf) / Tools(PacketGenerator)
> 제외: ChatServer / DummyClient (사용 안 함)
> 기준 문서: `docs/Portfolio_FeatureList.md`, `docs/Portfolio_Techniques.md`
> 추가 확인 반영: CRC/Seq, 송신 백로그 캡, DeadLockProfiler, Memory/Pool, RedisCodec, GameItemUidGen

---

## **Section 1: 오프닝 & 개요** (3장)

**Slide 1 - 표지**
```
[대문]
MMORPG 서버 포트폴리오
- IOCP 기반 멀티 서버 구조
- 서버 권위 게임 서버 (C++/Redis/ODBC)

[본인 이름 / 연락처 / GitHub]
```

**Slide 2 - 프로젝트 개요**
```
📌 프로젝트 목표
"상용 게임 서버의 핵심 기술을 직접 구현"

• 로그인/게임/DB 분리 멀티 프로세스
• 서버 권위 이동 검증 + NavMesh 슬라이딩
• AOI 최적화 + 스냅샷 배칭
• 1:1 거래 원자 커밋

⭐ 핵심 차별화 (강조)
• 서버 권위 이동 + NavMesh 보정 (치트/위치 변조 방지)
• AOI 그리드 + 스냅샷 배칭 (패킷 폭증 억제)
• 1:1 거래 2-Phase Commit (아이템/골드 정합성)
• Redis Write-Back + AutoCommit (성능/정합성 트레이드오프)
• DeadLockProfiler + StompAllocator (안정성/디버깅)

📈 성능/정합성 지표 (선택)
• 평균 RTT / TPS / 동접 / 스냅샷 배치 크기 (수치 기입)

🛠 기술 스택
C++(MSVC) | Win32 IOCP | Protobuf | Redis | ODBC(RDBMS) | Recast/Detour NavMesh | JSON
```

**Slide 3 - 시스템 아키텍처**
```
[다이어그램]
Client ←→ LoginServer
          ↓
        Redis
          ↓
Client ←→ GameServer ←→ DBAgent ←→ RDBMS(ODBC)

• LoginServer: 인증/토큰
• GameServer: 게임 로직 + 룸/인스턴스 관리
• DBAgent: DB 전용 프로세스 + 트랜잭션 처리
• 분리 목적: 장애 격리 / 부하 분산 / 확장성
```

---

## **Section 2: 핵심 기술 상세** (15장)

### **2-1. 서버 권위 이동 검증** (3장)

**Slide 4 - 문제 정의**
```
❌ 클라이언트 신뢰 시
• 과속/텔레포트
• 벽 통과
• 좌표 변조

✅ 서버 권위 검증 필수
```

**Slide 5 - 검증 Flow**
```
[플로우]
C_MOVE 수신
  ↓
① 시퀀스 검증 (역순 드롭)
  ↓
② dt/속도 검증 (클램프)
  ↓
③ NavMesh ValidateMove (슬라이딩 보정)
  ↓
S_MOVE 브로드캐스트

[코드 스니펫 자리]
```

**Slide 6 - 데모 결과**
```
[Before/After 캡처 또는 로그]
Before: (100,100) → (500,500)
After : (100,100) → (150,150)

✅ 서버 권위 보정 확인
```

---

### **2-2. AOI (Area of Interest) 최적화** (3장)

**Slide 7 - 문제 정의**
```
❌ 전체 브로드캐스트
• 불필요한 패킷 폭증
• CPU/네트워크 부담

✅ 주변 가시 범위만 동기화
```

**Slide 8 - SpatialGrid 구조**
```
[다이어그램]
맵 → Grid 분할
  ↓
주변 Zone(3x3)만 조회
  ↓
거리/Connectivity 필터링

복잡도: O(n²) → O(k)
```

**Slide 9 - 스냅샷 배칭 전송**
```
[Before/After]
입장 시 대량 스폰 → 배치 분할 전송

S_SPAWN { snapshot_begin, entities[50] }
...
S_SPAWN { snapshot_end }

• 패킷 버스트 완화 + 초기 로딩 안정화
```

---

### **2-3. 투사체 시스템** (3장)

**Slide 10 - 구현 목표**
```
• 서버에서 투사체 위치 갱신
• 벽 충돌/타겟 충돌 판정
• 관통/다단히트 지원
```

**Slide 11 - 충돌 판정**
```
[다이어그램]
- 선분-원 충돌
- NavMesh 레이캐스트로 벽 충돌

[코드 스니펫 자리]
```

**Slide 12 - 관통/다단히트**
```
• isPenetrating 플래그로 분기
• hitTargets set으로 중복 타격 방지
```

---

### **2-4. 1:1 거래 원자 커밋** (3장)

**Slide 13 - 문제 정의**
```
❌ 단순 교환의 위험
• 아이템 복사
• 골드 증발
• 인벤 부족 시 롤백 실패

✅ 원자성 보장 필수
```

**Slide 14 - 2-Phase Commit**
```
Phase 1: 메모리 시뮬레이션
 - 인벤 공간/아이템 검증
 - 최종 스냅샷 생성

Phase 2: DB 트랜잭션
 BEGIN TRAN
  DELETE/UPSERT/UPDATE
 COMMIT

성공 시 메모리+Redis 갱신
실패 시 롤백

• JobQueue 직렬화 + 2PC로 원자성 보장
```

**Slide 15 - 검증 결과**
```
[테스트 케이스 요약]
✅ 정상 교환 성공
✅ 인벤 부족 → Phase1 차단
✅ DB 오류 → 롤백
✅ 동시 거래 → JobQueue 직렬화
```

---

### **2-5. Redis Write-Back + AutoCommit** (3장)

**Slide 16 - 설계 배경**
```
Write-Through: 안전하지만 DB 부하 큼
Write-Back: 성능 유리, 손실 리스크 관리 필요

✅ Redis 캐시 + 주기 DB 저장 전략
```

**Slide 17 - Dirty 트래킹 구조**
```
[다이어그램]
1) 변경 → Redis Hash 업데이트
2) DirtySet에 PlayerId 기록
3) AutoCommit 주기적 DB 저장
4) 성공 시 Dirty 제거
```

**Slide 18 - 데이터 보호 전략**
```
• 접속 종료 시 즉시 Flush
• 거래 완료 시 동기 저장
• 손실 리스크: N초 (설정값)
• 장애 대응: Dirty 복구 + 재시도 전략(요약)
```

---

## **Section 3: 시스템 설계** (6장)

**Slide 19 - 파티 시스템 구조**
```
[Actor 모델]
PartyActor(JobQueue)
 - 파티 상태 관리 직렬화
 - 파티 채팅/상태 스냅샷
 - 던전 인스턴스 연동
```

**Slide 20 - 인스턴스 던전 구조**
```
RoomKey = (channelId, mapId, instanceId)

인스턴스 생명주기:
생성 → 입장 → 플레이 → 종료 → 자동 정리
```

**Slide 21 - 몬스터 AI FSM**
```
Idle → Chase → Attack → Return
+ NavMesh 경로 탐색
+ LOS(Line of Sight) 최적화
+ Leash 범위 제한
```

**Slide 22 - 맵 변경 핸드셰이크**
```
C_MAP_CHANGE_REQ
  ↓
토큰 생성 + MapChanging 상태
  ↓
S_MAP_CHANGE_BEGIN
  ↓
C_MAP_CHANGE_ACK (토큰)
  ↓
검증 → 룸 이동 → S_MAP_CHANGE_END
```

**Slide 23 - 로그인 → 플레이 → 로그아웃 데이터 흐름**
```
LoginServer: 토큰 발급(Redis TTL)
GameServer: 토큰 검증 → 로딩 게이트
Play: Redis write-back + Dirty
Logout: Flush → DB 저장
```

**Slide 24 - 네트워크/동시성 핵심 설계**
```
• IOCP 네트워크 스레드
• JobQueue/GlobalQueue 로직 스레드
• RoomActor 단일 스레드 처리
• 송신 백로그 하드/소프트 캡
• 패킷 CRC + Seq 무결성 검증
• 메모리 풀/락 분리로 성능 안정화
```

---

## **Section 4: 안정성/디버깅/트러블슈팅** (4장)

**Slide 25 - 디버깅/안정성 도구**
```
• DeadLockProfiler (락 순서 그래프/사이클 탐지)
• ASSERT_CRASH 매크로로 조기 실패 감지
• StompAllocator로 메모리 오버런 검출
• 운영 중 재현 어려운 문제를 기록/재현 가능
```

**Slide 26 - 트러블슈팅 케이스 #1 (투사체 중복 히트)**
```
문제: 관통 OFF인데 다단히트 발생
원인: hitTargets 업데이트 타이밍
해결: 판정 직후 hitTargets 갱신
```

**Slide 27 - 트러블슈팅 케이스 #2 (거래 중 아이템 복사)**
```
문제: 거래 확정 후 아이템 중복
원인: 거래 중 인벤 조작 허용
해결: 거래 상태머신 중 인벤 조작 차단
```

**Slide 28 - 개선 예정 사항 (TODO)**
```
• 계정 생성/비밀번호 검증
• DB 로딩 실패 처리
• 스킬 범위(원형/부채꼴) 확장
• 플레이어 부활/몬스터 리젠/드랍 테이블
• 하트비트 타임아웃/모니터링
```

---

## **Section 5: 클로징** (2장)

**Slide 29 - 배운 점 & 역량**
```
1) 서버 권위 설계의 중요성
2) 동시성/직렬화 설계 경험
3) 성능/정합성 트레이드오프 이해
4) 네트워크/DB/캐시까지 전 구간 구현
```

**Slide 30 - Q&A / 연락처**
```
감사합니다!

Email: your@email.com
GitHub: github.com/yourname
Demo: (영상/링크)
```
