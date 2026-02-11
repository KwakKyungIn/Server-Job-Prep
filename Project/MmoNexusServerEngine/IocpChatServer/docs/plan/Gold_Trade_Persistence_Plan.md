# 골드 교환 + 저장 기능 기획서

## 목표
플레이어 골드(currency)를 아래 흐름으로 완성한다.
- DB 로드: 로그인 시 `PLAYERS.gold` 로딩
- 저장: 자동 저장(오토커밋) 및 명시적 저장에 포함
- 교환: 기존 거래 흐름에 골드 추가 (아이템과 **원자적** 처리)
- 표시: 본인에게만 전달(다른 플레이어에게는 노출 금지)

## 반드시 지켜야 할 제약
- `protoc`, `genpackets.bat` **실행 금지** (사용자가 직접 실행)
- 자동 생성된 핸들러 헤더 **수정 금지** (예: `ClientPacketHandler.h`, `S2SPacketHandler.h`)
  - 수정이 필요하면 반드시 `.proto`를 수정
- Unity 스크립트는 `Client/Assets/Scripts/` 하위에 있음

## 설계 핵심 (골드 노출 방지)
`PlayerInfo`는 `S_SPAWN`으로 주변 플레이어에게 브로드캐스트된다.
따라서 `PlayerInfo`나 `StatInfo`에 골드를 넣으면 **골드가 타인에게 노출**된다.

→ 골드는 **Player 객체의 별도 필드**로 관리하고, 본인에게만 전송한다.
- `S_ENTER_GAME.gold`
- `S_GOLD_UPDATE`

## 데이터 흐름 요약
1) 로그인
- DBAgent가 `level, hp, total_exp, gold`를 로드
- GameServer가 Player에 골드를 세팅 + Redis core에 프라임
- 클라는 `S_ENTER_GAME.gold`를 통해 UI 표시

2) 런타임 변경
- 골드 변화 시 Player 값 + Redis core 해시의 `gold` 갱신
- AutoCommit에서 `S2S_REQ_SAVE_PLAYER_CORE`로 저장

3) 거래
- 클라가 골드 제안을 설정
- 서버가 골드 충분 여부 검증
- Trade Commit에서 아이템 + 골드를 **하나의 SQL 트랜잭션**으로 반영
- 성공 후 `S_GOLD_UPDATE`를 양쪽에 전송

---

## 프로토콜 변경 (Common/Protobuf/bin)

### Protocol_S2S.proto
플레이어 코어 저장/로드 및 거래 커밋에 골드 추가.
- `S2S_RES_LOAD_PLAYER_DATA`: `int64 gold` 추가
- `S2S_REQ_SAVE_PLAYER_CORE`: `int64 gold` 추가
- `S2S_REQ_TRADE_COMMIT`: `int64 finalGoldA`, `int64 finalGoldB` 추가
  - **최종값 방식 권장** (delta 방식보다 안전)

### Protocol.proto
클라용 골드 전달 메시지 추가.
- `S_ENTER_GAME`: `int64 gold` 추가 (본인 전용)
- 신규 메시지: `S_GOLD_UPDATE { int64 gold = 1; }`

### 거래 관련
- `S_TRADE_OFFER_UPDATE`: `int64 gold = 4` 추가 (whoPlayerId가 올린 골드)
- 신규 요청 메시지: `C_TRADE_GOLD_SET { uint64 tradeId = 1; int64 gold = 2; }`
  - 아이템과 별도 메시지로 분리 (안전하고 명확)

### Enum.proto (선택)
필요하면 골드 부족 오류 코드 추가:
- `TRADE_FAIL_NOT_ENOUGH_GOLD`

---

## 서버 변경 (C++)

### Player 상태
- `GameServer/Player.h`, `GameServer/Player.cpp`
  - `int64 _gold` 추가
  - `GetGold()/SetGold()/AddGold()` 제공
  - `PlayerInfo`에는 포함하지 않음

### 로그인/로비
- `GameServer/LobbyRoom.cpp`
  - `OnStatLoaded`: `pkt.gold()` 읽어서 Player에 저장
  - Redis core 프라임 시 gold 포함

- `GameServer/GameRoom.EnterLeave.cpp`
  - `S_ENTER_GAME`에 gold 세팅

### Persistence (Redis/AutoCommit)
- `GameServer/PersistenceService.h/.cpp`
  - `PrimeFromDb_PlayerCore`에서 gold 저장
  - `UpdatePlayerGold(pid, gold, markDirty)` 추가
  - `BuildSnapshot_PlayerCore`에서 gold 포함 (필수 필드로 체크)

- `GameServer/AutoCommitService.cpp`
  - 구조 변경 없음 (core snapshot에 gold 포함)

### Trade 시스템
- `GameServer/GameRoom.h`
  - `TradeSession`에 `offerGoldA`, `offerGoldB`
  - `TradeCommitPlan`에 `finalGoldA`, `finalGoldB`

- `GameServer/GameRoom.Trade.cpp`
  - `HandleTradeGoldSetById` 추가
  - 골드 설정 시 Ready/Confirm 초기화
  - `SendOfferUpdate_ActorOnly`에서 gold 포함
  - `BuildTradeCommitPlan_ActorOnly`에서 최종 골드 계산/검증
  - `StartTradeCommitPhase2_ActorOnly`에서 최종 골드 전달
  - `OnTradeCommitResult_ActorOnly`에서:
    - Player 골드 갱신
    - Redis gold 갱신 (markDirty=false)
    - `S_GOLD_UPDATE` 전송

### ClientPacketHandler.Trade
- `GameServer/ClientPacketHandler.Trade.cpp`
  - `C_TRADE_GOLD_SET` 핸들러 추가

### DBAgent
- `DBAgent/DBAgentPacketHandler.cpp`
  - Load: `SELECT level, hp, total_exp, gold`
  - Save: `UPDATE PLAYERS SET level=?, hp=?, total_exp=?, gold=?`
  - Trade Commit: 같은 트랜잭션에서 A/B 골드 업데이트

---

## 클라이언트 변경 (Unity)

### 패킷 생성
- `Client/Assets/Scripts/Packet/` 자동 생성 파일 직접 수정 금지
- `.proto` 수정 후 사용자 gen 수행

### 거래 UI/로직
- `Client/Assets/Scripts/Game/TradeManager.cs`
  - `MyGoldOffer`, `PeerGoldOffer` 관리
  - `S_TRADE_OFFER_UPDATE.gold` 반영
  - 골드 변경 시 `C_TRADE_GOLD_SET` 전송

- `Client/Assets/Scripts/Game/TradeApi.cs`
  - `OfferGold(ulong tradeId, long gold)` 추가

- `Client/Assets/Scripts/UI/UI_TradePanel.cs`
  - 골드 입력/표시 UI 추가
  - Locked 상태에서는 수정 불가

### 골드 표시
- HUD에 골드 표시 기능 추가
- `S_ENTER_GAME.gold` 및 `S_GOLD_UPDATE` 수신 처리

---

## 검증/예외 처리
- 골드는 음수 불가
- 제안 골드 > 보유 골드면 거절
- 제안 변경 시 Ready/Confirm 초기화
- 거래 취소/실패 시 골드 변화 없음
- DB 트랜잭션에서 아이템+골드가 원자적으로 반영되어야 함

---

## 수정 대상 파일 요약
- `Common/Protobuf/bin/Protocol.proto`
- `Common/Protobuf/bin/Protocol_S2S.proto`
- `Common/Protobuf/bin/Enum.proto` (선택)
- `GameServer/Player.h`
- `GameServer/Player.cpp`
- `GameServer/LobbyRoom.cpp`
- `GameServer/GameRoom.EnterLeave.cpp`
- `GameServer/PersistenceService.h`
- `GameServer/PersistenceService.cpp`
- `GameServer/GameRoom.h`
- `GameServer/GameRoom.Trade.cpp`
- `GameServer/ClientPacketHandler.Trade.cpp`
- `DBAgent/DBAgentPacketHandler.cpp`
- `Client/Assets/Scripts/Game/TradeManager.cs`
- `Client/Assets/Scripts/Game/TradeApi.cs`
- `Client/Assets/Scripts/UI/UI_TradePanel.cs`

---

## 수동 테스트 체크리스트
1) 로그인 후 골드 정상 로딩/표시
2) 골드만 거래 → 양쪽 골드 변경 확인
3) 아이템 + 골드 동시 거래 → 원자적 반영 확인
4) 거래 취소/타임아웃 → 골드 변화 없음
5) 거래 중 접속 종료 → 골드 변화 없음
6) 서버 재시작 후 DB 골드 유지 확인

---

## 주의사항
- `.proto` 변경 시 필드 번호는 새 번호로 추가 (기존 번호 재사용 금지)
- protoc/genpackets는 사용자 실행 대상
- 자동 생성 핸들러 헤더는 절대 수정하지 말 것
