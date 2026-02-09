# Prometheus/Grafana 연결 현황 전달 문서 (1~3단계 전체)

## 목적
- 이 문서는 현재 저장소의 계측 시스템 1~3단계 전체 구현 상태를 한 번에 전달하기 위한 핸드오프 문서다.
- 다른 AI에게 이 문서만 전달해도 Prometheus/Grafana 연결 안내를 받을 수 있도록 작성했다.

## 단계 진행 상태
- 1단계: 완료 (`2026-02-09`)
- 2단계: 완료 (`2026-02-09`)
- 3단계: 완료 (`2026-02-09`)
- 참고 기준 문서: `docs/prometheus_monitoring.md`

## 현재 메트릭 엔드포인트
- (호스트 직접 확인) GameServer: `http://127.0.0.1:8080/metrics`
- (호스트 직접 확인) DBAgent: `http://127.0.0.1:8081/metrics`
- (Prometheus 컨테이너 스크랩) GameServer: `http://host.docker.internal:8080/metrics`
- (Prometheus 컨테이너 스크랩) DBAgent: `http://host.docker.internal:8081/metrics`

## 메트릭 프리픽스
- GameServer: `gs_`
- DBAgent: `db_`

## 단계별 구현 파일

### 1단계 (공용 계측 인프라)
- `ServerCore/CoreGlobal.h`
- `ServerCore/CoreGlobal.cpp`
- `ServerCore/Metrics.h`
- `ServerCore/Metrics.cpp`
- `ServerCore/MetricsSystem.h`
- `ServerCore/MetricsSystem.cpp`
- `ServerCore/MetricsExporter.h`
- `ServerCore/MetricsExporter.cpp`
- `ServerCore/PrometheusTextRenderer.h`
- `ServerCore/PrometheusTextRenderer.cpp`
- `ServerCore/ProcessMetricsProvider.h`
- `ServerCore/ProcessMetricsProvider.cpp`
- `ServerCore/ServerCore.vcxproj`
- `ServerCore/ServerCore.vcxproj.filters`

### 2단계 (Core + GameServer 런타임 계측)
- `ServerCore/GlobalQueue.cpp`
- `ServerCore/JobQueue.cpp`
- `ServerCore/ThreadManager.cpp`
- `ServerCore/Session.cpp`
- `ServerCore/PacketMetricsHooks.h`
- `GameServer/GameMetrics.h`
- `GameServer/GameMetrics.cpp`
- `GameServer/GameServer.cpp`
- `GameServer/GameSessionManager.cpp`
- `GameServer/LobbyRoom.cpp`
- `GameServer/PlayerSession.cpp`
- `GameServer/ClientPacketHandler.h` (패킷 훅 호출 지점)
- `GameServer/S2SPacketHandler.h` (S2S 훅 경로)

### 3단계 (DBAgent 계측 + 대시보드/스크랩 설정)
- `DBAgent/DBAgentMetrics.h`
- `DBAgent/DBAgentMetrics.cpp`
- `DBAgent/DBAgent.cpp`
- `DBAgent/DBAgentPacketHandler.cpp`
- `DBAgent/DBAgentPacketHandler.h` (패킷 훅 호출 지점)
- `DBAgent/DBConnection.cpp`
- `DBAgent/DBConnectionPool.h`
- `DBAgent/DBConnectionPool.cpp`
- `DBAgent/DBAgent.vcxproj`
- `DBAgent/DBAgent.vcxproj.filters`
- `docs/monitoring/prometheus.yml`
- `docs/monitoring/grafana_dbagent_dashboard.json`
- `docs/monitoring/dbagent_dashboard_queries.md`
- `docs/monitoring/grafana_gameserver_dashboard.json`
- `docs/monitoring/gameserver_dashboard_queries.md`

## 연결에 필요한 설정 파일
- `GameServer/ServerConfig.json`
- `DBAgent/ServerConfig.json`
- `docs/monitoring/prometheus.yml`
- `docs/monitoring/grafana_dbagent_dashboard.json`
- `docs/monitoring/dbagent_dashboard_queries.md`
- `docs/monitoring/grafana_gameserver_dashboard.json`
- `docs/monitoring/gameserver_dashboard_queries.md`

## 공통 설정 정보 (알아야 할 값)
- `Metrics.Enabled`: true/false로 exporter on/off
- `Metrics.Port`: exporter 포트
- `Metrics.Prefix`: 메트릭 접두어 (`gs_`, `db_`)
- `Metrics.Path`: 기본 `"/metrics"`
- `Metrics.BindAddress`: 기본 `"127.0.0.1"`
- 설정 로딩 위치: `ServerCore/CoreGlobal.cpp`
- 설정 파일 규칙: 실행 폴더에서 `ServerConfig.<ExeName>.json` 우선, 없으면 `ServerConfig.json`

## 현재 실제 설정값
- `GameServer/ServerConfig.json`
- `"Enabled": true`
- `"Port": 8080`
- `"Prefix": "gs_"`
- `DBAgent/ServerConfig.json`
- `"Enabled": true`
- `"Port": 8081`
- `"Prefix": "db_"`
- `Binary/Debug/ServerConfig.GameServer.json`
- `"BindAddress": "0.0.0.0"`
- `Binary/Debug/ServerConfig.DBAgent.json`
- `"BindAddress": "0.0.0.0"`

## 메트릭 인벤토리 (1~3단계 전체)

### 1단계 공통 프로세스 메트릭 (프로세스별 prefix 적용)
- `process_uptime_seconds`
- `process_cpu_seconds_total{mode="user|system"}`
- `process_resident_memory_bytes`
- `metrics_response_seconds`

### 2단계 Core 메트릭 (프로세스별 prefix 적용)
- `globalqueue_push_total{queue}`
- `globalqueue_depth{queue,shard}`
- `globalqueue_pop_total{reason}`
- `globalqueue_steal_attempt_total`
- `globalqueue_steal_success_total`
- `jobqueue_depth{queue}`
- `jobqueue_batch_size{queue}`
- `jobqueue_wait_seconds{queue}`
- `jobqueue_exec_seconds{queue}`
- `worker_idle_seconds_total{type}`
- `worker_exec_seconds_total{type}`
- `worker_active{type}`
- `session_rx_bytes_total`
- `session_tx_bytes_total`

### 2단계 GameServer 메트릭 (`gs_` prefix)
- `packet_ingress_total{op}`
- `packet_handle_seconds{op}`
- `packet_failure_total{op,reason}`
- `lobby_wait_seconds{type}`
- `s2s_rtt_seconds{op}`
- `ccu`
- `ingame_players`

### 3단계 DBAgent 메트릭 (`db_` prefix)
- `req_total{op}`
- `req_handle_seconds{op}`
- `req_failure_total{op,reason}`
- `query_seconds{op}`
- `pool_wait_seconds{op}`
- `pool_size`
- `pool_inuse`

## prefix 적용 예시 (실제 쿼리할 이름)
- GameServer 예시: `gs_packet_handle_seconds`, `gs_jobqueue_wait_seconds`
- DBAgent 예시: `db_req_handle_seconds`, `db_query_seconds`, `db_pool_wait_seconds`

## Prometheus 연결 체크리스트
- Prometheus 타깃:
- `host.docker.internal:8080` (`job_name: game_server`)
- `host.docker.internal:8081` (`job_name: db_agent`)
- metrics_path: `/metrics`
- 스크랩 주기: `1s` (샘플 파일 기준)
- 설정 파일: `docs/monitoring/prometheus.yml`

## Grafana 연결 체크리스트
- 데이터소스: Prometheus URL (예: `http://127.0.0.1:9090`)
- Import JSON:
- `docs/monitoring/grafana_dbagent_dashboard.json`
- `docs/monitoring/grafana_gameserver_dashboard.json`
- 패널별 PromQL 참고:
- `docs/monitoring/dbagent_dashboard_queries.md`
- `docs/monitoring/gameserver_dashboard_queries.md`

## 주의사항
- SQL 원문/플레이어 식별자(`playerId`, `sessionId`, 닉네임)는 라벨로 사용하지 않음
- 라벨은 고정 집합(`op`, `reason`, `queue`, `shard`, `type`) 중심으로 설계됨
- 3단계의 포트폴리오 결과 문서화 항목은 사용자 직접 진행으로 제외됨
