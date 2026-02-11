# GameServer Grafana 패널/쿼리 정리

## 공통
- 데이터소스: Prometheus
- 기본 범위: 최근 15분
- 해상도 기준: `$__rate_interval`

## 패널 1) Game Packet Handle p95
- 목적: 클라이언트 패킷 처리 지연 상위 95% 추적
- PromQL:
```promql
histogram_quantile(
  0.95,
  sum(rate(gs_packet_handle_seconds_bucket{op!="other"}[$__rate_interval])) by (le, op)
)
```

## 패널 2) Game JobQueue Wait p95
- 목적: JobQueue 대기 지연 상위 95% 추적
- PromQL:
```promql
histogram_quantile(
  0.95,
  sum(rate(gs_jobqueue_wait_seconds_bucket[$__rate_interval])) by (le)
)
```

## 패널 3) Game JobQueue Exec p95
- 목적: Job 실행 구간 지연 상위 95% 추적
- PromQL:
```promql
histogram_quantile(
  0.95,
  sum(rate(gs_jobqueue_exec_seconds_bucket[$__rate_interval])) by (le)
)
```

## 패널 4) Lobby Enter Wait p95
- 목적: 로비 입장 완료 지연 상위 95% 추적
- PromQL:
```promql
histogram_quantile(
  0.95,
  sum(rate(gs_lobby_wait_seconds_bucket{type="enter_game"}[$__rate_interval])) by (le)
)
```

## 패널 5) CCU vs Ingame
- 목적: 접속자/인게임 동시 변화 추적
- PromQL:
```promql
gs_ccu
```
```promql
gs_ingame_players
```

## 패널 6) Worker Idle vs Exec (Rate)
- 목적: 로직 워커 유휴/실행 시간 비율 추적
- PromQL:
```promql
sum(rate(gs_worker_idle_seconds_total[$__rate_interval]))
```
```promql
sum(rate(gs_worker_exec_seconds_total[$__rate_interval]))
```

## 패널 7) Packet Failures
- 목적: 파싱/검증/핸들러 실패율 추적
- PromQL:
```promql
sum by (op, reason) (rate(gs_packet_failure_total[$__rate_interval]))
```

## 패널 8) Packet Ingress Throughput
- 목적: 패킷 타입별 처리량(RPS) 추적
- PromQL:
```promql
sum by (op) (rate(gs_packet_ingress_total[$__rate_interval]))
```

## 패널 9) S2S RTT p95
- 목적: GameServer->DBAgent S2S RTT 상위 95% 추적
- PromQL:
```promql
histogram_quantile(
  0.95,
  sum(rate(gs_s2s_rtt_seconds_bucket{op!="other"}[$__rate_interval])) by (le, op)
)
```

## 패널 10) Session RX/TX Throughput
- 목적: 세션 I/O 처리량 추적
- PromQL:
```promql
rate(gs_session_rx_bytes_total[$__rate_interval])
```
```promql
rate(gs_session_tx_bytes_total[$__rate_interval])
```

## 패널 11) Queue vs Skill/UseItem Handle p95 (CSV)
- 목적: 큐 대기 병목인지(핫 룸) 핸들러 자체 지연인지 한 패널에서 비교
- PromQL:
```promql
histogram_quantile(
  0.95,
  sum(rate(gs_jobqueue_wait_seconds_bucket{queue="job"}[$__rate_interval])) by (le)
)
```
```promql
histogram_quantile(
  0.95,
  sum(rate(gs_packet_handle_seconds_bucket{op="skill"}[$__rate_interval])) by (le)
)
```
```promql
histogram_quantile(
  0.95,
  sum(rate(gs_packet_handle_seconds_bucket{op="use_item"}[$__rate_interval])) by (le)
)
```

## 패널 12) CSV Snapshot - Major Signals
- 목적: CSV 오프라인 분석용으로 주요 운영 지표를 한 패널에 집약
- PromQL (시리즈별):
```promql
gs_ccu
gs_ingame_players
histogram_quantile(0.95, sum(rate(gs_jobqueue_wait_seconds_bucket{queue="job"}[$__rate_interval])) by (le))
histogram_quantile(0.95, sum(rate(gs_jobqueue_exec_seconds_bucket{queue="job"}[$__rate_interval])) by (le))
max(gs_jobqueue_depth{queue="job"})
sum(gs_globalqueue_depth{queue="global"})
sum(rate(gs_worker_exec_seconds_total{type="logic"}[$__rate_interval]))
sum(rate(gs_worker_idle_seconds_total{type="logic"}[$__rate_interval]))
sum(rate(gs_worker_exec_seconds_total{type="logic"}[$__rate_interval])) / clamp_min(sum(rate(gs_worker_exec_seconds_total{type="logic"}[$__rate_interval])) + sum(rate(gs_worker_idle_seconds_total{type="logic"}[$__rate_interval])), 1e-9)
sum(rate(gs_packet_ingress_total[$__rate_interval]))
sum(rate(gs_packet_ingress_total{op="skill"}[$__rate_interval]))
sum(rate(gs_packet_ingress_total{op="use_item"}[$__rate_interval]))
sum(rate(gs_packet_failure_total[$__rate_interval]))
histogram_quantile(0.95, sum(rate(gs_packet_handle_seconds_bucket[$__rate_interval])) by (le))
histogram_quantile(0.95, sum(rate(gs_packet_handle_seconds_bucket{op="skill"}[$__rate_interval])) by (le))
histogram_quantile(0.95, sum(rate(gs_packet_handle_seconds_bucket{op="use_item"}[$__rate_interval])) by (le))
histogram_quantile(0.95, sum(rate(gs_s2s_rtt_seconds_bucket{op!="other"}[$__rate_interval])) by (le))
rate(gs_session_rx_bytes_total[$__rate_interval])
rate(gs_session_tx_bytes_total[$__rate_interval])
histogram_quantile(0.95, sum(rate(db_req_handle_seconds_bucket{op!="other"}[$__rate_interval])) by (le))
histogram_quantile(0.95, sum(rate(db_query_seconds_bucket{op!="other"}[$__rate_interval])) by (le))
histogram_quantile(0.95, sum(rate(db_pool_wait_seconds_bucket{op!="other"}[$__rate_interval])) by (le))
db_pool_inuse / clamp_min(db_pool_size, 1)
```
