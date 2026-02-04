# Load Test Plan (Single PC, Full Stack)

## 목적
- 포트폴리오에 넣을 수 있는 **정량 성능 지표** 확보.
- IOCP + RoomActor + AOI + DB/Redis 연동을 포함한 **풀스택 부하** 검증.
- 단일 PC 환경에서 현실적인 **안정 동접(CCU)** 범위 측정.

## 핵심 원칙
- “최대 동접 수치”는 **사양/환경에 종속**됨.
- 단일 PC 결과는 **참고용(베이스라인)**임을 명확히 표기.
- 수치는 “목표”가 아니라 **측정 결과**로 제시.

## 추천 CCU 목표(포폴 기준)
- 단일 PC에서는 **100~500 CCU** 구간이 일반적으로 설득력이 있음.
- 실제 표기 값은 **측정으로 얻은 안정 구간**으로 결정.
- 추천 표기 방식:
  - “단일 PC 풀스택 환경에서 **X CCU에서 평균/95p 지연 안정**”
  - “Y CCU부터 95p 지연 급증/오류율 증가”

## 테스트 범위
- 대상 서버: LoginServer, GameServer, DBAgent, Redis, MSSQL
- 대상 플로우: 로그인 → 토큰 발급 → 입장 → 이동/전투 → DB 쓰기
- 단일 PC에서 **모든 서버 + 로드 클라이언트** 동시 실행

## 사전 준비
### 계정 데이터
- `Players` 테이블에 대량 계정 시드 필요
- 로그인은 현재 `SELECT playerId FROM Players WHERE name = ?` 방식
- DummyClient는 계정 목록을 순회하며 로그인

### 필수 서비스
- Redis 실행 필요 (LoginServer 토큰 저장/검증)
- MSSQL 실행 필요 (DBAgent가 직접 접근)

### 네트워크
- 현재는 127.0.0.1 기반 Loopback 테스트

## 부하 시나리오 정의
### Scenario A: Idle CCU
- 로그인/입장 완료 후 아무 행동 없음
- 목적: 연결 유지, 세션 관리 비용 측정

### Scenario B: Move + AOI
- 동일 맵/근접 좌표에 몰아서 `C_MOVE` 송신
- 송신 주기: 5~10 Hz
- 목적: AOI 갱신, 브로드캐스트 비용 측정

### Scenario C: Combat
- 몬스터 근처에서 `C_SKILL` 주기 송신
- 주기: 0.5~1 Hz
- 목적: AI/전투/드랍 처리 부하 측정

### Scenario D: Soak
- 안정 CCU에서 30~60분 유지
- 목적: 메모리 누수, 장기 안정성 확인

## 부하 스케줄(권장)
- Ramp-up 단계: 50 → 100 → 200 → 300 → 500 CCU
- 단계 유지 시간: 5~10분
- 종료 조건:
  - 로그인/입장 95p 지연 1초 초과
  - 이동 처리 95p 지연 200ms 초과
  - 오류율 1% 초과

## 측정 지표
### 클라이언트 지표
- 로그인 RTT: `C_LOGIN` → `S_LOGIN`
- 입장 RTT: `C_ENTER_GAME` → `S_ENTER_GAME`
- 이동 RTT: `C_MOVE` → `S_MOVE`
- 전투 RTT: `C_SKILL` → `S_SKILL`
- 오류율: connect/login/enter 실패 비율

### 서버 지표
- GameRoom Update 평균/95p 실행시간
- DBAgent 쿼리 평균/95p 시간
- Redis GET/SET 평균/95p 시간
- CPU/Memory/Network 사용률

## DummyClient 개조 기획
### 목표
- 단일 콘솔 입력 제거, **다중 세션 부하 클라이언트**로 변경
- **LoginServer → GameServer** 분리 접속
- 시나리오별 행동 스케줄링

### 상태 머신 설계
- Init
- ConnectLogin
- LoginPending
- ConnectGame
- EnterGamePending
- InGame
- Exit

### 프로토콜 흐름
1. LoginServer(7780)로 연결
2. `C_LOGIN(userId, password)` 송신
3. `S_LOGIN` 수신 후 token 확보
4. GameServer(7777)로 연결
5. `C_ENTER_GAME(token, channelId, mapId)` 송신
6. `S_ENTER_GAME` 수신 후 InGame 진입

### 설정 파일 (예시 스펙)
- 파일명: `DummyClient/LoadClientConfig.json`
- 주요 항목:
  - `ccu_target`
  - `ramp_step`
  - `ramp_interval_sec`
  - `scenario` (idle/move/combat)
  - `move_hz`, `skill_hz`
  - `map_id`, `channel_id`
  - `account_prefix`, `account_start`, `account_count`

### 통계 출력
- 표준 출력 + CSV 저장 권장
- 결과 경로 예: `docs/perf_results/YYYYMMDD_runX.csv`

## 서버 계측(권장)
### GameServer
- `GameRoom::Update()` 실행 시간 측정
- 1초 단위 평균/95p 로그

### DBAgent
- 쿼리별 실행 시간 로깅
- 쿼리 종류별 평균/95p

### Redis
- 토큰 GET/SET 시간 로깅(로그 레벨 조절)

## 결과 문서화(포트폴리오 기준)
- 환경 스펙 명시
- 테스트 시나리오 명시
- 안정 CCU와 한계 CCU 분리 표기
- 그래프: CCU vs RTT, CCU vs CPU

## 리스크 및 주의사항
- 단일 PC 테스트는 네트워크/IO 병목이 실제 운영과 다름
- LoginServer/DBAgent에 부하 집중 시 GameServer 성능이 왜곡될 수 있음
- AOI 테스트 시 좌표 분산/집중 방식에 따라 결과 크게 변동

## 다음 단계
1. DummyClient 개조 작업 시작
2. 계정 시드 스크립트 준비
3. 단계별 부하 실행 및 결과 수집

