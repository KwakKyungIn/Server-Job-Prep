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
- 전투/범위/투사체 — 즉발/원형/부채꼴 판정 + 투사체 충돌/벽 레이캐스트 + 쿨타임 서버 검증.
- 플레이어 리스폰 — 월드 스폰 부활 + 던전 강제 귀환 부활 처리.
- 몬스터 스폰/리젠/드랍 — JSON 스폰 포인트 + 확률 드랍 그룹 롤링.
- 몬스터 FSM AI — Idle/Chase/Attack/Return + 경로 탐색/LOS.
- 파티 초대 검증 — 이름 기반 대상 해석 + 동명이인 차단 + 60초 만료 제어.
- Redis Write-Back — 실시간 변경을 캐시하고 AutoCommit로 DB 저장.
- ODBC DB 트랜잭션 — Prepared Statement/Transaction으로 원자 커밋.
- 게임 아이템 UID 시드 부트스트랩 — 서버 시작 시 DB 최대 UID+1 동기화.
- 거래 2단계 커밋 — 메모리 시뮬레이션 + DB 트랜잭션(아이템/골드).
- Prometheus 메트릭 — `/metrics` exporter + Game/DB 커스텀 지표 수집.
- Grafana 대시보드 — Prometheus 지표 시각화 자산 구성.
- DummyClient 부하 테스트 — 시나리오 기반 CCU 램프업 + CSV(P95) 리포트.
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

### 서비스 하트비트 + 자동 재연결 — 내가 한 일: S2S 연결의 timeout/ping/reconnect 경로를 공통 서비스 계층에 구축
**근거**
- `ServerCore/Service.cpp` — `CheckHeartbeat`, `ClientService::CheckHeartbeat`
- `GameServer/DBSession.cpp`, `GameServer/LoginSession.cpp` — `Ping`
- `DBAgent/DBAgentPacketHandler.cpp` — `Handle_S2S_REQ_HEART_BEAT`
- `LoginServer/GameServerSession.cpp` — `PKT_S2S_REQ_HEART_BEAT` 직접 응답

1) Problem: DB/Login 연결이 끊기면 인증/영속화 흐름이 연쇄적으로 중단된다.
2) Implementation: 서비스 공통 계층에서 30초 timeout 검사 + Ping 송신을 수행하고, ClientService는 세션 부족 시 자동 재연결을 시도한다.
3) Pros/Cons: Pros) 서버간 연결 복원 자동화. Cons) 게임 클라이언트 heartbeat timeout은 현재 메인 루프에서 비활성.
4) Next: 클라이언트 세션 heartbeat 강제 종료/RTT 기반 정책까지 통합.

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

### 파티 초대 이름 매칭 + 만료 제어 — 내가 한 일: 온라인 이름 인덱스와 TTL 초대장을 결합해 초대 정합성 강화
**근거**
- `GameServer/ClientPacketHandler.Party.cpp` — `Handle_C_PARTY_INVITE_REQ`
- `GameServer/GameSessionManager.cpp` — `TryGetPlayerIdByName`, `RebuildNameIndex_Locked`
- `GameServer/PartyManagerCore.cpp` — `Invite`, `AcceptInvite` (`expireTick`)

1) Problem: 이름 기반 초대는 동명이인/오프라인/지연 응답에서 잘못된 대상 매칭이 발생하기 쉽다.
2) Implementation: SessionManager 이름 인덱스로 대상 ID를 해석하고, 동명이인은 ambiguous로 거절한다. PartyManagerCore가 초대장을 target 기준으로 보관하고 60초 만료(`expireTick`)를 강제한다.
3) Pros/Cons: Pros) 초대 대상 오매칭과 지연 수락 이슈를 줄임. Cons) 닉네임 중복 정책에 따라 이름 초대 UX 제약이 생김.
4) Next: 닉네임+태그(예: `name#1234`) 기반 식별자 도입 및 초대 만료 사유 코드 분리.

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

### 로비 DB 로딩 게이트 + 실패 롤백 — 내가 한 일: 비동기 3종 로딩 완료 전까지 월드 입장을 차단하고 실패 시 정리
**근거**
- `GameServer/ClientPacketHandler.EnterGame.cpp` — `SetPendingEnter_ActorOnly`, DB 로딩 요청
- `GameServer/LobbyRoom.cpp` — `OnStatLoaded`, `OnItemsLoaded`, `OnQuickSlotsLoaded`, `TryEnterWorldIfReady`, `OnDbLoadFailed`
- `GameServer/S2SPacketHandler.cpp` — 로딩 응답을 LobbyRoom으로 전달

1) Problem: 스탯/인벤/퀵슬롯이 반쯤 로딩된 상태로 월드 입장하면 데이터 정합성이 깨진다.
2) Implementation: 로비에서 3개 로딩 플래그를 게이트로 관리하고, 전부 완료될 때만 입장시킨다. `OnItemsLoaded` 단계에서 중복 장착(동일 부위 다중 장착)을 정규화해 Redis/Dirty까지 즉시 동기화한다. 실패 시 `S_ENTER_GAME(false)` 응답 후 세션 정리/종료한다.
3) Pros/Cons: Pros) 불완전 로딩 및 비정상 장비 상태를 입장 전에 차단. Cons) 로딩 단계에서 정규화 비용이 추가되고 부분복구 경로는 아직 단순.
4) Next: 로딩 실패 원인 코드화 및 재시도 백오프 추가.

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

### 전투 시스템(즉발/원형/부채꼴/투사체) — 내가 한 일: 스킬 타입별 판정과 투사체 로직을 분리
**근거**
- `GameServer/GameRoom.Combat.cpp` — `HandleSkill` 분기
- `GameServer/BattleSystem.cpp` — `ResolveSkill`
- `GameServer/GameRoom.Projectile.cpp` — 투사체 이동/충돌

1) Problem: 다양한 스킬 판정을 서버에서 일관되게 처리해야 한다.
2) Implementation: `ResolveSkill`에서 `SKILL_AUTO`, `SKILL_AREA_CIRCLE`, `SKILL_AREA_CONE`를 판정하고, `SKILL_PROJECTILE`은 별도 오브젝트 업데이트 루프에서 처리한다. `CanUseSkill`/`StartSkillCooldown`으로 쿨타임을 서버 권위로 소비한다.
3) Pros/Cons: Pros) 스킬 타입별 권위 판정 확립. Cons) 판정 파라미터(range/radius/angle) 튜닝 난이도.
4) Next: 스킬별 히트 피드백/로그 계측을 추가해 밸런싱 루프 단축.

### 플레이어 부활(월드 스폰 + 던전 강제 귀환) — 내가 한 일: `C_RESPAWN_REQ`를 월드/던전 전이에 맞게 처리
**근거**
- `GameServer/ClientPacketHandler.GamePlay.cpp` — `Handle_C_RESPAWN_REQ`
- `GameServer/GameRoom.MapChange.cpp` — `HandleRespawn`, `TransferMapChangeById`
- `GameServer/Player.cpp` — `OnDead`
- `Common/Protobuf/bin/Protocol.proto` — `C_RESPAWN_REQ`

1) Problem: 사망 후 복귀 경로가 없으면 플레이 흐름이 끊기고 던전 내부 고립이 발생한다.
2) Implementation: 월드맵은 스폰 좌표로 즉시 부활, 던전은 `pendingRespawn` 마킹 후 `ForceReturnToWorld` 핸드셰이크로 안전 복귀 후 부활하도록 분기했다.
3) Pros/Cons: Pros) 사망→복귀 흐름 일관화. Cons) 전용 respawn 응답 패킷 없이 `S_MOVE`/`S_CHANGE_HP` 조합에 의존.
4) Next: 부활 무적시간/쿨다운 정책과 전용 응답 패킷 정식화.

### 몬스터 FSM AI + 경로 탐색 — 내가 한 일: FSM과 NavMesh 경로/LOS를 결합
**근거**
- `GameServer/Monster.h/.cpp` — `AiState`, `UpdateChase/Attack`
- `GameServer/NavSystem.cpp` — `FindPathWaypoints`, `HasLineOfSight`
- `GameServer/GameRoom.Monster.cpp` — `Update` 루프

1) Problem: 서버가 몬스터 행동을 주도해야 치팅을 방지할 수 있다.
2) Implementation: FSM(Idle/Chase/Attack/Return) + LOS/경로 재계산으로 추적을 구현했다.
3) Pros/Cons: Pros) 예측 가능한 AI 흐름. Cons) 복잡한 지형에서 CPU 부담.
4) Next: AI 업데이트 주기 최적화/스킬 다양화.

### 몬스터 스폰/리젠 + 드랍 테이블 — 내가 한 일: JSON 테이블 기반 스폰 포인트/확률 드랍 시스템 적용
**근거**
- `GameServer/GameServer.cpp` — `MonsterTemplates/SpawnTables/DropTables` 로드
- `GameServer/DataManager.cpp` — `LoadSpawnTablesFromJson`, `LoadDropTablesFromJson`
- `GameServer/GameRoom.LifeTime.cpp` — `InitSpawnPoints_ActorOnly`, `UpdateSpawns_ActorOnly`
- `GameServer/GameRoom.Items.cpp` — `HandleMonsterDead`, `RollDropGroup`

1) Problem: 고정 스폰/고정 드랍은 밸런싱과 운영 확장성이 낮다.
2) Implementation: `spawnId/maxAlive/respawnSec` 기반 런타임 스폰 포인트를 관리하고, `DropGroup(weight/roll/noDropWeight)`로 처치 보상을 롤링한다.
3) Pros/Cons: Pros) 데이터 중심 밸런싱 가능. Cons) 인벤 가득 찬 보상은 현재 바닥 드랍으로 이관되지 않음.
4) Next: 월드 드랍 오브젝트/획득 실패 알림 추가.

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
2) Implementation: LoginServer가 토큰과 `token:name:{token}`를 Redis에 저장하고, GameServer가 토큰 검증과 이름 복구에 사용한다.
3) Pros/Cons: Pros) 간단한 중앙 인증/이름 컨텍스트 전달. Cons) Redis 장애 시 로그인 실패, `S_LOGIN.serverList`는 현재 정적 하드코딩이라 실시간 혼잡도 연동이 없다.
4) Next: 토큰 서명(JWT 등), Redis 장애 대응(캐시/재시도), 로그인 서버 리스트 동적 디스커버리/혼잡도 연동.

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

### JSON 기반 설정/맵 데이터 — 내가 한 일: 서버별 설정과 맵 파라미터를 실행 파일 기준으로 로드
**근거**
- `ServerCore/CoreGlobal.cpp` — `ServerConfig.{Exe}.json` 우선 + `ServerConfig.json` 폴백 로드
- `GameServer/DataManager.cpp` — `LoadMapConfigsFromJson`
- `GameServer/Maps.json` — `mapId/zoneSize/interestRadius` 등 설정 키

1) Problem: 서버 설정과 맵 파라미터를 코드에서 분리해야 한다.
2) Implementation: 실행 파일 이름 기반으로 설정 파일을 선택해 로드하고(`GameServer`/`DBAgent` 등), 맵 파라미터는 `Maps.json`으로 분리했다.
3) Pros/Cons: Pros) 운영 편의성. Cons) 파일 누락 시 런타임 위험.
4) Next: 환경별 설정 분리 및 유효성 검사 강화.

### JSON 기반 몬스터/스폰/드랍 데이터 — 내가 한 일: 기획 테이블을 서버 런타임 데이터로 변환
**근거**
- `GameServer/DataManager.h` — `MonsterTemplate`, `SpawnEntry`, `DropGroup`
- `GameServer/DataManager.cpp` — `LoadMonsterTemplatesFromJson`, `LoadSpawnTablesFromJson`, `LoadDropTablesFromJson`
- `Binary/Debug/MonsterTemplates.json`, `Binary/Debug/SpawnTables.json`, `Binary/Debug/DropTables.json`

1) Problem: 몬스터/드랍 밸런스 조정이 코드 수정을 요구하면 운영 속도가 느리다.
2) Implementation: JSON 테이블을 로드해 메모리 캐시로 변환하고, 룸/전투 시스템이 이를 조회하도록 연결했다.
3) Pros/Cons: Pros) 기획 데이터 반영 속도 향상. Cons) 스키마 유효성 검증이 느슨하면 런타임 누락 가능.
4) Next: JSON 스키마 검증과 로드 실패 리포트 고도화.

### 게임 데이터 S2S 로딩(스탯/아이템/스킬) — 내가 한 일: DBAgent에서 게임 데이터 스냅샷 로딩
**근거**
- `Common/Protobuf/bin/Protocol_S2S.proto` — `S2S_REQ_LOAD_GAME_DATA`
- `DBAgent/DBAgentPacketHandler.cpp` — `Handle_S2S_REQ_LOAD_GAME_DATA`
- `GameServer/S2SPacketHandler.cpp` — `Handle_S2S_RES_LOAD_GAME_DATA`

1) Problem: 게임 데이터(스탯/아이템/스킬)를 서버 시작 시 동기화해야 한다.
2) Implementation: GameServer가 S2S로 요청하고 DBAgent가 DB에서 로드해 전달한다.
3) Pros/Cons: Pros) 데이터 동기화 일원화. Cons) 로딩 실패 시 대체 경로 부족.
4) Next: 캐시/핫 리로드 지원.

### 게임 아이템 UID 시드 부트스트랩 — 내가 한 일: 재시작 시 UID 충돌을 막기 위해 DB 기준 시드를 동기화
**근거**
- `GameServer/DBSession.cpp` — `S2S_REQ_GAME_ITEM_UID_SEED` 요청
- `DBAgent/DBAgentPacketHandler.cpp` — `Handle_S2S_REQ_GAME_ITEM_UID_SEED` (MAX UID 조회)
- `GameServer/S2SPacketHandler.cpp` — `Handle_S2S_RES_GAME_ITEM_UID_SEED`
- `GameServer/GameItemUidGen.h/.cpp` — `Init`, `Alloc`

1) Problem: 서버 재시작 후 아이템 UID를 로컬 카운터로만 발급하면 기존 DB UID와 충돌할 수 있다.
2) Implementation: GameServer 시작 시 DBAgent에 UID 시드를 요청하고, DBAgent가 `ITEMS`의 `MAX(game_item_uid)+1`을 계산해 반환하면 `GameItemUidGen::Init(next_uid)`로 발급기를 초기화한다.
3) Pros/Cons: Pros) 재시작 이후에도 UID 단조 증가/중복 방지. Cons) 부팅 시 DB 의존성이 늘고, 실패 시 시드 동기화가 불완전할 수 있다.
4) Next: UID range lease(서버별 블록 할당)로 다중 게임서버 동시 발급 경로 확장.

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

## Observability
### Prometheus Exporter + MetricsSystem — 내가 한 일: 공용 `/metrics` 노출 계층 구축
**근거**
- `ServerCore/CoreGlobal.cpp` — Metrics 설정 로드 및 `MetricsSystem::Initialize`
- `ServerCore/MetricsSystem.cpp` — 레지스트리/프로세스 메트릭/응답시간 히스토그램 관리
- `ServerCore/MetricsExporter.cpp` — HTTP `/metrics` 서버 및 Prometheus text 렌더링

1) Problem: 서버 상태를 런타임에서 정량적으로 관찰할 공용 경로가 필요했다.
2) Implementation: CoreGlobal에서 메트릭 시스템을 부팅하고 exporter가 `/metrics`를 노출해 Prometheus 수집 포맷으로 제공한다.
3) Pros/Cons: Pros) 서버 공통 관측 인프라 확보. Cons) 지표 설계가 과도하면 cardinality 관리 이슈 발생.
4) Next: 고카디널리티 라벨 가이드와 샘플링 정책 정리.

### GameServer/DBAgent 도메인 메트릭 계측 — 내가 한 일: 패킷/로비/S2S/DB 풀 지표를 서비스별로 계측
**근거**
- `GameServer/GameMetrics.cpp` — packet ingress/handle/failure, lobby wait, S2S RTT, CCU/ingame
- `DBAgent/DBAgentMetrics.cpp` — req handle/failure, query, pool wait/size/inuse
- `ServerCore/PacketMetricsHooks.h` — 핸들러 계층 공통 훅

1) Problem: `/metrics`만 열어두고 업무 지표를 넣지 않으면 병목 원인 분석이 어렵다.
2) Implementation: packet dispatch/handle 훅, 로비 로딩 대기, S2S RTT, DB pool/query 시간을 히스토그램/카운터/게이지로 계측했다.
3) Pros/Cons: Pros) 장애 원인 구간을 빠르게 좁힐 수 있음. Cons) 지표 추가 시 라벨 설계 실수 위험.
4) Next: SLA 기준 대시보드 임계치와 알림 규칙(Alerting) 추가.

### Grafana 대시보드 자산 — 내가 한 일: Prometheus 수집 경로와 대시보드 템플릿을 문서화
**근거**
- `docs/monitoring/prometheus.yml`
- `docs/monitoring/docker-compose.yml`
- `docs/monitoring/grafana_gameserver_dashboard.json`, `docs/monitoring/grafana_dbagent_dashboard.json`

1) Problem: 메트릭은 있어도 재현 가능한 시각화 구성이 없으면 운영 전환이 느리다.
2) Implementation: Prometheus/Grafana compose, 스크랩 설정, 대시보드 JSON과 쿼리 문서를 저장소에 포함했다.
3) Pros/Cons: Pros) 온보딩/재현성 향상. Cons) 대시보드 버전 동기화 관리가 필요.
4) Next: 대시보드 버전 태깅과 환경별 datasource 템플릿 자동화.

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

### DummyClient 부하 테스트 러너 — 내가 한 일: 시나리오 기반 접속/행동 부하를 자동 실행하고 CSV 리포트를 생성
**근거**
- `DummyClient/DummyClient.cpp` — 실행 엔트리/루프
- `DummyClient/LoadClientManager.h` — `LoadScenario`, `MetricsCollector`
- `DummyClient/LoadClientManager.cpp` — 램프업, RTT 수집, CSV 출력
- `DummyClient/LoadClientConfig.json` — `ccu_target`, `move_hz`, `skill_hz`, `heartbeat_hz`

1) Problem: 기능 추가 후 성능 회귀를 수치로 재현할 부하 도구가 필요했다.
2) Implementation: `idle/move/combat/mix` 시나리오, CCU 램프업, NavMesh 이동, heartbeat 송신, RTT/전송량 집계를 통합했다.
3) Pros/Cons: Pros) 반복 가능한 성능 검증 경로 확보. Cons) 실제 클라 렌더/입력 부하는 반영되지 않음.
4) Next: 시나리오별 리포트 비교 자동화와 장기 soak 테스트 추가.

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
