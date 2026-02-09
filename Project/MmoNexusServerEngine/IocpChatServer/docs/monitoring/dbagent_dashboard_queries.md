# DBAgent Grafana 패널/쿼리 정리

## 공통
- 데이터소스: Prometheus
- 기본 범위: 최근 15분
- 해상도 기준: `$__rate_interval`

## 패널 1) DBAgent Request p95
- 목적: 요청 처리 지연의 요청 타입별 상위 95% 추적
- PromQL:
```promql
histogram_quantile(
  0.95,
  sum(rate(db_req_handle_seconds_bucket{op!="other"}[$__rate_interval])) by (le, op)
)
```

## 패널 2) DB Query p95
- 목적: 실제 SQL 실행 구간 지연의 요청 타입별 상위 95% 추적
- PromQL:
```promql
histogram_quantile(
  0.95,
  sum(rate(db_query_seconds_bucket{op!="other"}[$__rate_interval])) by (le, op)
)
```

## 패널 3) DB Pool Wait p95
- 목적: 커넥션 풀 부족/경합으로 인한 대기 시간 추적
- PromQL:
```promql
histogram_quantile(
  0.95,
  sum(rate(db_pool_wait_seconds_bucket[$__rate_interval])) by (le, op)
)
```

## 패널 4) DBAgent Request Throughput
- 목적: 요청 타입별 처리량(RPS) 확인
- PromQL:
```promql
sum by (op) (rate(db_req_total[$__rate_interval]))
```

## 패널 5) DBAgent Request Failures
- 목적: 파싱/검증/핸들러 실패를 요청 타입별로 분리
- PromQL:
```promql
sum by (op, reason) (rate(db_req_failure_total[$__rate_interval]))
```

## 패널 6) DB Pool Size vs InUse
- 목적: 설정 풀 크기 대비 실사용량 변화 추적
- PromQL:
```promql
db_pool_size
```
```promql
db_pool_inuse
```
