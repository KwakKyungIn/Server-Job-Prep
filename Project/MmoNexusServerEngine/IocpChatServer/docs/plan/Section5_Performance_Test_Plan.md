# Section5 성능 테스트 기획서

## 문서 목적
- 이 문서는 포트폴리오 `section5` 제작을 위한 실험 설계 문서다.
- 목표는 단순 부하 테스트가 아니라, 지금까지 구현한 서버 구조와 기능 선택이 왜 필요한지 `Prometheus + Grafana` 기반 데이터로 증명하는 것이다.
- 다른 AI가 이 문서만 받아도 구현 준비, 계측 추가, 실험 수행, 결과 정리까지 이어서 작업할 수 있도록 상세히 작성한다.

## Section5 구성 방향
- `Part A. 성능 검증 A/B`
- `Part B. 운영 안정성 검증`

### Part A 대상 시나리오
- `Hot Room Mix`
- `Persistence Drain`

### Part B 대상 시나리오
- `Enter Burst`
- `Idle Soak`

## 핵심 원칙
- `Hot Room Mix`, `Persistence Drain`는 `최종본(B)`과 `의도적으로 단순화한 baseline(A)`를 비교한다.
- `Enter Burst`, `Idle Soak`는 A/B 없이 `최종본 단독 검증`으로 간다.
- 모든 근거 데이터는 `Prometheus + Grafana`에서만 확보한다.
- `DummyClient CSV`는 section5 근거 자료로 사용하지 않는다.
- 모든 실험은 `동일 머신`, `동일 Release 빌드`, `동일 초기 데이터`, `동일 scrape 조건`, `동일 더미 설정`으로 수행한다.
- section5의 목적은 `최대 CCU 자랑`이 아니라 `구조 선택의 타당성 증명`이다.

---

## 1. 공통 실험 규칙

### 1.1 공통 환경 고정
- 서버 빌드: `x64 Release`
- 서버 프로세스: `LoginServer`, `DBAgent`, `GameServer`
- 더미클라이언트는 부하 생성 전용으로 사용
- Prometheus scrape interval: `1s`
- Grafana 기본 range: `15m`
- soak 전용 range: `1h`
- `replace_on_fail=false` 고정
- 계정 풀은 각 시나리오 `ccu_target`보다 충분히 크게 준비
- DB/Redis 초기 데이터셋과 계정 상태를 매 실험 전에 동일하게 맞춘다

### 1.2 메트릭 prefix 표준화
기존 `ServerConfig.json`에는 Metrics 설정이 비어 있으므로, 실험 전 각 서버 설정에 아래 prefix를 추가한다.

- `GameServer`: `gs`
- `DBAgent`: `db`
- `LoginServer`: `ls`

권장 예시:

```json
{
  "Metrics": {
    "Enabled": true,
    "Port": 8080,
    "Prefix": "gs",
    "Path": "/metrics",
    "BindAddress": "0.0.0.0"
  }
}
```

권장 포트:
- `GameServer`: `8080`
- `DBAgent`: `8081`
- `LoginServer`: `8082`

### 1.3 공통 결과 집계 규칙
- 모든 시나리오는 최소 `3회` 반복
- 최종 비교값은 `steady-state 구간` 기준으로 계산
- ramp-up 구간과 steady-state 구간을 분리해 기록
- 보고 시에는 `대표 run 1개 + 3회 반복 범위` 또는 `median`을 함께 쓴다
- 절대 최고치보다 `동일 조건 대비 변화량`을 우선 해석한다

### 1.4 공통 산출물
- Prometheus 시계열
- Grafana 패널 스크린샷
- Grafana panel/export JSON 또는 쿼리 문서
- 최종 비교표
- 실험 설정 JSON
- 실험 당시 git commit hash 또는 `Experiment` 모드 값

### 1.5 공통 주의사항
- 단일 PC 풀스택 결과는 절대적인 서비스 규모 수치가 아니라 `구조 비교`와 `회귀 검출` 중심으로 해석한다.
- baseline(A)은 일부 시나리오에서 최종 제품 계약과 완전히 같지 않을 수 있다. 이 경우 `실험용 단순화 모드`임을 포트폴리오 문구에 명시한다.
- 더미클라이언트 내부 CSV나 콘솔 로그는 디버깅 참고 자료일 뿐, section5 공개 자료의 근거로 쓰지 않는다.

---

## 2. 고정 시나리오 프로필

이 문서에서는 모든 시나리오의 기본값을 아래와 같이 고정한다. 다른 AI는 특별한 사유가 없으면 이 값을 그대로 사용한다.

| 시나리오 | ccu_target | ramp_step | ramp_interval_sec | hold_sec | scenario | move_hz | skill_hz | heartbeat_hz | quickslot_hz | channel_id | map_id | spawn_radius | navmesh |
| --- | ---: | ---: | ---: | ---: | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| Hot Room Mix | 250 | 50 | 10 | 180 | mix | 6 | 1 | 1 | 0 | 1 | 2 | 120 | enabled |
| Persistence Drain | 200 | 50 | 10 | 180 | persistence | 0 | 0 | 1 | 1 | 1 | 1 | 0 | disabled |
| Enter Burst | 300 | 100 | 5 | 120 | idle | 0 | 0 | 1 | 0 | 1 | 1 | 0 | disabled |
| Idle Soak | 200 | 50 | 10 | 1800 | idle | 0 | 0 | 1 | 0 | 1 | 1 | 0 | disabled |

### 2.1 공통 DummyClient 옵션
- `keep_login_connection=false`
- `replace_on_fail=false`
- `log_interval_sec=5`
- `csv_output=false`
- `io_threads=2`

### 2.2 Persistence Drain용 QuickSlot 패턴 고정
- 슬롯 범위: `0, 1, 2, 3`
- 참조 타입: `QS_SKILL`
- skillId 순환: `1 -> 2 -> 3 -> 1 ...`
- 매 4번째 이벤트는 `QS_NONE`으로 clear
- 송신 주기: `quickslot_hz=1`
- 목적: 같은 의미의 mutation rate를 안정적으로 만들고, 인벤토리/아이템 정합성 변수는 최소화한다

---

## 3. 실험용 설정/토글 설계

수동 주석 토글이나 별도 임시 브랜치 대신, 같은 코드베이스에서 `ServerConfig`만 바꿔 재현 가능한 런타임 실험 모드를 사용한다.

### 3.1 `ServerConfig`에 추가할 섹션

```json
{
  "Experiment": {
    "HotRoomAoiMode": "final",
    "PersistenceMode": "writeback",
    "EnableLoginMetrics": true
  }
}
```

### 3.2 권장 enum 값
- `HotRoomAoiMode`
  - `final`
  - `room_wide_baseline`
- `PersistenceMode`
  - `writeback`
  - `immediate_quickslot`

### 3.3 구현 위치
- `ServerCore/CoreGlobal.h`
- `ServerCore/CoreGlobal.cpp`

### 3.4 설계 원칙
- baseline 실험은 `같은 코드 + 다른 설정`으로 재현 가능해야 한다.
- Grafana 스크린샷과 함께 `Experiment` 모드 값을 반드시 기록한다.
- 추후 section5 문구에서는 `A/B branch`가 아니라 `A/B mode`로 설명하는 편이 더 정확하다.

---

## 4. Part A. 성능 검증 A/B

## 4.1 Hot Room Mix

### 4.1.1 시나리오 목적
- 이 시나리오는 `AOI를 왜 썼는가`를 증명하는 대표 실험이다.
- section2에서 설명한 `AOI`, `SpatialGrid`, `관심 영역 기반 브로드캐스트`, `가시성 캐시`, `spawn/despawn delta`, `배치 전송`이 실제 비용 차이를 만드는지 보여준다.
- 따라서 이 비교는 단순한 미세 최적화 비교가 아니라 `구조 선택의 근거`를 보여주는 핵심 페이지가 된다.

### 4.1.2 실험 질문
- 같은 혼합 플레이 입력에서 AOI가 없으면 송신 fan-out이 얼마나 증가하는가
- AOI가 없으면 JobQueue 대기와 worker busy ratio가 얼마나 악화되는가
- AOI가 없으면 packet handler 자체보다 `룸 전체 fan-out`이 먼저 병목이 되는가

### 4.1.3 A/B 정의

#### B안: 최종본
- `HotRoomAoiMode=final`
- `GameRoom.AOI.v2` 사용
- `ShouldUpdateAOI` 사용
- visible set 기반 diff update 사용
- spawn/despawn batching 사용
- `BroadcastToZone` 및 AOI visible set 기준 전파 사용

#### A안: baseline
- `HotRoomAoiMode=room_wide_baseline`
- AOI 비활성화
- 룸 전체를 단일 관심 영역처럼 취급
- 플레이어, 몬스터, 투사체, spawn/despawn, move, skill, hp 변경을 모두 룸 전체 기준으로 전송
- section5 문구에서는 `AOI off naive baseline`이라고 명시

### 4.1.4 A안이 제품 계약과 조금 다른 이유
- 최종본의 제품 계약은 `내 주변 객체만 본다`이다.
- baseline A안은 `같은 룸 객체를 모두 본다`로 가시성 규칙 자체가 달라진다.
- 즉, A안은 완전히 동일한 제품 모드가 아니라, `AOI가 없을 때 어떤 비용을 치르는지 보여주는 실험용 baseline`이다.
- 이 차이를 숨기면 안 되고, 오히려 `AOI를 왜 채택했는가`를 설명하는 근거로 써야 한다.

### 4.1.5 A안 구현 범위
이 baseline은 부분적으로 꺼서는 안 된다. 반드시 `전부` 룸 전체 기준으로 맞춘다.

#### 진입/맵 이동
- 파일:
  - `GameServer/GameRoom.EnterLeave.cpp`
  - `GameServer/GameRoom.MapChange.cpp`
- baseline 동작:
  - entering player에게 룸 전체 플레이어/몬스터/투사체 스냅샷 전송
  - 기존 플레이어 전원에게 entering player spawn 전송
  - 맵 이동 완료 후 `UpdateAOI` 대신 room-wide snapshot/broadcast 수행

#### 이동
- 파일:
  - `GameServer/GameRoom.Move.cpp`
- baseline 동작:
  - `UpdateAOI` 호출 생략
  - player-originated `S_MOVE`는 `Broadcast(moveSb, exceptId=playerId)` 사용

#### 전투
- 파일:
  - `GameServer/GameRoom.Combat.cpp`
  - `GameServer/GameRoom.Items.cpp`
  - `GameServer/Player.cpp`
- baseline 동작:
  - `BroadcastToZone` 대신 `Broadcast`
  - `S_SKILL`, `S_CHANGE_HP`, 아이템 사용으로 파생되는 이동/체력 갱신도 모두 room-wide

#### 퇴장
- 파일:
  - `GameServer/GameRoom.EnterLeave.cpp`
- baseline 동작:
  - `VisiblePlayers_ActorOnly()` 기준 despawn 대신 룸 전체 despawn

#### 몬스터/투사체
- 파일:
  - `GameServer/GameRoom.Monster.cpp`
  - `GameServer/GameRoom.Projectile.cpp`
- baseline 동작:
  - 플레이어에게 전달되는 몬스터/투사체 spawn/move/despawn도 room-wide로 맞춤

### 4.1.6 self echo 정책
- player-originated 패킷은 `exceptId=playerId`를 사용한다.
- 이유:
  - self echo를 포함하더라도 수신자 수 차이는 1명뿐이라 실험 결론에 큰 영향이 없다.
  - 반대로 self echo까지 강제로 넣으면 현재 클라이언트의 자기 자신 처리 의미가 불필요하게 바뀔 수 있다.
- 따라서 baseline은 `룸 전체 전송이되 자기 자신만 제외`로 고정한다.

### 4.1.7 이미 있는 핵심 지표
- `gs_packet_ingress_total`
- `gs_packet_handle_seconds`
- `gs_jobqueue_wait_seconds`
- `gs_jobqueue_exec_seconds`
- `gs_jobqueue_depth`
- `gs_globalqueue_depth`
- `gs_worker_idle_seconds_total`
- `gs_worker_exec_seconds_total`
- `gs_session_tx_bytes_total`
- `gs_packet_failure_total`

### 4.1.8 Hot Room Mix에서 반드시 추가할 지표

#### fan-out 직접 계측
- `gs_broadcast_recipients` histogram
- 라벨:
  - `kind=move|skill|hp|spawn|despawn`
  - `mode=aoi|room`
- 위치:
  - `GameServer/GameRoom.AOI.cpp`
  - `GameServer/GameRoom.AOI.v2.cpp`
  - `GameServer/GameRoom.Combat.cpp`
  - `GameServer/GameRoom.EnterLeave.cpp`

#### AOI 비용 계측
- `gs_aoi_update_seconds` histogram
- `gs_aoi_update_total` counter
- 위치:
  - `GameServer/GameRoom.AOI.v2.cpp`

#### AOI 후보/가시 객체 수
- `gs_aoi_candidates` histogram
- `gs_aoi_visible` histogram
- 라벨:
  - `kind=player|monster|projectile`
- 위치:
  - `GameServer/GameRoom.AOI.v2.cpp`

#### 송신 backlog 보호
- `gs_session_send_drop_total{reason="hard_cap|soft_cap"}`
- 위치:
  - `ServerCore/Session.cpp`

### 4.1.9 대표 차트
section5 page에서 가장 크게 보여줄 차트는 아래 3개다.

#### 대표 차트 1. Session TX Throughput
- 메시지:
  - `AOI off에서는 fan-out 증가가 곧바로 송신량 폭증으로 이어진다`
- PromQL:
```promql
sum(rate(gs_session_tx_bytes_total[$__rate_interval]))
```

#### 대표 차트 2. JobQueue Wait p95
- 메시지:
  - `송신량 증가는 결국 룸 직렬 처리 대기 증가로 이어진다`
- PromQL:
```promql
histogram_quantile(
  0.95,
  sum(rate(gs_jobqueue_wait_seconds_bucket{queue="job"}[$__rate_interval])) by (le)
)
```

#### 대표 차트 3. Broadcast Recipients p95
- 메시지:
  - `AOI가 줄여주는 비용의 본질은 recipient fan-out 축소다`
- PromQL:
```promql
histogram_quantile(
  0.95,
  sum(rate(gs_broadcast_recipients_bucket[$__rate_interval])) by (le, kind, mode)
)
```

### 4.1.10 서브 차트

#### 서브 차트 1. Packet Handle p95 by `move|skill`
```promql
histogram_quantile(
  0.95,
  sum(rate(gs_packet_handle_seconds_bucket{op=~"move|skill"}[$__rate_interval])) by (le, op)
)
```

#### 서브 차트 2. Worker Busy Ratio
```promql
sum(rate(gs_worker_exec_seconds_total{type="logic"}[$__rate_interval]))
/
clamp_min(
  sum(rate(gs_worker_exec_seconds_total{type="logic"}[$__rate_interval]))
  +
  sum(rate(gs_worker_idle_seconds_total{type="logic"}[$__rate_interval])),
  1e-9
)
```

#### 서브 차트 3. AOI Update p95
```promql
histogram_quantile(
  0.95,
  sum(rate(gs_aoi_update_seconds_bucket[$__rate_interval])) by (le)
)
```

#### 서브 차트 4. AOI Candidate vs Visible
```promql
histogram_quantile(
  0.95,
  sum(rate(gs_aoi_candidates_bucket[$__rate_interval])) by (le, kind)
)
```
```promql
histogram_quantile(
  0.95,
  sum(rate(gs_aoi_visible_bucket[$__rate_interval])) by (le, kind)
)
```

#### 서브 차트 5. Send Drop Rate
```promql
sum by (reason) (rate(gs_session_send_drop_total[$__rate_interval]))
```

### 4.1.11 해석 기준
- `TX throughput`과 `broadcast recipients`가 같이 오르면 fan-out이 직접 원인이다.
- `packet_handle`보다 `jobqueue_wait`가 더 가파르게 오르면 핫 룸 직렬 처리 적체가 본질이다.
- `AOI on`에서 `aoi_update_seconds`가 추가 비용으로 보이더라도, 전체 `TX`와 `queue wait`가 낮아지면 구조적으로 이득이다.
- `send_drop_total`이 A안에서만 관측되면 `AOI가 없을 때 보호 장치까지 밀어붙였다`는 강한 근거가 된다.

### 4.1.12 포트폴리오 결론 문장
- AOI는 단순 미세 최적화가 아니라, 혼합 플레이 부하에서 `recipient fan-out`, `네트워크 송신량`, `로직 큐 대기`를 동시에 줄이는 핵심 구조다.

---

## 4.2 Persistence Drain

### 4.2.1 시나리오 목적
- `Redis write-back + AutoCommit + DBAgent transaction` 구조가 왜 필요한지 보여주는 대표 저장 부하 시나리오다.
- section4에서 설명한 영속화 구조를 실제 비용과 병목 관점에서 입증한다.

### 4.2.2 실험 질문
- 같은 저장 의미를 유지할 때 즉시 저장보다 write-back이 얼마나 DB 경로 부하를 줄이는가
- DB query, pool wait, S2S RTT가 어떤 순서로 악화되는가
- write-back이 없으면 DBAgent 커넥션 풀이 먼저 병목이 되는가

### 4.2.3 저장 부하 모델
- 더미 시나리오: `scenario=persistence`
- 이동/전투는 끄고 QuickSlot mutation만 발생시킨다
- QuickSlot 패턴은 `2.2`에서 정의한 고정 패턴을 그대로 사용한다
- 목적:
  - GameServer 내부 저장 경로와 DBAgent/DB 병목만 분리해서 보기 위함

### 4.2.4 A/B 정의

#### B안: 최종본
- `PersistenceMode=writeback`
- `UpdateQuickSlot -> markDirty -> AutoCommitService flush`
- DBAgent save는 배치/주기/flushNow에 따라 수행

#### A안: baseline
- `PersistenceMode=immediate_quickslot`
- QuickSlot 변경이 발생할 때마다 즉시 `S2S_REQ_SAVE_QUICKSLOT` 전송
- QuickSlot 도메인에서는 AutoCommit을 우회

### 4.2.5 A안 구현 방식

#### 실험 모드 토글
- `PersistenceMode == immediate_quickslot`

#### QuickSlot 변경 처리
- 파일:
  - `GameServer/ClientPacketHandler.Misc.cpp`
  - `GameServer/PersistenceService.cpp`

#### 권장 흐름
1. QuickSlot 변경 요청 수신
2. Redis 반영은 그대로 유지
3. baseline 모드에서는 `markDirty=false`
4. 즉시 `BuildSnapshot_QuickSlot(pid, req)`
5. 즉시 `G_DBSession->Send(...)`
6. `S2S_RES_SAVE_QUICKSLOT` 응답 경로는 기존 로직 재사용

#### 중요한 주의사항
- baseline A안에서 QuickSlot을 `markDirty=true`로 두면 AutoCommit과 즉시 저장이 중복된다.
- baseline A안에서는 QuickSlot 도메인만 명시적으로 AutoCommit 대상에서 제외해야 한다.
- 1차 실험에서는 Core/Inventory를 immediate save로 바꾸지 않는다.

### 4.2.6 이미 있는 핵심 지표
- `gs_s2s_rtt_seconds{op="save_quickslot"}`
- `db_req_handle_seconds`
- `db_query_seconds`
- `db_pool_wait_seconds`
- `db_pool_size`
- `db_pool_inuse`

### 4.2.7 Persistence Drain에서 반드시 추가할 지표

#### AutoCommit tick 비용
- `gs_autocommit_tick_seconds` histogram
- 위치:
  - `GameServer/AutoCommitService.cpp`

#### AutoCommit target/inflight
- `gs_autocommit_targets`
- `gs_autocommit_inflight`
- `gs_autocommit_sent_total{domain="core|inv|qs"}`
- 위치:
  - `GameServer/AutoCommitService.cpp`

#### skip reason
- `gs_autocommit_skip_total{reason="inflight|empty_snapshot|redis_missing"}`
- 위치:
  - `GameServer/AutoCommitService.cpp`

#### dirty backlog
- `gs_dirty_set_size{domain="player|inv|qs"}`
- 위치:
  - `GameServer/PersistenceService.cpp`
  - 또는 `GameServer/AutoCommitService.cpp`

#### mutation source counter
- `gs_persistence_mutation_total{domain="qs", path="writeback|immediate"}`
- 위치:
  - `GameServer/PersistenceService.cpp`
  - `GameServer/ClientPacketHandler.Misc.cpp`

### 4.2.8 더미클라이언트 구현 요구사항
- 파일:
  - `DummyClient/LoadClientManager.h`
  - `DummyClient/LoadClientManager.cpp`
  - `DummyClient/LoadClientConfig.h`
  - `DummyClient/LoadClientConfig.json`
- 추가 항목:
  - `LoadScenario::Persistence`
  - `quickslotHz`
  - `SendQuickSlot(uint64 nowMs)`
- 주의:
  - section5에서는 DummyClient CSV를 쓰지 않으므로, 클라이언트 RTT 통계 컬럼은 필수가 아니다
  - 더미는 부하 생성기 역할만 충실하면 된다

### 4.2.9 대표 차트
section5 page에서 가장 크게 보여줄 차트는 아래 3개다.

#### 대표 차트 1. DB Pool Wait p95
- 메시지:
  - `즉시 저장은 가장 먼저 커넥션 풀 대기를 악화시킨다`
- PromQL:
```promql
histogram_quantile(
  0.95,
  sum(rate(db_pool_wait_seconds_bucket{op="save_quickslot"}[$__rate_interval])) by (le)
)
```

#### 대표 차트 2. Game -> DBAgent S2S RTT p95
- 메시지:
  - `write-back이 없으면 게임 서버 입장에서 저장 RTT가 급격히 커진다`
- PromQL:
```promql
histogram_quantile(
  0.95,
  sum(rate(gs_s2s_rtt_seconds_bucket{op="save_quickslot"}[$__rate_interval])) by (le)
)
```

#### 대표 차트 3. DB Query p95
- 메시지:
  - `즉시 저장은 실제 DB 실행 비용까지 밀어올린다`
- PromQL:
```promql
histogram_quantile(
  0.95,
  sum(rate(db_query_seconds_bucket{op="save_quickslot"}[$__rate_interval])) by (le)
)
```

### 4.2.10 서브 차트

#### 서브 차트 1. Pool InUse Ratio
```promql
db_pool_inuse / clamp_min(db_pool_size, 1)
```

#### 서브 차트 2. AutoCommit Tick p95
```promql
histogram_quantile(
  0.95,
  sum(rate(gs_autocommit_tick_seconds_bucket[$__rate_interval])) by (le)
)
```

#### 서브 차트 3. AutoCommit Inflight
```promql
max(gs_autocommit_inflight)
```

#### 서브 차트 4. Dirty Set Size
```promql
max by (domain) (gs_dirty_set_size)
```

#### 서브 차트 5. Mutation Throughput
```promql
sum by (path) (rate(gs_persistence_mutation_total{domain="qs"}[$__rate_interval]))
```

#### 서브 차트 6. AutoCommit Skip Reasons
```promql
sum by (reason) (rate(gs_autocommit_skip_total[$__rate_interval]))
```

### 4.2.11 해석 기준
- `db_pool_wait`가 먼저 오르면 커넥션 풀이 가장 먼저 포화된 것이다.
- `db_query`도 같이 오르면 실제 DB 실행 구간까지 병목이 확장된 것이다.
- `gs_s2s_rtt`만 높고 `db_query`는 낮으면 GameServer flush scheduling 또는 DBAgent queueing 쪽 문제다.
- `dirty_set_size`가 계속 증가하면 flush throughput이 mutation rate를 따라가지 못한 것이다.

### 4.2.12 포트폴리오 결론 문장
- write-back은 단순 캐시 기법이 아니라, 고빈도 저장 이벤트를 DB 경로에서 흡수해 `S2S RTT`, `pool wait`, `query`를 안정화하는 핵심 구조다.

---

## 5. Part B. 운영 안정성 검증

## 5.1 Enter Burst

### 5.1.1 시나리오 목적
- 대량의 로그인/입장 요청이 짧은 시간에 몰릴 때 입장 경로가 올바르게 수렴하는지 본다.
- 핵심은 `빠른 입장`이 아니라 `잘못된 입장 방지`, `정확한 바인딩`, `부분 로드 실패 처리`다.

### 5.1.2 실험 질문
- burst 상황에서 `ccu -> ingame_players`가 안정적으로 수렴하는가
- Lobby 게이트가 반쯤 로드된 플레이어를 world에 넣지 않는가
- DB 로드 실패 시 fail-fast 후 세션 정리가 정상적으로 되는가

### 5.1.3 서버 계측만 사용한다는 점
- 이 시나리오는 DummyClient CSV를 사용하지 않는다.
- 로그인/입장 관련 판단도 전부 `LoginServer`, `GameServer`, `DBAgent` 메트릭으로 설명한다.
- 따라서 section5 공개 자료에는 클라이언트 RTT가 아니라 서버 내부 admission path 지표만 사용한다.

### 5.1.4 이미 있는 핵심 지표
- `gs_lobby_wait_seconds`
- `gs_s2s_rtt_seconds{op="load_player_data|items_load|quickslot_load"}`
- `gs_ccu`
- `gs_ingame_players`

### 5.1.5 Enter Burst에서 반드시 추가할 지표

#### LoginServer 계측
- `ls_login_req_total`
- `ls_login_req_handle_seconds`
- `ls_login_req_failure_total{reason}`
- `ls_token_issue_total`
- `ls_token_issue_failure_total`
- 위치:
  - `LoginServer/ClientPacketHandler.cpp`
  - `LoginServer/S2SPacketHandler.*`
  - 필요 시 Login 전용 metrics 모듈 추가

#### GameServer 입장 계측
- `gs_enter_request_total`
- `gs_enter_success_total`
- `gs_enter_fail_total{reason="invalid_token|db_load_failed|invalid_map|other"}`
- `gs_lobby_pending_players`
- `gs_enter_admission_seconds{result="success|fail"}`
- 위치:
  - `GameServer/ClientPacketHandler.EnterGame.cpp`
  - `GameServer/LobbyRoom.cpp`

### 5.1.6 대표 차트
section5 page에서 가장 크게 보여줄 차트는 아래 3개다.

#### 대표 차트 1. CCU vs Ingame
- 메시지:
  - `burst가 와도 월드 진입 수렴이 무너지지 않는다`
- PromQL:
```promql
gs_ccu
```
```promql
gs_ingame_players
```

#### 대표 차트 2. Lobby Wait p95
- 메시지:
  - `입장 게이트 내부 대기가 어느 수준에서 수렴하는지 보여준다`
- PromQL:
```promql
histogram_quantile(
  0.95,
  sum(rate(gs_lobby_wait_seconds_bucket{type="enter_game"}[$__rate_interval])) by (le)
)
```

#### 대표 차트 3. Enter Failure by Reason
- 메시지:
  - `실패가 있더라도 어떤 보호 장치가 발동했는지 설명 가능해야 한다`
- PromQL:
```promql
sum by (reason) (rate(gs_enter_fail_total[$__rate_interval]))
```

### 5.1.7 서브 차트

#### 서브 차트 1. Enter Admission p95
```promql
histogram_quantile(
  0.95,
  sum(rate(gs_enter_admission_seconds_bucket[$__rate_interval])) by (le, result)
)
```

#### 서브 차트 2. Load S2S RTT p95
```promql
histogram_quantile(
  0.95,
  sum(rate(gs_s2s_rtt_seconds_bucket{op=~"load_player_data|items_load|quickslot_load"}[$__rate_interval])) by (le, op)
)
```

#### 서브 차트 3. Login Request Handle p95
```promql
histogram_quantile(
  0.95,
  sum(rate(ls_login_req_handle_seconds_bucket[$__rate_interval])) by (le)
)
```

#### 서브 차트 4. Lobby Pending Players
```promql
max(gs_lobby_pending_players)
```

#### 서브 차트 5. Login Failure by Reason
```promql
sum by (reason) (rate(ls_login_req_failure_total[$__rate_interval]))
```

### 5.1.8 해석 기준
- `gs_ccu`는 올랐는데 `gs_ingame_players`가 뒤따르지 못하면 입장 병목이다.
- `gs_lobby_pending_players`가 오래 남으면 DB/S2S/Lobby 중 한 곳이 막힌 것이다.
- `gs_enter_fail_total{reason!="invalid_token"}`가 관측되면 기능 회귀 가능성을 우선 의심한다.
- `gs_enter_admission_seconds`가 높더라도 fail count가 0이고 최종 수렴이 되면 `느린 것`과 `잘못된 것`을 구분해서 설명할 수 있다.

### 5.1.9 포트폴리오 결론 문장
- burst 상황에서도 인증, 로비, DB 로드 게이트가 정합성을 유지하며 반쯤 로드된 진입 없이 정상 수렴한다.

---

## 5.2 Idle Soak

### 5.2.1 시나리오 목적
- 장시간 연결 유지 상황에서 메모리, CPU, 세션 상태가 드리프트 없이 안정적인지 본다.
- 최대 처리량보다 `오래 돌려도 이상하지 않은가`를 증명하는 시나리오다.

### 5.2.2 이미 있는 핵심 지표
- `gs_process_resident_memory_bytes`
- `gs_process_cpu_seconds_total`
- `gs_ccu`
- `gs_ingame_players`
- `gs_session_rx_bytes_total`
- `gs_session_tx_bytes_total`
- `gs_metrics_response_seconds`

### 5.2.3 Idle Soak에서 반드시 추가할 지표
- `gs_session_disconnect_total`
- `gs_session_disconnect_total{reason}` 가능하면 reason 분해
- `gs_session_send_drop_total`
- 위치:
  - `ServerCore/Session.cpp`

### 5.2.4 대표 차트
section5 page에서 가장 크게 보여줄 차트는 아래 3개다.

#### 대표 차트 1. Resident Memory
- 메시지:
  - `warm-up 이후 메모리가 계속 우상향하지 않아야 한다`
- PromQL:
```promql
gs_process_resident_memory_bytes
```

#### 대표 차트 2. CPU Usage Rate
- 메시지:
  - `idle 유지 비용이 시간에 따라 비정상적으로 커지지 않아야 한다`
- PromQL:
```promql
sum(rate(gs_process_cpu_seconds_total[$__rate_interval])) by (mode)
```

#### 대표 차트 3. CCU vs Ingame
- 메시지:
  - `장시간 유지 중에도 세션과 인게임 인원이 자연 감소하지 않아야 한다`
- PromQL:
```promql
gs_ccu
```
```promql
gs_ingame_players
```

### 5.2.5 서브 차트

#### 서브 차트 1. Metrics Response p95
```promql
histogram_quantile(
  0.95,
  sum(rate(gs_metrics_response_seconds_bucket[$__rate_interval])) by (le)
)
```

#### 서브 차트 2. Session Disconnect Rate
```promql
sum by (reason) (rate(gs_session_disconnect_total[$__rate_interval]))
```

#### 서브 차트 3. Session Send Drop Rate
```promql
sum by (reason) (rate(gs_session_send_drop_total[$__rate_interval]))
```

#### 서브 차트 4. Session RX/TX Throughput
```promql
rate(gs_session_rx_bytes_total[$__rate_interval])
```
```promql
rate(gs_session_tx_bytes_total[$__rate_interval])
```

### 5.2.6 해석 기준
- `resident memory`가 warm-up 이후 계속 우상향하면 누수 또는 정리 미흡 가능성이 크다.
- CPU rate가 시간이 갈수록 상승하면 background task나 reconnect/drift를 의심한다.
- `CCU`와 `Ingame`이 서서히 떨어지면 세션 안정성 문제다.
- `/metrics` 응답 시간이 함께 나빠지면 전체 프로세스 상태가 이미 흔들리고 있다는 뜻이다.

### 5.2.7 포트폴리오 결론 문장
- 장시간 연결 유지 상황에서도 메모리 누수, CPU 드리프트, 세션 누적 손실 없이 안정적으로 운영된다.

---

## 6. Prometheus / Grafana 자산 계획

## 6.1 기존 자산
- `docs/monitoring/prometheus.yml`
- `docs/monitoring/gameserver_dashboard_queries.md`
- `docs/monitoring/dbagent_dashboard_queries.md`
- `docs/monitoring/grafana_gameserver_dashboard.json`
- `docs/monitoring/grafana_dbagent_dashboard.json`

## 6.2 반드시 추가할 자산
- `LoginServer` scrape target
- `LoginServer` 패널 또는 통합 대시보드 패널
- `Hot Room Mix` 전용 AOI/fan-out 패널
- `Persistence Drain` 전용 AutoCommit/dirty backlog 패널
- `Enter Burst` 전용 admission 패널
- `Idle Soak` 전용 soak 패널

## 6.3 section5에서 대표로 보여줄 차트 묶음

### Part A. 성능 검증 A/B
- `Hot Room Mix`
  - 대표:
    - `Session TX Throughput`
    - `JobQueue Wait p95`
    - `Broadcast Recipients p95`
  - 서브:
    - `Worker Busy Ratio`
    - `Packet Handle p95`
    - `AOI Update p95`
    - `Send Drop Rate`
- `Persistence Drain`
  - 대표:
    - `DB Pool Wait p95`
    - `S2S RTT p95(save_quickslot)`
    - `DB Query p95(save_quickslot)`
  - 서브:
    - `Pool InUse Ratio`
    - `AutoCommit Tick p95`
    - `AutoCommit Inflight`
    - `Dirty Set Size`
    - `Mutation Throughput`

### Part B. 운영 안정성 검증
- `Enter Burst`
  - 대표:
    - `CCU vs Ingame`
    - `Lobby Wait p95`
    - `Enter Failure by Reason`
  - 서브:
    - `Enter Admission p95`
    - `Load S2S RTT p95`
    - `Login Request Handle p95`
    - `Lobby Pending Players`
- `Idle Soak`
  - 대표:
    - `Resident Memory`
    - `CPU Usage Rate`
    - `CCU vs Ingame`
  - 서브:
    - `Metrics Response p95`
    - `Session Disconnect Rate`
    - `Session Send Drop Rate`
    - `Session RX/TX Throughput`

## 6.4 section5에서 사용하지 않는 것
- DummyClient CSV
- DummyClient 콘솔 통계
- 수동 로그 캡처를 주 근거로 삼는 방식

---

## 7. 시나리오별 우선 구현 작업

## 7.1 P0
- `ServerConfig`에 `Experiment` 섹션 추가
- `Metrics.Prefix`를 `gs/db/ls`로 표준화
- `docs/monitoring/prometheus.yml`에 `LoginServer` scrape target 추가
- `HotRoomAoiMode=room_wide_baseline` 구현
- `PersistenceMode=immediate_quickslot` 구현
- DummyClient에 `Persistence` 시나리오와 `quickslot_hz` 추가
- `gs_broadcast_recipients`, `gs_aoi_update_seconds`, `gs_session_send_drop_total` 추가
- `gs_autocommit_*`, `gs_dirty_set_size`, `gs_persistence_mutation_total` 추가
- `ls_login_*`, `ls_token_issue_*`, `gs_enter_*`, `gs_lobby_pending_players`, `gs_enter_admission_seconds` 추가
- `gs_session_disconnect_total` 추가

## 7.2 P1
- `Hot Room Mix`용 Grafana 패널 추가
- `Persistence Drain`용 Grafana 패널 추가
- `Enter Burst`용 Grafana 패널 추가
- `Idle Soak`용 Grafana 패널 추가
- 쿼리 문서를 대표/서브 차트 기준으로 정리

## 7.3 P2
- section5 페이지용 캡션 문구 템플릿 작성
- 반복 실험 결과를 정리하는 비교표 템플릿 작성
- Grafana 패널 export JSON 정리

---

## 8. 다른 AI가 이 문서를 받고 바로 해야 할 일

1. `ServerConfig`에 `Experiment` 섹션과 `Metrics` 섹션 확장
2. `GameServer`, `DBAgent`, `LoginServer`에 `Metrics.Prefix` 적용
3. `HotRoomAoiMode=room_wide_baseline` 구현
4. `PersistenceMode=immediate_quickslot` 구현
5. DummyClient에 `Persistence` 시나리오 추가
6. 문서에 적힌 신규 메트릭을 모두 추가
7. `prometheus.yml`과 Grafana 자산 업데이트
8. 문서의 고정 시나리오 프로필대로 실험 설정 JSON 작성
9. 대표 차트와 서브 차트를 기준으로 section5 산출물 제작

---

## 9. 최종 메시지
- section5는 단순히 `서버가 빨랐다`를 보여주는 장이 아니다.
- 이 프로젝트가 왜 `AOI`, `JobQueue`, `Write-Back`, `AutoCommit`, `입장 게이트`, `운영 메트릭`을 채택했는지 실험으로 증명하는 근거 파트다.
- 따라서 이 문서의 모든 시나리오는 `최대 CCU 자랑`보다 `구조 선택의 타당성 증명`을 우선한다.
