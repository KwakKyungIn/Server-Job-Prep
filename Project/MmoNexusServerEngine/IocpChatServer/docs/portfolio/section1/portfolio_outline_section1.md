## **Section 1: 개요 & 범위 정의** (3페이지)

**Page 3 - 프로젝트 한 페이지 요약 (정리본)**
```text
프로젝트 한 줄 요약
"실서비스형 MMORPG 서버 핵심 경로를 직접 구현"

핵심 차별화 5개
1) 서버 권위 이동 검증 + NavMesh 슬라이딩
2) AOI SpatialGrid + 스냅샷 배칭
3) 1:1 거래 2-Phase Commit (아이템/골드 원자성)
4) Redis Write-Back + AutoCommit + 종료 Flush
5) Prometheus/Grafana + DummyClient 부하 검증
```

**Page 4 - 시스템 아키텍처**
- Mermaid 다이어그램 필요
```text
[다이어그램]
Client ↔ LoginServer
        ↘ Redis (Token/Cache)
Client ↔ GameServer ↔ DBAgent ↔ RDBMS(ODBC)

컴포넌트 책임
- LoginServer: C_LOGIN 처리 + Redis 토큰 발급(TTL 300초)
- GameServer: 입장/플레이 서버 권위 판정 + Redis Dirty 반영
- DBAgent: ODBC Connection Pool + BEGIN/COMMIT 트랜잭션 전담

핵심 경로 요약
- 인증/입장: C_LOGIN → S_LOGIN(token) → C_ENTER_GAME(token) → 월드 입장
- 저장 경로: S2S_REQ_SAVE_* → DBAgent 트랜잭션 커밋

코드 근거
- LoginServer/LoginServer.cpp
- LoginServer/S2SPacketHandler.cpp
- GameServer/GameServer.cpp
- GameServer/AutoCommitService.cpp
- DBAgent/DBAgentPacketHandler.cpp
```

**Page 5 - End-to-End 데이터 흐름**
- Mermaid 다이어그램 필요
```text
Login: C_LOGIN → S2S_REQ_LOGIN → S_LOGIN(token)
Enter: C_ENTER_GAME → 토큰 검증 → 로비 로딩 게이트
Play : 이동/전투/거래/파티/던전
Save : Redis Dirty 누적 → AutoCommit → DB 저장
Exit : Disconnect 시 Flush + 인스턴스 정리

주요 저장 이벤트
- S2S_REQ_SAVE_PLAYER_CORE
- S2S_REQ_SAVE_INVENTORY
- S2S_REQ_SAVE_QUICKSLOT
- S2S_REQ_TRADE_COMMIT / S2S_RES_TRADE_COMMIT

실패 처리 원칙
- DB 실패 시 ROLLBACK으로 원자성 유지
- 종료 시 Flush로 미커밋 데이터 손실 최소화
- 로딩 게이트 실패 시 입장 차단 + 세션 정리

코드 근거
- GameServer/ClientPacketHandler.EnterGame.cpp
- GameServer/AutoCommitService.cpp
- GameServer/PersistenceService.cpp
- DBAgent/DBAgentPacketHandler.cpp
```

---

