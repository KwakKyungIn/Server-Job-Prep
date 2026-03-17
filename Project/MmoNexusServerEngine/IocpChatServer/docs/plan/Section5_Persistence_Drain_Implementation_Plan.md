# Section5 Persistence Drain 구현 계획서

## 문서 목적
- 본 문서는 기존 [Section5_Performance_Test_Plan.md](/Users/GGI/Desktop/Github_job/Server-Job-Prep/Project/MmoNexusServerEngine/IocpChatServer/docs/plan/Section5_Performance_Test_Plan.md)의 `Persistence Drain` 구간을 실제 코드 구조에 맞춰 구현 가능한 수준으로 구체화한 보완 계획서다.
- 목표는 단순히 `QuickSlot 저장 부하를 걸어본다`가 아니라, `writeback(B)`과 `immediate_quickslot(A)`의 차이를 Prometheus + Grafana로 설명 가능한 형태로 재현하는 것이다.
- 이 문서는 다음을 한 번에 다룬다.
  - 현재 코드 기준 준비 상태
  - 반드시 추가해야 할 설정/코드/메트릭
  - 테스트를 바로 돌리기 위한 실행 순서
  - 결과 해석 기준
  - 구현 시 주의할 리스크

---

## 1. 테스트 목표 재정의

### 1.1 Persistence Drain에서 검증하려는 것
- `QuickSlot` 변경이 반복적으로 발생할 때, `GameServer -> DBAgent -> DB` 저장 경로가 어떤 압력 분포를 만드는지 본다.
- 같은 workload에서 저장 전략만 바꿨을 때 다음이 어떻게 달라지는지 비교한다.
  - DB connection pool wait
  - GameServer -> DBAgent S2S RTT
  - DB query latency
  - AutoCommit backlog와 inflight 상태
  - QuickSlot dirty set 증감 추이

### 1.2 A/B 정의
- `B안 (final)`:
  - `PersistenceMode=writeback`
  - `UpdateQuickSlot -> markDirty -> AutoCommitService 주기 flush`
- `A안 (baseline)`:
  - `PersistenceMode=immediate_quickslot`
  - `QuickSlot 변경마다 즉시 S2S_REQ_SAVE_QUICKSLOT 전송`
  - QuickSlot 도메인은 AutoCommit 경로를 타지 않음

### 1.3 핵심 비교 질문
- 같은 `quickslot_hz`에서 어느 쪽이 DB pool wait를 더 줄이는가
- 어느 쪽이 S2S RTT를 더 안정적으로 유지하는가
- writeback이 inflight와 dirty backlog를 감당 가능한 수준으로 유지하는가
- immediate baseline이 어느 구간에서 pool/RTT를 급격히 악화시키는가

---

## 2. 현재 코드 기준 준비 상태

### 2.1 이미 구현되어 있는 것
- QuickSlot 변경 패킷 처리
  - [ClientPacketHandler.Misc.cpp](/Users/GGI/Desktop/Github_job/Server-Job-Prep/Project/MmoNexusServerEngine/IocpChatServer/GameServer/ClientPacketHandler.Misc.cpp)
- Redis 기반 QuickSlot 반영 / dirty 처리 / snapshot 생성
  - [PersistenceService.cpp](/Users/GGI/Desktop/Github_job/Server-Job-Prep/Project/MmoNexusServerEngine/IocpChatServer/GameServer/PersistenceService.cpp)
- AutoCommit worker 구조
  - [AutoCommitService.cpp](/Users/GGI/Desktop/Github_job/Server-Job-Prep/Project/MmoNexusServerEngine/IocpChatServer/GameServer/AutoCommitService.cpp)
- GameServer -> DBAgent `save_quickslot` RTT 추적
  - [GameMetrics.cpp](/Users/GGI/Desktop/Github_job/Server-Job-Prep/Project/MmoNexusServerEngine/IocpChatServer/GameServer/GameMetrics.cpp)
- DBAgent 저장 처리 및 DB 계측
  - [DBAgentPacketHandler.cpp](/Users/GGI/Desktop/Github_job/Server-Job-Prep/Project/MmoNexusServerEngine/IocpChatServer/DBAgent/DBAgentPacketHandler.cpp)
  - [DBAgentMetrics.cpp](/Users/GGI/Desktop/Github_job/Server-Job-Prep/Project/MmoNexusServerEngine/IocpChatServer/DBAgent/DBAgentMetrics.cpp)

### 2.2 현재 바로는 안 되는 것
- `PersistenceMode` 실험 토글이 없다.
- `immediate_quickslot` baseline 경로가 없다.
- DummyClient에 `scenario=persistence`가 없다.
- DummyClient 설정에 `quickslot_hz`가 없다.
- 계획서에 적힌 AutoCommit/dirty 관련 메트릭이 없다.
- AutoCommit 주기가 `120초` 하드코딩이라, `hold_sec=180` 테스트에서 steady-state 해석이 애매하다.

### 2.3 현재 상태에서의 해석상 문제
- 지금 상태로는 모든 QuickSlot 변경이 사실상 `writeback`으로만 간다.
- 따라서 A/B 비교를 하더라도 실험 모드 차이가 없다.
- AutoCommit 주기가 너무 길어서, 부하가 steady-state에서 분산되는지 아니면 마지막에 몰리는지 구분이 어렵다.
- disconnect 시 `RequestFlushNow`가 들어가므로 테스트 종료 시점의 저장 burst가 결과를 오염시킬 수 있다.

---

## 3. 구현 범위

### 3.1 이번 Persistence Drain 준비 작업의 범위
- `Experiment` 설정에 Persistence Drain용 모드 추가
- QuickSlot writeback / immediate baseline 분기 추가
- AutoCommit 관측 메트릭 추가
- Dirty set / mutation 계측 추가
- DummyClient persistence 시나리오 추가
- 테스트용 설정 JSON 추가 또는 보완
- Grafana용 대표/서브 패널에 필요한 메트릭 기반 완성

### 3.2 이번 단계에서 굳이 하지 않아도 되는 것
- Inventory/Core 도메인의 immediate baseline 확장
- 월드 이동/전투/맵 전환과 persistence를 섞은 복합 시나리오
- 운영 수준 장시간 soak 자동화
- 결과 보고서용 이미지 정리

---

## 4. 설정 설계

### 4.1 Experiment 설정 확장안
- 위치
  - [CoreGlobal.h](/Users/GGI/Desktop/Github_job/Server-Job-Prep/Project/MmoNexusServerEngine/IocpChatServer/ServerCore/CoreGlobal.h)
  - [CoreGlobal.cpp](/Users/GGI/Desktop/Github_job/Server-Job-Prep/Project/MmoNexusServerEngine/IocpChatServer/ServerCore/CoreGlobal.cpp)

### 4.2 추가할 필드
- `PersistenceMode`
  - `writeback`
  - `immediate_quickslot`
- `AutoCommitIntervalSec`
  - 권장 기본값: `60`
  - 테스트용 허용 범위: `10 ~ 300`

### 4.3 권장 JSON 예시
```json
{
  "Experiment": {
    "Enabled": true,
    "PersistenceMode": "writeback",
    "AutoCommitIntervalSec": 60
  }
}
```

### 4.4 모드 의미
- `writeback`
  - 기존 서버 구조 유지
  - QuickSlot 변경은 Redis + dirty set 적재
  - AutoCommit이 주기적으로 snapshot 생성 후 DBAgent 저장
- `immediate_quickslot`
  - QuickSlot 변경은 Redis 상태 갱신 후 즉시 `S2S_REQ_SAVE_QUICKSLOT` 전송
  - QuickSlot dirty set에는 넣지 않음
  - QuickSlot은 AutoCommit 대상에서 제외

### 4.5 AutoCommitIntervalSec를 꼭 넣는 이유
- 현재 [AutoCommitService.cpp](/Users/GGI/Desktop/Github_job/Server-Job-Prep/Project/MmoNexusServerEngine/IocpChatServer/GameServer/AutoCommitService.cpp)의 주기는 `120초` 고정이다.
- 계획서 기본 `hold_sec=180`으로는 steady-state 동안 periodic flush가 사실상 1회만 보일 가능성이 높다.
- 이 상태로는 `writeback`이 좋은지 나쁜지보다, `언제 flush가 발생했는지`가 결과를 좌우한다.
- 따라서 Persistence Drain 전용 실험에서는 interval을 설정화하는 것이 사실상 필수다.

---

## 5. 서버 로직 변경 계획

### 5.1 QuickSlot 처리 분기
- 수정 파일
  - [ClientPacketHandler.Misc.cpp](/Users/GGI/Desktop/Github_job/Server-Job-Prep/Project/MmoNexusServerEngine/IocpChatServer/GameServer/ClientPacketHandler.Misc.cpp)

### 5.2 `writeback` 모드 동작
- 현재 로직 유지
- `UpdateQuickSlot(..., markDirty=true)`
- 이후 AutoCommit이 snapshot 생성 후 저장 전송

### 5.3 `immediate_quickslot` 모드 동작
- Redis 상 최종 QuickSlot 상태는 동일하게 반영
- 단, `markDirty=false`
- 같은 요청 안에서 중복 아이템 clear가 먼저 발생하더라도, 저장 전송은 `최종 상태 기준 1회`만 보내야 한다.
- 권장 순서:
  1. 필요 시 중복 슬롯 clear
  2. 요청 슬롯에 최종 값 반영
  3. `BuildSnapshot_QuickSlot` 호출
  4. `S2S_REQ_SAVE_QUICKSLOT` 즉시 전송
  5. 클라이언트에 성공 응답 전송

### 5.4 immediate 모드에서 주의할 점
- 현재 handler는 한 요청 안에서 여러 `UpdateQuickSlot` 호출이 가능하다.
- baseline 구현을 잘못하면 `clear용 save + 최종 save`처럼 DBAgent 호출이 2번 나갈 수 있다.
- 이 테스트의 baseline은 `QuickSlot 변경 이벤트당 즉시 저장 1회`가 되어야지, 내부 중간 상태까지 DB에 흘리는 구현이 되어서는 안 된다.

### 5.5 AutoCommit 서비스 변경
- 수정 파일
  - [AutoCommitService.cpp](/Users/GGI/Desktop/Github_job/Server-Job-Prep/Project/MmoNexusServerEngine/IocpChatServer/GameServer/AutoCommitService.cpp)

### 5.6 변경 사항
- `AutoCommitIntervalSec` 설정 반영
- tick 시작/종료 시간 측정
- target 수, inflight 수, sent 수, skip 수 계측
- `PersistenceMode=immediate_quickslot`일 때 QuickSlot dirty set scan 생략
- QuickSlot snapshot send 생략

### 5.7 inflight 처리 주의
- 현재 inflight는 `pid -> outstanding packet count` 구조다.
- baseline immediate 모드에서는 QuickSlot 저장이 더 자주 발생하므로, 다음 사항을 명확히 해야 한다.
  - immediate save도 inflight 제어 대상에 넣을지
  - 넣는다면 AutoCommit과 같은 카운터를 재사용할지
  - 아니면 QuickSlot immediate는 별도 inflight로 분리할지

### 5.8 권장 방향
- 1차 구현은 단순하게 간다.
- `AutoCommit inflight`는 AutoCommit이 보낸 요청만 집계한다.
- immediate baseline의 즉시 저장은 `AutoCommit inflight`에 섞지 않고, RTT/DB pool/query로 영향도를 본다.
- 이렇게 해야 writeback 고유 비용과 immediate 고유 비용을 계측상 분리하기 쉽다.

---

## 6. PersistenceService 변경 계획

### 6.1 수정 파일
- [PersistenceService.cpp](/Users/GGI/Desktop/Github_job/Server-Job-Prep/Project/MmoNexusServerEngine/IocpChatServer/GameServer/PersistenceService.cpp)

### 6.2 추가 목적
- QuickSlot mutation 자체의 throughput 측정
- dirty set backlog 추적
- snapshot build 실패 원인 분기 보강

### 6.3 추가할 관측 포인트
- `UpdateQuickSlot` 진입 시
  - `gs_persistence_mutation_total{domain="qs", path="writeback|immediate"}`
- dirty mark 직후 또는 주기적 tick 시
  - `gs_dirty_set_size{domain="player|inv|qs"}`

### 6.4 dirty set size 수집 방식
- 이상적 방법
  - `RedisManager::SCard()` 추가
- 임시 방법
  - `SMembers().size()` 사용
- 권장
  - 이번 작업에서는 `SCard`를 추가하는 것이 맞다.
  - 이유:
    - dirty set size는 관측 빈도가 높아질 수 있다.
    - `SMembers`는 값 전체를 가져오므로 테스트 규모가 커질수록 오버헤드가 커진다.

---

## 7. 추가 메트릭 설계

### 7.1 새로 추가해야 할 GameServer 메트릭

| 이름 | 타입 | 라벨 | 의미 | 관측 위치 |
|---|---|---|---|---|
| `gs_autocommit_tick_seconds` | histogram | 없음 | AutoCommit 한 번 도는 데 걸린 시간 | `AutoCommitService::TickCommit_Internal` |
| `gs_autocommit_targets` | histogram 또는 gauge | 없음 | 한 tick에서 수집된 대상 수 | `AutoCommitService::TickCommit_Internal` |
| `gs_autocommit_inflight` | gauge | 없음 | AutoCommit 기준 저장 대기 중 pid 수 | `AutoCommitService` |
| `gs_autocommit_sent_total` | counter | `domain` | tick에서 실제 보낸 저장 요청 수 | core/inv/qs 전송 직전 |
| `gs_autocommit_skip_total` | counter | `reason` | inflight, empty snapshot, redis missing 등으로 skip된 횟수 | tick 내부 분기 |
| `gs_dirty_set_size` | gauge | `domain` | player/inv/qs dirty set 크기 | Redis set size 측정 시 |
| `gs_persistence_mutation_total` | counter | `domain`, `path` | QuickSlot mutation throughput | `UpdateQuickSlot` |

### 7.2 기존 메트릭 중 그대로 쓰는 것

| 이름 | 타입 | 라벨 | 역할 |
|---|---|---|---|
| `gs_s2s_rtt_seconds` | histogram | `op` | GameServer -> DBAgent RTT |
| `db_req_handle_seconds` | histogram | `op` | DBAgent request handling 시간 |
| `db_query_seconds` | histogram | `op` | 실제 DB query 시간 |
| `db_pool_wait_seconds` | histogram | `op` | DB connection pool 대기 시간 |
| `db_pool_size` | gauge | 없음 | pool 크기 |
| `db_pool_inuse` | gauge | 없음 | 사용 중 connection 수 |

### 7.3 skip reason 표준화 권장안
- `inflight`
- `empty_snapshot`
- `redis_missing`
- `send_handler_missing`
- `mode_excluded`

### 7.4 계측상 주의점
- `empty_snapshot`과 `redis_missing`을 정확히 나누려면 snapshot build 함수가 단순 `bool`만 반환하면 부족할 수 있다.
- 1차 구현은 enum 결과를 돌려주는 보조 함수로 확장하는 것을 권장한다.

---

## 8. DummyClient 변경 계획

### 8.1 수정 파일
- [LoadClientManager.h](/Users/GGI/Desktop/Github_job/Server-Job-Prep/Project/MmoNexusServerEngine/IocpChatServer/DummyClient/LoadClientManager.h)
- [LoadClientManager.cpp](/Users/GGI/Desktop/Github_job/Server-Job-Prep/Project/MmoNexusServerEngine/IocpChatServer/DummyClient/LoadClientManager.cpp)
- [LoadClientConfig.h](/Users/GGI/Desktop/Github_job/Server-Job-Prep/Project/MmoNexusServerEngine/IocpChatServer/DummyClient/LoadClientConfig.h)
- [LoadClientConfig.cpp](/Users/GGI/Desktop/Github_job/Server-Job-Prep/Project/MmoNexusServerEngine/IocpChatServer/DummyClient/LoadClientConfig.cpp)
- 실행용 JSON
  - [Binary/Release/LoadClientConfig.json](/Users/GGI/Desktop/Github_job/Server-Job-Prep/Project/MmoNexusServerEngine/IocpChatServer/Binary/Release/LoadClientConfig.json)
  - [Binary/Debug/LoadClientConfig.json](/Users/GGI/Desktop/Github_job/Server-Job-Prep/Project/MmoNexusServerEngine/IocpChatServer/Binary/Debug/LoadClientConfig.json)

### 8.2 추가할 시나리오
- `LoadScenario::Persistence`

### 8.3 추가할 설정값
- `quickslot_hz`

### 8.4 persistence 시나리오 동작
- heartbeat는 유지
- move는 보내지 않음
- skill도 보내지 않음
- quickslot만 주기적으로 변경

### 8.5 QuickSlot 패턴 고정안
- slot 순서: `0 -> 1 -> 2 -> 3 -> 0 -> ...`
- ref type: `QS_SKILL`
- skillId 순환: `1 -> 2 -> 3 -> 1 -> ...`
- 매 4번째 이벤트는 `QS_NONE` clear

### 8.6 권장 구현 방식
- DummyClient 내부에 다음 상태를 둔다.
  - 현재 slot cursor
  - 현재 skill cursor
  - 현재 event count
- `quickslot_hz` 기준 시각이 되면
  - `eventCount % 4 == 3`이면 clear
  - 아니면 `QS_SKILL + nextSkillId`

### 8.7 navmesh 처리
- Persistence Drain은 이동 부하가 목적이 아니다.
- 권장 설정:
  - `move_hz=0`
  - `navmesh.enabled=false`
- 이렇게 해야 NavMesh 준비 상태가 테스트 결과를 오염시키지 않는다.

---

## 9. 권장 테스트 설정

### 9.1 공통 설정
```json
{
  "ccu_target": 200,
  "ramp_step": 50,
  "ramp_interval_sec": 10,
  "hold_sec": 300,
  "scenario": "persistence",
  "move_hz": 0,
  "skill_hz": 0,
  "heartbeat_hz": 1,
  "quickslot_hz": 1,
  "channel_id": 1,
  "map_id": 1,
  "spawn_cluster": {
    "radius": 0.0
  },
  "navmesh": {
    "enabled": false
  }
}
```

### 9.2 왜 hold_sec를 300으로 늘리는가
- AutoCommit을 `60초`로 설정해도 최소 몇 번의 tick은 봐야 한다.
- `180초`는 가능은 하지만 경계가 너무 빡빡하다.
- `300초`면 steady-state 구간에서 tick이 반복되는 패턴을 더 안정적으로 비교할 수 있다.

### 9.3 단계별 권장 CCU
- 1차 smoke
  - `20`
  - `50`
- 2차 functional
  - `100`
- 3차 비교 본실험
  - `200`
- baseline이 먼저 흔들리면
  - `50 / 100 / 150 / 200` staircase로 공통 안정 구간을 찾는다.

---

## 10. Grafana에서 봐야 할 패널

### 10.1 대표 패널
- `DB Pool Wait p95 (save_quickslot)`
- `Game -> DBAgent S2S RTT p95 (save_quickslot)`
- `DB Query p95 (save_quickslot)`

### 10.2 서브 패널
- `Pool InUse Ratio`
- `AutoCommit Tick p95`
- `AutoCommit Inflight`
- `Dirty Set Size`
- `QuickSlot Mutation Throughput`
- `AutoCommit Skip Reasons`

### 10.3 해석 포인트
- `immediate_quickslot`에서
  - DB pool wait와 S2S RTT가 더 빠르게 올라가면 baseline 부하 집중이 증명된다.
- `writeback`에서
  - pool wait는 낮지만 dirty set과 AutoCommit inflight가 커지면 writeback trade-off가 드러난다.
- 좋은 final 결과는 보통 다음 특성을 가진다.
  - pool wait가 더 낮다
  - RTT가 더 안정적이다
  - query time이 급격히 악화되지 않는다
  - dirty set/backlog가 통제 가능한 범위에서 움직인다

---

## 11. 바로 진행할 구현 순서

### 11.1 1단계: 설정 토글 추가
- `PersistenceMode`
- `AutoCommitIntervalSec`

### 11.2 2단계: GameServer 분기 추가
- `writeback`
- `immediate_quickslot`

### 11.3 3단계: AutoCommit/Dirty 메트릭 추가
- 새 지표 모두 등록
- `/metrics`에서 이름 확인

### 11.4 4단계: DummyClient persistence 시나리오 추가
- `scenario=persistence`
- `quickslot_hz`
- 고정 QuickSlot 패턴 구현

### 11.5 5단계: 스모크 테스트
- `20~50 CCU`
- writeback / immediate 각각 1회
- 메트릭이 정상적으로 증가하는지 확인

### 11.6 6단계: 본 실험
- `200 CCU`
- 동일 조건 A/B
- Grafana 패널과 CSV 추출

---

## 12. 구현 완료 판정 기준

### 12.1 기능 기준
- `PersistenceMode=writeback`에서 QuickSlot 변경이 dirty set으로 쌓인다.
- `PersistenceMode=immediate_quickslot`에서 QuickSlot 변경이 즉시 저장된다.
- DummyClient `scenario=persistence`가 move/skill 없이 quickslot만 반복 전송한다.

### 12.2 관측 기준
- `/metrics`에서 다음이 보여야 한다.
  - `gs_autocommit_tick_seconds`
  - `gs_autocommit_targets`
  - `gs_autocommit_inflight`
  - `gs_autocommit_sent_total`
  - `gs_autocommit_skip_total`
  - `gs_dirty_set_size`
  - `gs_persistence_mutation_total`
- 기존 메트릭과 함께 대시보드에서 A/B 비교가 가능해야 한다.

### 12.3 테스트 기준
- `20~50 CCU` smoke에서 크래시 없이 유지
- `200 CCU` 또는 공통 안정 구간에서 `save_quickslot` 관련 차이가 숫자로 드러남

---

## 13. 리스크와 주의사항

### 13.1 종료 시 flush 오염
- 현재 disconnect 시 [PlayerSession.cpp](/Users/GGI/Desktop/Github_job/Server-Job-Prep/Project/MmoNexusServerEngine/IocpChatServer/GameServer/PlayerSession.cpp#L55) 에서 `RequestFlushNow`가 호출된다.
- 따라서 종료 직전 구간은 별도 구간으로 보고, steady-state와 분리해 해석해야 한다.

### 13.2 inflight 정의 혼선
- AutoCommit inflight와 immediate 저장 RTT를 한 그래프에 섞으면 의미가 흐려질 수 있다.
- writeback 전용/즉시 저장 전용 의미를 명확히 유지해야 한다.

### 13.3 dirty set size 수집 비용
- `SMembers` 기반 계산은 테스트 규모가 커질수록 불필요한 오버헤드를 만든다.
- 가능하면 `SCard`를 추가하는 것이 좋다.

### 13.4 baseline 구현 과잉 단순화
- baseline은 의도적으로 단순해야 하지만, 중간 상태까지 여러 번 DB에 흘리는 구현은 계획서 의미를 벗어난다.
- `이벤트당 1회 즉시 저장` 원칙을 지켜야 한다.

---

## 14. 추천 최종 진행안

### 14.1 이번 구현에서 반드시 포함할 것
- `PersistenceMode`
- `AutoCommitIntervalSec`
- `scenario=persistence`
- `quickslot_hz`
- `gs_autocommit_*`
- `gs_dirty_set_size`
- `gs_persistence_mutation_total`

### 14.2 이번 구현에서 권장 포함할 것
- `RedisManager::SCard`
- skip reason 세분화
- 설정 JSON에 Persistence Drain 전용 예시값 반영

### 14.3 실제 작업 착수 순서
1. 설정 구조 변경
2. GameServer 분기 구현
3. AutoCommit 메트릭 추가
4. DummyClient persistence 시나리오 추가
5. Release 설정 작성
6. smoke run
7. Grafana 패널 정리

---

## 15. 실행 후 기대 산출물
- `writeback` CSV 1개 이상
- `immediate_quickslot` CSV 1개 이상
- 대표 패널 스크린샷
  - DB Pool Wait p95
  - S2S RTT p95
  - DB Query p95
- 서브 패널 스크린샷
  - AutoCommit Tick
  - AutoCommit Inflight
  - Dirty Set Size
  - Mutation Throughput
- 최종 비교 문장
  - `writeback은 pool wait를 낮추는 대신 dirty backlog와 tick 비용을 관리해야 한다`
  - `immediate baseline은 구현 단순성은 높지만 quickslot 저장 burst가 DBAgent/DB에 직접 전달된다`

---

## 16. 결론
- 현재 코드베이스는 `Persistence Drain`의 기반 경로는 이미 충분히 갖추고 있다.
- 하지만 계획서 기준 A/B 실험을 재현하려면 `실험 모드 토글`, `AutoCommit 전용 계측`, `Dummy persistence 시나리오`가 반드시 추가되어야 한다.
- 특히 `AutoCommitIntervalSec` 설정화는 결과 해석 품질을 크게 좌우하므로 우선순위를 높게 두는 것이 맞다.
- 본 문서의 순서대로 진행하면, 구현 직후 곧바로 smoke -> 본실험 -> Grafana 비교까지 자연스럽게 이어갈 수 있다.
