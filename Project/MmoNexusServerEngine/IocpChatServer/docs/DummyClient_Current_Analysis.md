# DummyClient 현재 구현 분석 (LoadClient 기준)

## 분석 범위
- 코드 기준: `DummyClient/DummyClient.cpp`, `DummyClient/LoadClientConfig.*`, `DummyClient/LoadClientManager.*`, `DummyClient/ServerPacketHandler.*`
- 목적: 현재 DummyClient가 실제로 제공하는 기능, 발생 가능한 부하 유형, 실행 가능한 시나리오를 정리

## 1. 현재 기능 요약

### 1.1 실행/제어
- 콘솔 앱 시작 시 `LoadClientConfig.json`을 로드하고 `LoadClientManager`를 초기화함.
- `io_threads` 개수만큼 IOCP dispatch 스레드를 띄우고, 메인 루프에서 `manager.Update()`를 10ms 주기로 호출함.
- `Ctrl+C`/종료 이벤트 수신 시 안전 종료.

### 1.2 접속/인증/입장 상태 머신
- 상태: `Init -> ConnectingLogin -> LoginPending -> LoginOk -> ConnectingGame -> EnterPending -> InGame`
- 실패 상태: `Failed`, 종료 상태: `Exit`
- 프로토콜 흐름:
  1. LoginServer 연결
  2. `C_LOGIN(userId, password)`
  3. `S_LOGIN` 성공 시 token 저장
  4. GameServer 연결
  5. `C_ENTER_GAME(token, channelId, mapId)`
  6. `S_ENTER_GAME` 성공 시 `InGame`
- `keep_login_connection=false`면 로그인 성공 직후 LoginServer 세션을 끊고 Game 세션만 유지.

### 1.3 계정 자동 생성/소모
- 계정명 규칙: `prefix + zero-padding(index)`
- 예: `test_000001`
- 상한: `account.start + account.count` 범위를 넘으면 신규 클라이언트 생성 불가.

### 1.4 시나리오 동작
- 지원 시나리오 문자열: `idle`, `move`, `combat`, `mix` (그 외 입력은 `idle` 처리)
- InGame 상태에서 동작:
  - `idle`: Heartbeat만 송신(heartbeat_hz > 0일 때)
  - `move`: Heartbeat + Move
  - `combat`: Heartbeat + Skill
  - `mix`: Heartbeat + Move + Skill

### 1.5 이동 생성 로직
- 기본: 스폰 클러스터 반경 내부 랜덤 워크.
- NavMesh 활성화 시:
  - 랜덤 목표점 선택 -> NavMesh path waypoint 생성
  - waypoint 추종 이동
  - 스턱 감지(ACK 위치 고정 반복) 시 강제 리패스
- 이동 송신 시 서버 clamp와 충돌을 줄이기 위해 클라이언트 측 step 크기를 보수적으로 제한함.

### 1.6 램프업/유지/종료
- `ccu_target`까지 `ramp_step` 단위로 `ramp_interval_sec`마다 점증.
- `hold_sec > 0`이면 목표 동접 도달 후 hold 시간 경과 시 자동 정지.
- `hold_sec == 0`이면 수동 종료(Ctrl+C)까지 계속 실행.

### 1.7 장애/실패 처리
- connect/login/enter 각각 timeout(ms) 초과 시 실패 처리.
- `replace_on_fail=true`이면 살아있는 클라 수 기준으로 목표 CCU를 맞추기 위해 신규 클라를 계속 투입.
- 단, 계정 풀(`account.count`) 소진 후에는 추가 대체 불가.

### 1.8 지표/로그/CSV
- 집계 지표:
  - 카운터: connect_fail, login_success/fail, enter_success/fail, errors, move_send, skill_send, heartbeat_send
  - RTT: login/enter/move/skill avg, p95
- `log_interval_sec` 주기로 콘솔 로그 출력 + CSV 스냅샷 저장.
- CSV 컬럼:
  - `timestamp_ms,active,ingame,...,login_avg,login_p95,enter_avg,enter_p95,move_avg,move_p95,skill_avg,skill_p95`

### 1.9 패킷 처리 범위
- 실제 부하/지표에 직접 사용되는 수신 패킷:
  - `S_LOGIN`, `S_ENTER_GAME`, `S_MOVE`, `S_SKILL`, `S_HEART_BEAT_RES`
- 그 외 다수 패킷(채팅/파티/던전/거래 등)은 핸들러가 등록되어 있으나 현재는 대부분 no-op(수신 후 `true` 반환)임.

## 2. DummyClient가 줄 수 있는 부하 유형

| 부하 유형 | 주요 송신 패킷 | 주 타깃 서버/모듈 | 핵심 설정 |
|---|---|---|---|
| 로그인/인증 버스트 | `C_LOGIN` | LoginServer, Redis/토큰, 계정 조회 경로 | `ccu_target`, `ramp_step`, `ramp_interval_sec` |
| 게임 입장 버스트 | `C_ENTER_GAME` | GameServer 입장/초기화 로직 | `ccu_target`, `map_id`, `channel_id` |
| 세션 유지 부하 | `C_HEART_BEAT_REQ` | GameServer 세션 관리 | `heartbeat_hz` |
| 이동/AOI 부하 | `C_MOVE` | 위치 검증, AOI/브로드캐스트, Room 업데이트 | `scenario=move/mix`, `move_hz`, `spawn_cluster`, `navmesh.*` |
| 전투 액션 부하 | `C_SKILL` | 스킬 처리, 전투/판정 경로 | `scenario=combat/mix`, `skill_hz` |
| 혼합 게임플레이 부하 | `C_MOVE + C_SKILL + C_HEART_BEAT_REQ` | Game loop 전반 | `scenario=mix`, `move_hz`, `skill_hz`, `heartbeat_hz` |
| 실패 대체/재접속 부하 | (재연결 + 재로그인 재입장) | Login/Game 접속 경로, 세션 생성/정리 | `replace_on_fail`, timeout 설정 |

## 3. 부하량 계산(대략)

정상적으로 InGame에 올라온 클라이언트를 `N`이라 하면,

- Heartbeat PPS: `N * heartbeat_hz`
- Move PPS: `N * move_hz` (`scenario`가 `move` 또는 `mix`일 때)
- Skill PPS: `N * skill_hz` (`scenario`가 `combat` 또는 `mix`일 때)
- 총 C2S PPS(steady-state):
  - `N * (heartbeat_hz + move_hz + skill_hz)`
  - 단, 시나리오별로 비활성 항목은 0

램프업 구간의 신규 세션 투입률(상한 근사):
- `ramp_step / ramp_interval_sec` clients/sec

주의:
- 실제 처리량은 timeout, 실패율, 계정 풀 크기(`account.count`), 서버 ACK 지연에 따라 변동됨.
- `replace_on_fail=true`여도 계정이 부족하면 목표 CCU 유지가 깨질 수 있음.

## 4. 현재 사용 가능한 테스트 시나리오

### 시나리오 A: Idle CCU 안정성
- 목적: 연결 유지/세션 관리 비용 측정
- 예시 설정:
  - `scenario: "idle"`
  - `heartbeat_hz: 1`
  - `move_hz: 0`, `skill_hz: 0`

### 시나리오 B: Move(AOI) 집중 부하
- 목적: 이동 검증 + 근접 플레이어 브로드캐스트 부하 확인
- 예시 설정:
  - `scenario: "move"`
  - `move_hz: 6~10`
  - `spawn_cluster.radius`를 작게 설정해 밀집도 증가
  - `navmesh.enabled: true` 권장

### 시나리오 C: Combat 부하
- 목적: 스킬 처리 루프/전투 경로 부하 확인
- 예시 설정:
  - `scenario: "combat"`
  - `skill_hz: 0.5~2`
  - `move_hz: 0`

### 시나리오 D: Mix 종합 부하
- 목적: 실제 플레이 유사 종합 부하(이동+스킬+하트비트)
- 예시 설정:
  - `scenario: "mix"`
  - `move_hz: 4~8`
  - `skill_hz: 0.5~1.5`
  - `heartbeat_hz: 1`

### 시나리오 E: Soak(장시간)
- 목적: 장시간 안정성/누수/지연 드리프트 확인
- 방법:
  - 목표 CCU 도달 후 `hold_sec`를 크게 설정(예: 1800~3600)
  - `log_interval_sec` 단위로 CSV 추세 확인

### 시나리오 F: 실패 대체 내구성
- 목적: 장애 상황에서 CCU 복원 및 재접속 churn 확인
- 방법:
  - `replace_on_fail: true`
  - 짧은 timeout 또는 서버 재시작 이벤트를 주고 active/inGame 회복 추적

## 5. 한계 및 해석 시 주의
- DummyClient는 현재 채팅/파티/거래/던전 시나리오를 능동적으로 발생시키지 않음(핸들러 대부분 no-op).
- Heartbeat RTT는 별도 통계로 계산하지 않음(송신 카운트 중심).
- CSV 경로 디렉터리 자동 생성 로직이 없으므로 경로가 없으면 파일 출력이 실패할 수 있음.
- 단일 PC 풀스택 결과는 절대 성능값보다 회귀/비교 지표로 해석하는 것이 적절함.

## 6. 빠른 체크리스트
- `ccu_target <= account.count`인지 확인
- `scenario` 문자열 오탈자 확인(오탈자 시 idle로 동작)
- `navmesh.enabled=true`인 경우 `maps_path/navmesh_path` 유효성 확인
- `docs/perf_results` 같은 CSV 대상 폴더 사전 생성 확인
- `replace_on_fail` 정책이 테스트 의도(복원 vs 순수 실패 관찰)와 맞는지 확인
