# Tech Stack & Techniques

## 한 페이지 요약
- IOCP 기반 멀티 서버 구조 — Login/Game/DBAgent 분리와 S2S 통신 골격을 구성.
- 전역 초기화(CoreGlobal) — 소켓/설정/매니저/Redis를 부팅 시점에 일괄 세팅.
- Hybrid 스레드 모델 — IOCP 네트워크 스레드 + JobQueue 로직 스레드 분리.
- JobQueue/GlobalQueue — 액터 단위 작업 큐로 경합을 줄이는 구조.
- RW SpinLock + 매크로 — 읽기/쓰기 락을 간단히 적용하는 공용 패턴.
- Protobuf 기반 프로토콜 — 클라/S2S 메시지를 .proto로 정의하고 코드 생성 활용.
- 맵/인스턴스/룸 — 채널/맵/인스턴스를 RoomKey로 관리.
- 맵 변경 핸드셰이크 — 토큰 검증 + 안전 귀환(던전/강퇴).
- 서버 권위 이동 검증 — 속도 클램프 + NavMesh 슬라이딩 보정.
- AOI/SpatialGrid — 근접 엔티티만 동기화하고 스냅샷 배칭.
- 전투/투사체 — 즉발 판정 + 투사체 충돌/벽 레이캐스트 + 쿨타임 서버 검증.
- 몬스터 FSM AI — Idle/Chase/Attack/Return + 경로 탐색/LOS.
- Redis Write-Back — 실시간 변경을 캐시하고 AutoCommit로 DB 저장.
- ODBC DB 트랜잭션 — Prepared Statement/Transaction으로 원자 커밋.
- 거래 2단계 커밋 — 메모리 시뮬레이션 + DB 트랜잭션(아이템/골드).
- 메모리 풀링 — ObjectPool/SendBufferChunk로 할당 비용 절감.
- 패킷 생성 도구 — Python+Jinja2로 핸들러 템플릿 생성.

---

## Architecture
### 멀티 서버 분리 + S2S 통신 골격 — 내가 한 일: Login/Game/DBAgent 서버 분리와 S2S 프로토콜 골격 구성
**근거**
- `GameServer/GameServer.cpp` — `int main()`에서 DB 세션 연결 로직
- `LoginServer/LoginServer.cpp` — `int main()`에서 서비스 분리(클라/게임서버/DBAgent)
- `DBAgent/DBAgent.cpp` — GameServer용/ LoginServer용 서비스 분리
- `Common/Protobuf/bin/Protocol_S2S.proto` — S2S 메시지 정의

1) Problem: 게임/로그인/DB 역할이 뒤섞이면 장애 격리와 스케일링이 어렵다.
2) Implementation: Login/Game/DBAgent를 별도 프로세스로 두고, S2S 패킷으로 요청/응답을 전달하도록 구성했다.
3) Pros/Cons: Pros) 장애 격리/역할 분리. Cons) 네트워크/운영 복잡도 증가.
4) Next: S2S 메시지 버전 관리/헬스체크 및 재시도 정책 강화.

### RoomActor + Lobby/Game 룸 분리 — 내가 한 일: 액터 기반 룸 구조로 작업 흐름을 단일 스레드화
**근거**
- `GameServer/RoomActor.h` — `RoomActor`, `RoomKind`
- `GameServer/GameRoom.h` — `RoomActor` 상속 및 `Push()` 큐 실행
- `GameServer/PlayerSession.h` — `Post()`/`PostRoom()` 경로
- `GameServer/LobbyRoom.cpp` — 로비에서 데이터 로드 후 월드 입장

1) Problem: 룸 내부 상태(플레이어/몬스터/거래 등)를 멀티스레드로 다루면 경합이 급증한다.
2) Implementation: RoomActor/JobQueue로 룸 로직을 단일 실행 경로로 직렬화하고, 세션은 `PostRoom()`으로 룸에 위임한다.
3) Pros/Cons: Pros) 동시성 버그 감소, 로직 단순화. Cons) 룸 단일 스레드 병목 가능.
4) Next: 핫룸 샤딩(Zone 단위 분리) 또는 오브젝트 단위 분산 큐 도입 검토.

### CoreGlobal 전역 초기화 — 내가 한 일: 서버 부팅 시 설정/소켓/매니저를 일괄 초기화
**근거**
- `ServerCore/CoreGlobal.cpp` — `CoreGlobal::CoreGlobal()`에서 Config 로드/SocketUtils/Redis/매니저 초기화
- `ServerCore/CoreGlobal.h` — `ServerConfig`, 전역 매니저 선언
- `ServerCore/SocketUtils.cpp` — WinSock 초기화

1) Problem: 서버 부팅 시 필수 구성 요소가 누락되면 런타임 장애가 발생한다.
2) Implementation: 전역 CoreGlobal 객체를 통해 부팅 시점에 설정/소켓/매니저/Redis를 준비한다.
3) Pros/Cons: Pros) 초기화 순서 일관화. Cons) 전역 의존성 증가.
4) Next: 서버별 설정/의존성 주입으로 전환(테스트/환경 분리 강화).

---

## Concurrency
### Hybrid 스레드 모델(IOCP + 로직 워커) — 내가 한 일: 네트워크 처리와 로직 처리를 분리
**근거**
- `GameServer/GameServer.cpp` — IOCP Dispatch 스레드 + 로직 스레드 분리
- `DBAgent/DBAgent.cpp` — 네트워크 스레드와 로직 스레드 분리
- `ServerCore/ThreadManager.cpp` — `DoGlobalQueueWork()`

1) Problem: 네트워크 I/O와 로직 처리를 같은 스레드에서 처리하면 지연이 커진다.
2) Implementation: IOCP 처리 스레드와 JobQueue 로직 스레드를 분리해 병렬 처리한다.
3) Pros/Cons: Pros) I/O 지연 감소. Cons) 스레드 수 관리/디버깅 복잡.
4) Next: 부하 기반 스레드 수 동적 조절, 작업량 기반 스케줄링 개선.

### JobQueue + GlobalQueue 스케줄링 — 내가 한 일: 작업 큐를 글로벌 큐로 분배하는 실행 모델 구현
**근거**
- `ServerCore/JobQueue.h/.cpp` — `JobQueue::Push/Execute`
- `ServerCore/GlobalQueue.h/.cpp` — `GlobalQueue::Push/Pop`
- `ServerCore/ThreadManager.cpp` — `DoGlobalQueueWork`가 GlobalQueue를 소비

1) Problem: 모든 로직을 하나의 큐에 넣으면 병목이 심해진다.
2) Implementation: JobQueue를 등록하고 GlobalQueue가 작업 큐를 분배해 워커가 처리한다.
3) Pros/Cons: Pros) 큐 분산으로 병목 완화. Cons) 작업 순서 추적이 어려움.
4) Next: 큐 우선순위/백프레셔 추가.

### RW SpinLock + 매크로 적용 — 내가 한 일: 간단한 동시성 제어 유틸 제공
**근거**
- `ServerCore/Lock.h` — `Lock`, `ReadLockGuard`, `WriteLockGuard`
- `ServerCore/CoreMacro.h` — `READ_LOCK/WRITE_LOCK` 매크로
- `GameServer/GameSessionManager.cpp` — `READ_LOCK/WRITE_LOCK` 사용

1) Problem: 빈번한 읽기/쓰기에 대한 간단한 락이 필요하다.
2) Implementation: RW 스핀락과 RAII Guard, 매크로로 적용 범위를 축소했다.
3) Pros/Cons: Pros) 구현 단순, 읽기 병렬성. Cons) 스핀락 특성상 과점유 위험.
4) Next: 경합이 큰 영역은 뮤텍스/락 프리 구조로 교체 검토.

---

## Network
### Windows IOCP + Overlapped 소켓 — 내가 한 일: 고성능 네트워크 레이어 구현
**근거**
- `ServerCore/IocpCore.cpp` — `CreateIoCompletionPort`, `GetQueuedCompletionStatus`
- `ServerCore/Session.h/.cpp` — `IocpObject::Dispatch`, `RegisterRecv/Send`
- `ServerCore/SocketUtils.cpp` — `ConnectEx/AcceptEx` 바인딩

1) Problem: 다수 연결을 처리하기 위해 고성능 I/O 모델이 필요하다.
2) Implementation: IOCP 코어와 Overlapped 소켓을 사용해 Accept/Recv/Send를 비동기로 처리한다.
3) Pros/Cons: Pros) 높은 동시 접속 처리. Cons) Win32 종속/디버깅 난이도 상승.
4) Next: 플랫폼 추상화 계층 분리(테스트/이식성 개선).

### 패킷 파이프라인(헤더+핸들러 매핑) — 내가 한 일: 고정 헤더와 핸들러 테이블로 패킷 라우팅
**근거**
- `ServerCore/Session.h` — `PacketHeader { size, id, crc, seq }`
- `GameServer/ClientPacketHandler.h` — `GPacketHandler` 매핑
- `LoginServer/ClientPacketHandler.h` — `GPacketHandler` 매핑

1) Problem: 수신 패킷을 빠르고 일관되게 디스패치해야 한다.
2) Implementation: 고정 헤더로 패킷을 식별하고 핸들러 테이블로 분기 처리한다.
3) Pros/Cons: Pros) O(1) 디스패치. Cons) 패킷 ID 관리가 수동/경직.
4) Next: 자동 ID 충돌 검증 및 버전 관리 추가.

### Protobuf 기반 메시지 직렬화 — 내가 한 일: 클라이언트/서버 및 S2S 메시지를 .proto로 정의
**근거**
- `Common/Protobuf/bin/Protocol.proto` — 게임/클라 메시지 정의
- `Common/Protobuf/bin/Protocol_S2S.proto` — S2S 메시지 정의
- `GameServer/Protocol.pb.cc` — generated protobuf 코드 사용

1) Problem: 메시지 스키마를 명확히 정의하고 버전 변경을 관리해야 한다.
2) Implementation: protobuf로 메시지를 정의하고 생성된 코드를 서버/클라에 포함했다.
3) Pros/Cons: Pros) 스키마 명확/호환성 확보. Cons) 코드 생성 의존성.
4) Next: 스키마 버전/마이그레이션 정책 정식화.

---

## Gameplay Server Systems
### 룸/채널/인스턴스 관리 — 내가 한 일: RoomKey로 채널/맵/인스턴스를 식별해 룸을 관리
**근거**
- `GameServer/RoomManager.h/.cpp` — `RoomKey`, `GetOrCreateRoom`
- `GameServer/InstanceManagerCore.cpp` — `CreateOrGetForParty`, `CloseForParty`
- `GameServer/GameRoom.MapChange.cpp` — 룸 이동 흐름

1) Problem: 다중 채널/맵/던전 인스턴스를 안정적으로 관리해야 한다.
2) Implementation: `RoomKey(channelId, mapId, instanceId)`로 룸을 분리하고 InstanceActor로 인스턴스를 관리했다.
3) Pros/Cons: Pros) 명확한 룸 식별. Cons) 룸 수 증가 시 관리 비용 상승.
4) Next: 룸 메타 모니터링/강제 정리 정책 개선.

### 맵 변경 핸드셰이크 + 안전 귀환 — 내가 한 일: 토큰 기반 전이와 강제 귀환 흐름 통합
**근거**
- `GameServer/ClientPacketHandler.MapChangeUtil.cpp` — `MakeMapChangeToken`, `SendMapChangeBegin`, `ForceReturnToWorld`
- `GameServer/ClientPacketHandler.MapChange.cpp` — `Handle_C_MAP_CHANGE_REQ/ACK`
- `GameServer/ClientPacketHandler.Dungeon.cpp` — 던전 입장/퇴장 맵 전이
- `GameServer/PartyActor.cpp` — 강퇴/해산/인스턴스 종료 시 강제 귀환

1) Problem: 맵 이동/던전 전이는 스푸핑·중복 요청에 취약하고 실패 시 유저가 고립될 수 있다.
2) Implementation: 토큰 발급 → MapChanging 상태 전이 → ACK 검증 후 룸 이동, 강제 귀환은 안전 좌표 계산 후 동일 핸드셰이크로 처리한다.
3) Pros/Cons: Pros) 전이 검증 일원화, 안전 복귀 보장. Cons) 전이 중 지연/끊김 처리 복잡.
4) Next: 전이 타임아웃/롤백 및 실패 응답 표준화.

### 서버 권위 이동 검증 + NavMesh 슬라이딩 — 내가 한 일: 속도/시퀀스/네비 검증을 통합
**근거**
- `GameServer/GameRoom.Move.cpp` — `HandleMove` 검증 흐름
- `GameServer/MoveValidationUtils.h` — `CheckSpeed2D`, `ComputeDtSec`
- `GameServer/NavSystem.cpp` — `ValidateMove` 슬라이딩 보정

1) Problem: 클라이언트 조작 이동(스피드핵/벽뚫기)을 서버가 방어해야 한다.
2) Implementation: 시퀀스/시간 검증 → 속도 클램프 → NavMesh 검증/슬라이딩 보정 순으로 처리했다.
3) Pros/Cons: Pros) 서버 권위 강화. Cons) 네비 데이터 없으면 제한적.
4) Next: 클라이언트 예측 보정/스냅샷 리컨실리에이션 추가.

### AOI + SpatialGrid 기반 가시성 동기화 — 내가 한 일: 주변 객체만 전송하고 스냅샷 배칭
**근거**
- `GameServer/SpatialGrid.h/.cpp` — `GetNearbyZones`
- `GameServer/GameRoom.AOI.v2.cpp` — `UpdateAOI`, `SendSpawnBatchedToMe`
- `Common/Protobuf/bin/Protocol.proto` — `S_SPAWN`, `S_DESPAWN`

1) Problem: 모든 객체를 전송하면 네트워크 비용이 과도하다.
2) Implementation: SpatialGrid로 후보를 줄이고 거리/Connectivity 필터 후 스폰/디스폰을 배칭 전송한다.
3) Pros/Cons: Pros) 네트워크/CPU 절약. Cons) 복잡한 상태 동기화 필요.
4) Next: 시야 갱신 주기/임계값 동적 조절.

### 전투 시스템(즉발/투사체) — 내가 한 일: 스킬 타입에 따라 판정과 투사체 로직을 분리
**근거**
- `GameServer/GameRoom.Combat.cpp` — `HandleSkill` 분기
- `GameServer/BattleSystem.cpp` — `ResolveSkill`
- `GameServer/GameRoom.Projectile.cpp` — 투사체 이동/충돌

1) Problem: 다양한 스킬 판정을 서버에서 일관되게 처리해야 한다.
2) Implementation: 즉발형은 BattleSystem에서 판정하고, 투사체는 별도 오브젝트로 갱신한다. `CanUseSkill`로 서버 쿨타임을 검증하고 `StartSkillCooldown`으로 소비한다.
3) Pros/Cons: Pros) 스킬 타입 확장 용이. Cons) 일부 스킬 타입 미구현.
4) Next: 원형/부채꼴 등 범위 스킬 판정 추가.

### 몬스터 FSM AI + 경로 탐색 — 내가 한 일: FSM과 NavMesh 경로/LOS를 결합
**근거**
- `GameServer/Monster.h/.cpp` — `AiState`, `UpdateChase/Attack`
- `GameServer/NavSystem.cpp` — `FindPathWaypoints`, `HasLineOfSight`
- `GameServer/GameRoom.Monster.cpp` — `Update` 루프

1) Problem: 서버가 몬스터 행동을 주도해야 치팅을 방지할 수 있다.
2) Implementation: FSM(Idle/Chase/Attack/Return) + LOS/경로 재계산으로 추적을 구현했다.
3) Pros/Cons: Pros) 예측 가능한 AI 흐름. Cons) 복잡한 지형에서 CPU 부담.
4) Next: AI 업데이트 주기 최적화/스킬 다양화.

### 거래 2단계 커밋(Phase1/Phase2) — 내가 한 일: 아이템/골드를 시뮬레이션 후 원자 커밋
**근거**
- `GameServer/GameRoom.Trade.cpp` — `BuildTradeCommitPlan_ActorOnly`, `StartTradeCommitPhase2_ActorOnly`
- `GameServer/GameRoom.Trade.cpp` — `HandleTradeGoldSetById`, `OnTradeCommitResult_ActorOnly`
- `DBAgent/DBAgentPacketHandler.cpp` — `Handle_S2S_REQ_TRADE_COMMIT`
- `Common/Protobuf/bin/Protocol_S2S.proto` — `S2S_REQ/RES_TRADE_COMMIT`

1) Problem: 거래 중 아이템/골드 유실·복제가 발생하면 치명적인 데이터 오류가 생긴다.
2) Implementation: Phase1에서 인벤/골드를 메모리 시뮬레이션해 검증하고, Phase2에서 DBAgent 트랜잭션으로 커밋 후 결과를 메모리/Redis에 반영한다.
3) Pros/Cons: Pros) 원자성 확보, 롤백 가능. Cons) DB 지연에 민감, 실패 처리 복잡.
4) Next: 커밋 타임아웃/재시도 정책 및 모니터링 추가.

---

## Persistence & Data
### Redis 토큰 인증 + 캐시 — 내가 한 일: 로그인 토큰 저장과 런타임 캐시 사용
**근거**
- `LoginServer/S2SPacketHandler.cpp` — 토큰 생성/Redis `Set`
- `GameServer/ClientPacketHandler.EnterGame.cpp` — Redis 토큰 조회
- `ServerCore/RedisManager.h` — `Set/Get` API

1) Problem: 로그인과 게임 접속 사이의 인증 정보를 안전하게 전달해야 한다.
2) Implementation: LoginServer가 토큰을 Redis에 저장하고 GameServer가 검증한다.
3) Pros/Cons: Pros) 간단한 중앙 인증. Cons) Redis 장애 시 로그인 실패.
4) Next: 토큰 서명(JWT 등)과 Redis 장애 대응(캐시/재시도) 강화.

### Redis Write-Back + AutoCommit — 내가 한 일: 실시간 변경을 Redis에 누적 후 주기 저장
**근거**
- `GameServer/PersistenceService.h` — `UpdatePlayerCore/Inventory/QuickSlot`
- `GameServer/AutoCommitService.cpp` — Dirty Set 수집/DB 저장 요청
- `GameServer/RedisKeys.h` — `dirty:*` 키 정의

1) Problem: 모든 변경을 즉시 DB에 쓰면 성능이 급락한다.
2) Implementation: Redis에 변경 사항을 기록하고 AutoCommit이 주기적으로 DB 저장 요청을 보낸다.
3) Pros/Cons: Pros) DB 부하 감소. Cons) Redis/프로세스 장애 시 손실 가능.
4) Next: 스냅샷 주기/즉시 저장 정책 세분화.

### ODBC 기반 DB 접근 + 트랜잭션 — 내가 한 일: Prepared Statement와 트랜잭션으로 데이터 정합성 확보
**근거**
- `DBAgent/DBConnection.h` — ODBC API(`SQLHDBC`, `Prepare/Execute`)
- `DBAgent/DBAgentPacketHandler.cpp` — `BEGIN TRAN`/`COMMIT` 사용
- `DBAgent/DBConnectionPool.cpp` — 커넥션 풀

1) Problem: 게임 데이터는 원자적 업데이트가 필요하다.
2) Implementation: ODBC로 Prepared Statement를 사용하고 트랜잭션으로 일괄 커밋한다.
3) Pros/Cons: Pros) 정합성 보장. Cons) DB 병목 시 지연 증가.
4) Next: 쿼리 배치 최적화/비동기 재시도 추가.

### JSON 기반 설정/맵 데이터 — 내가 한 일: ServerConfig와 Maps.json을 로드
**근거**
- `ServerCore/CoreGlobal.cpp` — `ServerConfig.json` 로드(`nlohmann::json`)
- `GameServer/DataManager.cpp` — `LoadMapConfigsFromJson`
- `GameServer/Maps.json` — `mapId/zoneSize/interestRadius` 등 설정 키

1) Problem: 서버 설정과 맵 파라미터를 코드에서 분리해야 한다.
2) Implementation: JSON 파일에서 설정 값을 읽어 전역/맵 설정에 반영했다.
3) Pros/Cons: Pros) 운영 편의성. Cons) 파일 누락 시 런타임 위험.
4) Next: 환경별 설정 분리 및 유효성 검사 강화.

### 게임 데이터 S2S 로딩(스탯/아이템/스킬) — 내가 한 일: DBAgent에서 게임 데이터 스냅샷 로딩
**근거**
- `Common/Protobuf/bin/Protocol_S2S.proto` — `S2S_REQ_LOAD_GAME_DATA`
- `DBAgent/DBAgentPacketHandler.cpp` — `Handle_S2S_REQ_LOAD_GAME_DATA`
- `GameServer/S2SPacketHandler.cpp` — `Handle_S2S_RES_LOAD_GAME_DATA`

1) Problem: 게임 데이터(스탯/아이템/스킬)를 서버 시작 시 동기화해야 한다.
2) Implementation: GameServer가 S2S로 요청하고 DBAgent가 DB에서 로드해 전달한다.
3) Pros/Cons: Pros) 데이터 동기화 일원화. Cons) 로딩 실패 시 대체 경로 부족.
4) Next: 캐시/핫 리로드 지원.

---

## Memory & Performance
### ObjectPool + MemoryPool — 내가 한 일: 객체/메모리 풀링으로 할당 비용 절감
**근거**
- `ServerCore/ObjectPool.h` — `ObjectPool::MakeShared`
- `ServerCore/MemoryPool.h` — lock-free SLIST 풀
- `GameServer/GameRoom.Combat.cpp` — `ObjectPool<Projectile>::MakeShared()` 사용

1) Problem: 투사체/잡 등 빈번한 생성/삭제 비용이 크다.
2) Implementation: MemoryPool 기반 ObjectPool을 도입해 재사용하도록 했다.
3) Pros/Cons: Pros) 할당/해제 비용 감소. Cons) 풀 사이즈 추적 필요.
4) Next: 풀 사용량 모니터링/튜닝.

### SendBufferChunk 풀링 + 송신 백프레셔 — 내가 한 일: 송신 버퍼 재사용 및 임계치 제한
**근거**
- `ServerCore/SendBuffer.h` — `SendBufferChunk`, `SendBufferManager`
- `ServerCore/Session.h` — `MAX_BACKLOG_BYTES/COUNT`
- `ServerCore/SendBuffer.cpp` — 글로벌 풀 관리

1) Problem: 빈번한 송신 버퍼 생성이 성능을 저하시킨다.
2) Implementation: 6KB Chunk 풀을 재사용하고 백로그 임계치로 과도한 송신을 제한한다.
3) Pros/Cons: Pros) 메모리 재사용/폭주 방지. Cons) 큰 패킷 처리 시 조각화.
4) Next: 동적 Chunk 크기/임계치 튜닝.

### AOI 스냅샷 배칭 전송 — 내가 한 일: 대량 스폰/디스폰 패킷을 배치로 분할
**근거**
- `GameServer/GameRoom.AOI.v2.cpp` — `SendSpawnBatchedToMe`, `SendDespawnBatchedToMe`
- `Common/Protobuf/bin/Protocol.proto` — `S_SPAWN.snapshot_*` 필드

1) Problem: 맵 진입 시 대량 스폰 패킷으로 소켓 버퍼가 터질 수 있다.
2) Implementation: 배치 크기 단위로 스폰 패킷을 분할하고 begin/end 플래그를 전송한다.
3) Pros/Cons: Pros) 패킷 폭주 완화. Cons) 스냅샷 처리 로직 복잡.
4) Next: 배치 크기 동적 조절.

---

## Tooling & Build
### 패킷 코드 생성 도구 — 내가 한 일: Proto 기반 핸들러 생성 스크립트 유지
**근거**
- `Tools/PacketGenerator/PacketGenerator.py` — Jinja2 템플릿 기반 생성
- `Tools/PacketGenerator/Templates/PacketHandler.h` — 출력 템플릿
- `Tools/PacketGenerator/ProtoParser.py` — .proto 파싱

1) Problem: 패킷 핸들러 수작업은 실수가 많다.
2) Implementation: Python+Jinja2로 핸들러/ID 매핑 코드를 자동 생성한다.
3) Pros/Cons: Pros) 반복 작업 감소. Cons) 템플릿/스키마 변경 시 관리 필요.
4) Next: CI에서 자동 생성/검증 단계 추가.

### Visual Studio 빌드 구성 — 내가 한 일: 솔루션/프로젝트 기반으로 빌드 구조 유지
**근거**
- `IocpChatServer.sln` — 솔루션 엔트리
- `GameServer/GameServer.vcxproj` — 프로젝트 설정
- `DBAgent/DBAgent.vcxproj` — 프로젝트 설정

1) Problem: 여러 서버 모듈을 동시에 관리할 빌드 구성이 필요하다.
2) Implementation: VS 솔루션과 프로젝트 파일로 모듈별 빌드를 분리했다.
3) Pros/Cons: Pros) 윈도우 개발 편의. Cons) 플랫폼 종속.
4) Next: CMake 등 크로스 플랫폼 빌드 스크립트 추가 검토.

### 외부 라이브러리 사용 사실 — 내가 한 일: 필수 라이브러리를 도입해 기능을 구현
**근거**
- `GameServer/packages.config` — `nlohmann.json`
- `ServerCore/RedisManager.h` — `cpp_redis`
- `GameServer/NavSystem.cpp` — `DetourNavMesh/DetourNavMeshQuery` 포함

1) Problem: JSON 파싱, Redis, NavMesh 등은 직접 구현하기 비효율적이다.
2) Implementation: nlohmann/json, cpp_redis, Detour/Recast를 사용(내부 구현 분석은 생략).
3) Pros/Cons: Pros) 개발 시간 단축. Cons) 외부 의존성 증가.
4) Next: 버전/보안 업데이트 체계화.

### 콘솔 명령 기반 수동 테스트 — 내가 한 일: 서버 내 테스트 커맨드로 기능 검증
**근거**
- `GameServer/GameServer.cpp` — `ConsoleThread` 테스트 명령

1) Problem: 클라이언트 없이도 기능을 빠르게 검증할 경로가 필요했다.
2) Implementation: 서버 콘솔 명령으로 파티/상태 등 핵심 흐름을 수동 검증한다.
3) Pros/Cons: Pros) 빠른 수동 검증. Cons) 자동화 테스트 부재.
4) Next: 자동화 테스트/시뮬레이터 추가.
