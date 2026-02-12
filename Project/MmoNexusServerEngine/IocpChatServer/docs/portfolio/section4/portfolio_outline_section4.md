## **Section 4: 데이터 정합성 & 영속화** (5페이지)

**Page 28 - Redis 토큰 인증 + 세션 바인딩**
```text
LoginServer
- 토큰 생성 + Redis 저장(TTL)

GameServer
- 토큰 검증 후 playerId 확정
- 이름/세션 컨텍스트 복구
```

**Page 29 - Redis Write-Back + AutoCommit**
- Mermaid 다이어그램 필요
```text
런타임 변경
- Redis Hash 반영
- Dirty Set 기록

주기 저장
- AutoCommit이 Dirty 수집
- S2S_REQ_SAVE_*로 DBAgent 저장
- 성공 시 Dirty 제거
```

**Page 30 - DBAgent 트랜잭션 경로**
```text
ODBC + Connection Pool
- Prepared Statement
- BEGIN/COMMIT 트랜잭션
- Save/Load/Trade 커밋 전담

효과
- 게임 로직과 DB I/O 분리
```

**Page 31 - 부팅 데이터 동기화**
```text
S2S_REQ_LOAD_GAME_DATA
- Stat/Item/Skill Template 로드
- DataManager 메모리 적재

JSON 로드
- Maps/Monster/Spawn/Drop 테이블 반영
```

**Page 32 - GameItem UID 시드 부트스트랩**
```text
문제
- 재시작 시 UID 충돌 위험

해결
- DBAgent가 MAX(game_item_uid)+1 계산
- GameItemUidGen::Init(next_uid)
- 런타임 Alloc()로 단조 증가 발급
```

---

