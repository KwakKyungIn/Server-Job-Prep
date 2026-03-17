# DummyClient Load Client Design (Step 1)

## 목표
- 단일 PC에서 **LoginServer + GameServer + DBAgent + Redis + MSSQL**을 모두 포함한 풀스택 부하 테스트 수행.
- 기존 DummyClient를 **다중 세션 기반 로드 클라이언트**로 개조.
- 포트폴리오용 지표를 안정적으로 재현.

## 범위
- DummyClient 프로젝트 코드만 변경.
- 기존 Protocol.proto 기반 패킷 사용.
- 게임 클라이언트 UI 없이 자동 실행.

## 핵심 변경점
- 콘솔 입력 제거.
- 단일 세션 → 다중 세션.
- 로그인 흐름 분리.
- 설정 파일 기반 실행.
- 시나리오 스케줄러 추가.
- 통계 수집 및 결과 파일 저장.

## 프로토콜 흐름
1. LoginServer(7780) 연결
2. `C_LOGIN(userId, password)` 송신
3. `S_LOGIN` 수신 후 token 확보
4. LoginServer 연결 종료 또는 유지
5. GameServer(7777) 연결
6. `C_ENTER_GAME(token, channelId, mapId)` 송신
7. `S_ENTER_GAME` 수신 후 InGame

## 상태 머신
- Init
- ConnectLogin
- LoginPending
- ConnectGame
- EnterGamePending
- InGame
- Exit
- Failed

## 시나리오 모델
### Scenario: Idle
- InGame 상태 유지
- 주기적 HeartBeat만 송신

### Scenario: Move
- 일정 반경 내 랜덤 워크
- `C_MOVE` 송신 주기: 기본 5~10Hz
- 목적: AOI 브로드캐스트 부하

### Scenario: Combat
- `C_SKILL` 주기 송신
- 목적: 전투/AI/드랍 부하
- 기본 스킬 ID는 1 사용

### Scenario: Mix
- Move + Combat 혼합
- 비율 설정 가능

## 부하 스케줄
- Ramp-up 단계 적용
- 예시: 50 → 100 → 200 → 300 → 500 CCU
- 단계 유지 시간: 5~10분

## 계정 정책
- DB에 사전 생성된 계정 사용
- 계정 이름 패턴 기반 자동 생성
- 예: `test_000001` ~ `test_000500`

## DummyClient 구성 구조
### 주요 클래스
- `LoadClientManager`
- `SimSession`
- `ScenarioScheduler`
- `MetricsCollector`

### 역할 요약
- `LoadClientManager`
- 설정 파일 로드
- 세션 생성 및 램프업 관리
- 전체 통계 집계

- `SimSession`
- 상태 머신 수행
- 로그인/입장 흐름 처리
- 송신 스케줄 실행

- `ScenarioScheduler`
- 시나리오별 주기 관리
- Move/Skill/Heartbeat 타이밍 제어

- `MetricsCollector`
- RTT/성공률/오류율 기록
- CSV 저장

## 설정 파일 스펙
### 파일명
- `DummyClient/LoadClientConfig.json`

### 스키마
```json
{
  "login_server": { "ip": "127.0.0.1", "port": 7780 },
  "game_server": { "ip": "127.0.0.1", "port": 7777 },

  "ccu_target": 300,
  "ramp_step": 50,
  "ramp_interval_sec": 10,
  "hold_sec": 300,

  "channel_id": 1,
  "map_id": 2,

  "scenario": "move",
  "move_hz": 6,
  "skill_hz": 1,
  "heartbeat_hz": 1,

  "spawn_cluster": {
    "center_x": 512.0,
    "center_y": 0.0,
    "center_z": 512.0,
    "radius": 120.0
  },

  "account": {
    "prefix": "test_",
    "start": 1,
    "count": 500,
    "pad_width": 6,
    "password": "pw"
  },

  "timeouts": {
    "connect_ms": 3000,
    "login_ms": 5000,
    "enter_ms": 5000
  },

  "options": {
    "keep_login_connection": false,
    "log_interval_sec": 5,
    "csv_output": true,
    "csv_path": "docs/perf_results/run.csv"
  }
}
```

## 통계 수집 항목
- connect_success, connect_fail
- login_success, login_fail, login_rtt_ms
- enter_success, enter_fail, enter_rtt_ms
- move_send, move_recv
- skill_send, skill_recv
- heartbeat_send, heartbeat_recv
- error_count, timeout_count

## RTT 측정 방식
- 로그인 RTT: `C_LOGIN` 송신 시간과 `S_LOGIN` 수신 시간
- 입장 RTT: `C_ENTER_GAME` 송신 시간과 `S_ENTER_GAME` 수신 시간
- 이동 RTT: `C_MOVE` 송신 시간과 `S_MOVE` 수신 시간
- 이동 RTT는 `objectId`가 내 플레이어와 일치할 때만 측정

## 패킷 핸들러 수정 포인트
- `S_LOGIN` 처리 후 token 저장
- `S_ENTER_GAME` 처리 후 myPlayer 정보 저장
- `S_MOVE`, `S_SKILL` 수신 통계 기록
- `S_SPAWN`, `S_DESPAWN`은 기본 로그 최소화

## 로그/출력 정책
- 콘솔 출력은 요약 단위로 제한
- CSV 저장은 5초 단위 스냅샷
- 예시 항목: timestamp, ccu_active, avg_login_rtt, p95_login_rtt, avg_enter_rtt, p95_enter_rtt, errors

## 실행 흐름
1. 설정 파일 로드
2. 램프업 시작
3. 시나리오 스케줄 실행
4. 유지 시간 종료 후 종료

## 실패 처리
- 로그인/입장 실패 시 재시도 가능
- 연결 실패 시 일정 시간 후 재시도
- 최대 재시도 횟수 초과 시 세션 종료

## 주의사항
- 단일 PC 환경은 네트워크/CPU 병목이 겹침
- LoginServer/DBAgent 부하가 GameServer 결과에 영향을 줄 수 있음
- CCU는 “측정 결과”로만 표기
