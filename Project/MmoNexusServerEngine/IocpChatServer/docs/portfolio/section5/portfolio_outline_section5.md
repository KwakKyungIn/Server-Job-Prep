## **Section 5: 운영 관측 & 성능 검증** (3페이지)

**Page 33 - 메모리/송신 성능 가드레일**
```text
ObjectPool + MemoryPool
- 빈번 객체 할당/해제 비용 감소

SendBufferChunk 풀링 + Backpressure
- 송신 버퍼 재사용
- 백로그 임계치 기반 과부하 제어
```

**Page 34 - 관측 체계 (Prometheus + Grafana)**
```text
공용 계층
- /metrics Exporter
- MetricsSystem 초기화

도메인 계측
- GameServer: packet/lobby/s2s/CCU
- DBAgent: req/query/pool wait

대시보드
- Grafana JSON 자산으로 재현 가능
```

**Page 35 - DummyClient 부하 검증 보고**
```text
시나리오
- idle / move / combat / mix
- CCU 램프업 + heartbeat + RTT 수집

산출물
- CSV(평균/P95/전송량)
- 기능 변경 전/후 비교표
```

---

