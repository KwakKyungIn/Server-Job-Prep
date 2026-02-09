# Prometheus 모니터링 구현 3단계 실행 계획서

## 문서 목적
- 이 문서는 AI 프롬프트 모음이 아니라, 구현 작업을 3단계로 분할한 실행 계획서다.
- 각 단계는 독립 커밋/검증 단위로 수행한다.

## 현재 상태 재검토

### 적절한 점
- `ServerCore` 공용 레이어(`GlobalQueue`, `JobQueue`, `ThreadManager`, `Session`)가 명확해서 공통 계측 추가가 쉽다.
- `GameServer`/`DBAgent` 모두 패킷 진입점과 메인 루프가 분명해 계측 포인트 선정이 가능하다.
- `GameServer/ServerConfig.json`, `DBAgent/ServerConfig.json`에 `Metrics` 섹션이 이미 존재한다.

### 보완 필요한 점
- `ServerCore/CoreGlobal.cpp`에서 `Metrics` 설정을 아직 실제로 파싱/적용하지 않는다.
- `ThreadManager::DoGlobalQueueWork()`가 `while (true)`라 종료 동기화 기준이 필요하다.
- 라벨 허용 집합/카디널리티 예산이 문서에 고정되어 있지 않다.
- 현재 문서는 구현 순서보다 계측 항목 나열 비중이 커서 작업 분배 용도로는 부족했다.

### 범위 명확화
- 본 계획의 exporter 적용 대상 프로세스는 `GameServer`, `DBAgent`다.
- `ThreadManager`/`CoreGlobal` 변경은 `LoginServer`에도 공통 반영되므로 종료 경로 회귀를 함께 확인한다.

---

## 공통 구현 원칙
- 핫패스는 `atomic/thread_local` 중심으로 처리하고 잠금 경합을 추가하지 않는다.
- 메트릭 라벨은 저카디널리티 고정 집합만 허용한다.
- `playerId`, `sessionId`, 닉네임, SQL 원문은 라벨로 금지한다.
- `/metrics` 기본 바인딩은 `127.0.0.1`로 시작한다.
- 단계 완료 전 범위 외 리팩터링은 하지 않는다.

---

## 필수 결정사항 확정(1~7)

### 1. Histogram 버킷 정책 (확정)
- 단위 표기는 `ms`로 관리하고, 실제 Prometheus 기록은 `seconds`로 변환해 사용한다.
- 패킷 처리(`packet_handle`) 버킷(ms): `0.001, 0.005, 0.01, 0.05, 0.1, 0.5, 1.0, 5.0`
- DB 쿼리(`db_query`) 버킷(ms): `0.01, 0.05, 0.1, 0.5, 1.0, 5.0, 10.0`
- 큐 대기(`jobqueue_wait`) 버킷(ms): `0.001, 0.01, 0.1, 1.0, 10.0, 100.0`

### 2. JobQueue timestamp 측정 방식 (A 선택)
- 선택: `방법 A` (Job 구조체에 enqueue timestamp 추가)
- 적용 방식:
- `Job`에 `enqueueTick` 필드를 추가한다.
- enqueue 시점은 `JobQueue::Push()`에서 기록한다.
- wait 시간은 `JobQueue::Execute()`가 큐에서 꺼내는 시점 기준으로 계산한다.
- 선택 이유:
- `Thread_local` 근사값 방식은 다중 producer/consumer 구조에서 오차가 커진다.
- 현재 코드 구조에서 `Job`은 모든 작업의 공통 단위라 침투 대비 정확도 이점이 크다.

### 3. S2S RTT 측정 방식 (A 선택)
- 선택: `방법 A` (요청-응답 상관키 map 매칭)
- 적용 방식:
- 요청 송신 시 `(op, correlationKey) -> startTick`을 저장한다.
- 응답 수신 시 동일 키로 조회하여 RTT를 기록하고 map에서 제거한다.
- TTL(예: 30초) 청소 루틴을 둬서 유실 응답/누락 키를 정리한다.
- map 접근은 mutex 보호로 구현하고, 상한(예: 10,000개) 초과 시 경고 후 신규 삽입을 제한한다.
- 상관키 규칙:
- `LOAD_*` 계열: `gameSessionId`
- `ITEM_CREATE`, `TRADE_COMMIT`: `requestId`
- `SAVE_*` 계열: `playerId` (현재 AutoCommit inflight 제약을 전제로 허용)
- 선택 이유:
- 이 저장소의 S2S 프로토콜은 이미 상관 필드를 다수 보유하고 있어 정확 측정이 가능하다.
- queue wait 근사값은 네트워크 RTT가 아니라 내부 스케줄링 지연만 반영해 목적과 다르다.

### 4. Exporter accept 블로킹 해제 (확정)
- 목표: 1초마다 종료 플래그(`GIsRunning`)를 확인 가능한 구조로 구현한다.
- 구현 기준:
- exporter 소켓 accept 루프에 1000ms 타임아웃 폴링을 적용한다.
- 구현은 `select/poll + 1000ms`를 우선 사용한다.
- 소켓 옵션 방식으로 구현할 경우 동일하게 1초 주기 확인이 보장되어야 한다.

### 5. 라벨 카디널리티 상한 (확정)
- 고정 enum:
- `queue`: 최대 3
- `reason`: 최대 3
- `mode`: 최대 2
- 동적 허용:
- `op`: 최대 50
- 상한 규칙:
- 최대 시계열 목표는 900개 이내로 유지한다.
- 상한 초과 라벨 값은 `other`로 폴딩한다.

### 6. `process_cpu` 플랫폼 구현 (확정)
- Windows: `GetProcessTimes()`
- Linux: `/proc/self/stat` 파싱
- 공통 인터페이스(`ProcessMetricsProvider`)로 추상화한다.

### 7. `/metrics` 응답시간 SLA (확정)
- 목표: `100ms` 이하
- 한계: `500ms` 이하
- 초과 시 조치:
- registry snapshot/문자열 렌더링 경로 프로파일링
- 고비용 라벨/히스토그램 축소

---

## 구현 1단계: 계측 인프라 뼈대 구축

### 목표
- 공용 Metrics 모듈과 exporter를 붙여 `GameServer`, `DBAgent`에서 `/metrics`를 안정적으로 노출한다.

### 작업 태스크(상세)
1. `ServerConfig` 확장
- `Metrics.Enabled`, `Metrics.Port`, `Metrics.Prefix`, `Metrics.Path`, `Metrics.BindAddress` 구조를 `ServerCore/CoreGlobal.h`에 반영한다.
- `CoreGlobal` JSON 로딩에 기본값/누락 처리 로직을 넣는다.
- 기존 `GameServer/ServerConfig.json`, `DBAgent/ServerConfig.json`과 호환되도록 누락 필드는 기본값으로 보정한다.

2. 공용 Metrics 자료구조 추가
- `Counter`, `Gauge`, `Histogram` 타입 구현.
- `MetricsRegistry`에서 메트릭 등록과 스냅샷 조회를 지원.
- Histogram 버킷 정책(패킷/DB/큐대기 3종)을 코드 상수로 고정한다.
- `Histogram` 기록 입력 단위는 내부 `seconds`로 통일하고, 호출부에서 ms->seconds 변환 규칙을 명시한다.

3. Prometheus 텍스트 렌더러 구현
- `# HELP`, `# TYPE`, 샘플 라인을 Prometheus 포맷으로 출력한다.
- 라벨 출력 순서를 고정해 diff/검증이 가능하게 만든다.

4. Exporter 서버 구현
- 별도 스레드에서 `GET /metrics` 처리.
- 경로 불일치 시 404, 메서드 불일치 시 405 반환.
- 설정으로 켜고 끌 수 있게 한다.
- accept 루프는 1000ms 타임아웃 폴링으로 종료 신호를 확인한다.
- 서버 소켓 바인드 실패 시 프로세스 전체를 죽이지 않고, metrics 비활성 상태로 기동 계속(경고 로그) 정책을 사용한다.

5. 수명주기 연결
- 서버 시작 시 exporter 시작, 종료 시 join/정리.
- `GIsRunning` 기준으로 worker/exporter 루프 종료 규칙 통일.

6. 최소 공통 메트릭 연결
- `process_uptime_seconds`
- `process_cpu_seconds_total{mode="user|system"}`
- `process_resident_memory_bytes`
- `/metrics` 응답시간 측정(목표 100ms, 한계 500ms)

7. 프로젝트 파일 반영
- 신규 소스/헤더는 `ServerCore/ServerCore.vcxproj`와 `ServerCore/ServerCore.vcxproj.filters`에 모두 등록한다.
- `GameServer`/`DBAgent`에서 새 헤더 참조가 생기면 각 프로젝트의 `.vcxproj/.filters`도 동기화한다.

### 1단계 진행 상태
- 상태: 완료 (`2026-02-09`)
- 기획 대비 차이: 없음
- 비고: 수명주기 연결은 `CoreGlobal` 시작/종료 경로에 exporter start/stop을 붙여 공통 처리로 반영했다.

### 예상 변경 파일
- `ServerCore/CoreGlobal.h`
- `ServerCore/CoreGlobal.cpp`
- `ServerCore/ThreadManager.cpp`
- `ServerCore/ServerCore.vcxproj`
- `GameServer/GameServer.cpp`
- `DBAgent/DBAgent.cpp`
- 신규 Metrics 관련 `.h/.cpp`

### 단계 완료 기준
- `GameServer:8080/metrics`, `DBAgent:8081/metrics`가 정상 응답한다.
- `Metrics.Enabled=false`일 때 기존 동작 회귀가 없다.
- 30분 구동 중 크래시/데드락/종료 행이 없다.

### 단계 산출물
- 공용 계측 모듈 코드
- 설정 스키마 반영
- `/metrics` 수동 확인 로그

---

## 구현 2단계: Core + GameServer 런타임 계측 삽입

### 목표
- Core 스케줄링 병목과 GameServer 요청 지연을 수치로 추적할 수 있게 만든다.

### 작업 태스크(상세)
1. Core Queue/Worker 계측
- `GlobalQueue::Push`에서 push 카운트 및 shard depth 증가.
- `GlobalQueue::Pop`에서 `hit/miss` 카운트.
- Steal 시도/성공 카운트 분리.
- `ThreadManager::DoGlobalQueueWork`에서 idle/exec 시간 누적.
- `worker_active` gauge 관리.

2. JobQueue 계측
- `JobQueue::Push`에서 depth 반영.
- `JobQueue::Execute`에서 batch size histogram 반영.
- wait/exec 측정을 위해 `Job` enqueue timestamp 방식(A안)을 반영한다.
- `ServerCore/Job.h` 변경 시 기존 `ObjectPool<Job>::MakeShared(...)` 호출 패턴과 ABI 호환을 깨지 않도록 기본 생성/호출 경로를 유지한다.

3. Session I/O 계측
- `Session::ProcessRecv`, `Session::ProcessSend`에서 RX/TX bytes 누적.

4. GameServer 패킷/로비 계측
- `ClientPacketHandler::HandlePacket` 진입 카운트.
- `Handle_C_*` 처리 시간 histogram.
- 파싱/검증/핸들러 실패 사유를 고정 reason 코드로 계측.
- `LobbyRoom::EnterGame` 시작부터 `TryEnterWorldIfReady` 완료까지 대기 시간 계측.
- S2S 요청-응답 RTT는 상관키 map 방식(A안)으로 계측.

5. Game 상태 지표
- `gs_ccu` (세션 기준)
- `gs_ingame_players` (PlayerId 바인딩 기준)

6. 라벨 정책 고정
- 허용 라벨 집합: `type`, `queue`, `reason`, `shard`, `op`
- 허용 값 집합을 enum/상수 테이블로 관리.
- 카디널리티 상한(최대 시계열 900) 초과 시 `other` 폴딩.
- `type/op` 문자열은 패킷ID/쿼리ID를 코드 테이블로 매핑해 생성하고, 런타임 자유 문자열은 금지한다.

### 2단계 진행 상태
- 상태: 완료 (`2026-02-09`)
- 기획 대비 차이:
- PacketHandler 계측 삽입은 자동 생성 파일 직접 수정 대신 `GenPackets` 템플릿(`PacketHandler.h`) 훅 방식으로 반영했다.
- 비고:
- S2S RTT는 `S2SPacketHandler`의 send/parse 훅을 사용해 `(op, correlationKey)` inflight map 매칭/TTL/상한 정책으로 계측한다.

### 예상 변경 파일
- `ServerCore/GlobalQueue.cpp`
- `ServerCore/JobQueue.cpp`
- `ServerCore/ThreadManager.cpp`
- `ServerCore/Session.cpp`
- `GameServer/ClientPacketHandler.h`
- `GameServer/ClientPacketHandler.*.cpp`
- `GameServer/LobbyRoom.cpp`
- `GameServer/S2SPacketHandler.cpp`
- `GameServer/GameSessionManager.*`

### 단계 완료 기준
- Prometheus 쿼리에서 다음이 확인된다.
- `gs_packet_handle_seconds` p95
- `gs_jobqueue_wait_seconds` p95
- `gs_jobqueue_exec_seconds` p95
- `gs_lobby_wait_seconds` p95
- `*_worker_idle_seconds_total`, `*_worker_exec_seconds_total`
- 로그인→입장→이동/스킬 수동 테스트에서 기능 회귀가 없다.

### 단계 산출물
- Core/GameServer 계측 코드
- 메트릭 이름/타입/라벨 목록표
- 수동 회귀 테스트 로그

---

## 구현 3단계: DBAgent 계측 + 대시보드 + 결과 문서화

### 목표
- DB 병목을 요청 처리/쿼리/풀 대기로 분리해 설명 가능한 상태를 만든다.

### 작업 태스크(상세)
1. DBAgent 요청 경로 계측
- `DBAgentPacketHandler` 요청 타입별 카운트.
- 요청 처리 시간 histogram.

2. DB 풀 계측
- `DBConnectionPool`에서 `pool_size`, `pool_inuse` gauge.
- `Pop()` 대기 시간 histogram.

3. DB 쿼리 계측
- 쿼리 구간 시간 측정.
- SQL 원문 라벨 금지, 고정 `op` 라벨 사용.

4. DBAgent 큐 계측
- 필요 시 DBAgent 쪽 JobQueue wait/exec 분리.

5. 관측 구성 정리
- Prometheus `scrape_configs` 최종 반영.
- Grafana 대시보드 패널/쿼리 정리.

6. 포트폴리오 결과 문서화
- 부하 시나리오(예: 100/300/500 CCU) 고정.
- 워밍업/측정 구간 분리.
- 개선 전/후 비교 템플릿 작성.

### 예상 변경 파일
- `DBAgent/DBAgentPacketHandler.cpp`
- `DBAgent/DBConnectionPool.cpp`
- `DBAgent/DBAgent.cpp`
- `docs/` 하위 결과 문서
- Prometheus/Grafana 설정 파일

### 단계 완료 기준
- `db_req_handle_seconds`, `db_query_seconds`, `db_pool_wait_seconds` 3축 분석 가능.
- 개선 전/후 비교 그래프를 최소 3개 이상 확보.
- 재현 절차 문서가 독립적으로 실행 가능하다.

### 단계 산출물
- DBAgent 계측 코드
- 대시보드/쿼리 문서
- 비교 리포트 템플릿

---

## 단계 게이트(진행 규칙)
- 1단계 완료 후 2단계 진행.
- 2단계 완료 후 3단계 진행.
- 각 단계는 별도 커밋으로 관리.
- 단계 완료 시점마다 빌드/기동/기능 회귀 확인을 먼저 수행.

## 최종 검증 체크리스트
- 서버 시작/종료가 기존과 동일한가
- `/metrics`가 1초 스크랩에서도 안정적인가
- 핫패스 락 경합이 증가하지 않았는가
- 고카디널리티 라벨 유입이 없는가
- 부하 테스트 결과가 동일 시나리오로 재현 가능한가
