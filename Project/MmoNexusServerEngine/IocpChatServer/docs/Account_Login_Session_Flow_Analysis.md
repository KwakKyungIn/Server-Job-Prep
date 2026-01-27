# Account/Login/Session Flow Analysis and Detailed Summary

이 문서는 계정/로그인/세션 흐름을 **처음 서버 프로그래밍을 접하는 사람도 이해할 수 있도록** 아주 자세히 설명합니다. 로그인 → 토큰 발급 → 게임 접속 → 세션 바인딩 → 로비 게이트 → 월드 입장까지의 실제 흐름을 코드 근거와 함께 정리했습니다.

> 문서 제목만 영어, 내용은 한국어로 작성합니다.

---

## 0) 문서 범위와 목표

- **대상 기능**: `docs/Portfolio_FeatureList.md`의 1) 계정/로그인/세션 파트.
- **핵심 목표**: “왜 이런 흐름을 쓰는지”, “어디서 어떤 데이터가 오가는지”, “어떤 스레드/컨텍스트에서 실행되는지”를 초보자 눈높이로 설명.
- **근거 코드 범위**:
  - LoginServer, DBAgent, GameServer 내부 코드
  - `Common/Protobuf/bin/Protocol.proto`
  - `ServerCore/RedisManager.h` 및 관련 세션/JobQueue

---

## 1) 전체 시스템 구성 한눈에 보기

### 1.1 서버 역할 요약

- **LoginServer**
  - 로그인 인증 게이트.
  - 클라이언트의 `C_LOGIN`을 받아 DB로 확인 요청.
  - 성공 시 **토큰을 발급**하고 Redis에 저장.
  - 클라이언트에 `S_LOGIN`(토큰 + 서버 리스트) 응답.

- **DBAgent**
  - SQL DB 접근을 전담.
  - LoginServer/GameServer의 S2S 요청을 받아 **DB 조회/저장 처리**.
  - 네트워크 스레드를 막지 않기 위해 **JobQueue에서 DB 작업 수행**.

- **GameServer**
  - 실제 게임 로직 서버.
  - `C_ENTER_GAME`에서 **토큰 검증 → playerId 바인딩 → 로비 게이트 → DB 데이터 로드 → 월드 입장** 흐름 수행.

- **Redis**
  - **로그인 토큰의 짧은 생명주기 캐시**.
  - 이후에는 플레이어 코어/인벤/퀵슬롯의 **write-back 캐시**로도 사용.

### 1.2 네트워크 연결과 포트 (실제 코드 기준)

**LoginServer** (`LoginServer/LoginServer.cpp`)
- 클라이언트 접속용: `127.0.0.1:7775` (ServerService)
- GameServer 접속용: `127.0.0.1:7780` (ServerService)
- DBAgent 접속용: `127.0.0.1:7779` (ClientService)

**GameServer** (`GameServer/GameServer.cpp`)
- 클라이언트 접속용: `127.0.0.1:7777` (ServerService)
- LoginServer 접속용: `127.0.0.1:7780` (ClientService)
- DBAgent 접속용: `127.0.0.1:7778` (ClientService)

> 주의: LoginServer 콘솔 로그에 “Listening on 7776” 문구가 있으나 실제 포트는 7775입니다. (코드 우선)

---

## 2) 초보자를 위한 용어 설명 (핵심 개념)

### 2.1 Session 이란?

- **Session = 하나의 TCP 연결**을 추상화한 객체.
- Session에는 **네트워크 소켓**, **수신 버퍼**, **send/recv 이벤트 처리 함수**가 있음.
- 이 프로젝트에서 `PacketSession`은 “패킷 단위”로 데이터를 파싱하고 핸들러로 넘기는 역할을 합니다.

관련 코드:
- `ServerCore/Session.h`, `ServerCore/Session.cpp`
- LoginServer: `ClientSession`, `DBAgentSession`
- GameServer: `PlayerSession`, `DBSession`, `LoginSession`

### 2.2 JobQueue / Actor 방식

- 여러 스레드가 동시에 플레이어 데이터나 룸 상태를 건드리면 **데이터 레이스**가 발생합니다.
- 이를 막기 위해 **“한 객체는 한 스레드에서만 수정한다”**는 규칙을 둡니다.
- 그 구현이 `JobQueue`입니다.
  - 네트워크 스레드에서는 **작업을 큐에 넣기만** 하고,
  - 실제 로직 스레드(세션/룸 actor)가 그 작업을 **순차적으로 실행**합니다.

관련 코드:
- `ServerCore/JobQueue.h`
- `DBAgent/GameSession.h` (DBAgent 요청을 JobQueue로 처리)
- `GameServer/PlayerSession.h` (Post API)
- `GameServer/LobbyRoom.cpp`, `GameServer/GameRoom.*`

### 2.3 Redis를 왜 쓰나?

- 로그인 토큰은 **“짧은 시간만 유효한 자격증명”**입니다.
- DB에 저장하면 느리고, 즉시 삭제/만료도 번거롭습니다.
- Redis는 **빠른 조회 + TTL**에 최적화되어 있어 로그인 토큰 캐시에 적합합니다.

관련 코드:
- `ServerCore/RedisManager.h`
- `LoginServer/S2SPacketHandler.cpp` (token 저장)
- `GameServer/ClientPacketHandler.EnterGame.cpp` (token 검증)

---

## 3) 로그인 패킷 요약

### 3.1 로그인 프로토콜

- `C_LOGIN`: `userId`, `password`
- `S_LOGIN`: `success`, `serverList`, `token`

정의:
- `Common/Protobuf/bin/Protocol.proto`

### 3.2 Enter Game 프로토콜

- `C_ENTER_GAME`: `token`, `channelId`, `mapId`
- `S_ENTER_GAME`: `success`, `myPlayer`

정의:
- `Common/Protobuf/bin/Protocol.proto`

---

## 4) 로그인 흐름 (LoginServer + DBAgent)

### 4.1 클라이언트 관점

1. 아이디/비밀번호로 로그인 요청.
2. 성공하면 서버 리스트 + 토큰 수신.
3. 토큰을 들고 GameServer에 접속.

### 4.2 서버 내부 흐름 (완전 상세)

#### Step 1) LoginServer가 `C_LOGIN` 처리

- 코드: `LoginServer/ClientPacketHandler.cpp`

처리 과정:
1) `ClientSession`으로 캐스팅 (LoginServer는 “가벼운 세션” 사용).
2) `userId`가 빈 문자열이면 실패 처리.
3) 로그인 이름을 `ClientSession::_loginName`에 저장.
4) DBAgent 연결 여부 확인 (`GDBAgentSession == nullptr`이면 실패).
5) DBAgent에게 `S2S_REQ_LOGIN` 요청 전송.

특징:
- **password는 사용되지 않음** (주석 처리).
- 즉, 현재 로그인은 **ID 존재 여부만 확인**.

#### Step 2) DBAgent가 DB 조회

- 코드: `DBAgent/DBAgentPacketHandler.cpp`

처리 과정:
1) DBAgent는 받은 패킷을 `GameSession`으로 캐스팅.
2) **JobQueue에 람다를 등록** (DB 작업은 로직 스레드에서).
3) DB 연결 풀에서 커넥션 획득.
4) SQL 실행:
   - `SELECT playerId FROM Players WHERE name = ?`
5) Fetch 성공 시 `success=true`, `playerId` 기록.
6) 실패 시 `success=false`, TODO 주석만 존재.
7) DB 연결 반환 후 응답 패킷 전송 (`S2S_RES_LOGIN`).

핵심 이유:
- DB 쿼리는 느리므로 **네트워크 스레드를 막으면 안 됨**.
- 그래서 JobQueue로 분리해 처리.

#### Step 3) LoginServer가 토큰 발급

- 코드: `LoginServer/S2SPacketHandler.cpp`

처리 과정:
1) `playerSessionId`로 원래 `ClientSession` 찾기.
2) 성공 시 토큰 생성:
   - `Token_{playerId}_{timestamp}_{random}`
3) Redis 저장:
   - `token -> playerId`, TTL 300초(5분)
4) 추가로 이름 캐싱:
   - `token:name:{token} -> loginName`
5) 클라이언트에게 `S_LOGIN` 전송 (token + confirm).

**왜 `playerSessionId`를 쓰나?**
- DBAgent 응답이 돌아오는 동안 클라이언트가 여러 명일 수 있습니다.
- 그래서 **요청-응답 매칭을 위한 “왕복 티켓”**으로 `playerSessionId`를 사용합니다.

---

## 5) GameServer 입장 흐름 (Token → 로비 → 월드)

### 5.1 클라이언트 관점

1. `C_ENTER_GAME(token, channelId, mapId)` 요청.
2. 서버가 토큰 검증 후 데이터 로딩.
3. 로딩 완료 시 `S_ENTER_GAME` + 주변 스냅샷 수신.

### 5.2 서버 내부 상세 흐름

#### Step 1) 토큰 검증

- 코드: `GameServer/ClientPacketHandler.EnterGame.cpp`

1) `PlayerSession` 캐스팅.
2) Redis에서 토큰 조회:
   - `GRedisManager->Get(token)`
3) 값이 없으면 즉시 `Disconnect`.
4) 값이 있으면 `playerId`로 변환.
5) `token:name:{token}` 키로 플레이어 이름 조회.

핵심: 토큰이 없으면 **즉시 차단**. (보안 게이트)

#### Step 2) 맵/채널 검증

- 채널 ID가 0 이하이면 1로 보정.
- `DataManager`로 맵 ID 유효성 검사.
- 월드맵이 아니거나 잘못된 맵이면 기본 월드맵으로 보정.
- 맵 설정이 없으면 기본 스폰 좌표 사용.

이 단계의 목적:
- 잘못된 맵 요청을 무시해 **서버/클라 크래시 방지**.

#### Step 3) 세션 바인딩 및 Pending Enter 설정

- 코드: `GameServer/ClientPacketHandler.EnterGame.cpp`, `GameServer/GameSessionManager.cpp`, `GameServer/PlayerSession.h`

1) `GameSessionManager::BindPlayerId`로 `sessionId ↔ playerId` 맵핑.
2) `PlayerSession`에 Pending Enter 데이터 저장:
   - channelId, mapId, instanceId
3) 이 값은 **DB 응답 라우팅에 필요**.

왜 Pending Enter가 필요한가?
- DBAgent 응답이 올 때는 **세션 ID만 알고 있음**.
- 어느 로비로 보내야 하는지 알아야 하므로 pending 채널/맵 정보를 세션에 저장해둔다.

#### Step 4) LobbyRoom에서 Player 객체 준비

- 코드: `GameServer/LobbyRoom.cpp`

1) `LobbyRoom::EnterGame` 호출.
2) Player 객체가 없으면 생성, 있으면 재활용.
3) 이름 결정: Redis 이름이 없으면 기본값 생성.
4) Player에 spawn/채널/맵/세션 설정.
5) 로딩 플래그 초기화:
   - `itemsLoaded=false`, `statLoaded=false`, `quickLoaded=false`
6) LobbyRoom에 등록 (`Adopt`).

왜 로비를 쓰는가?
- 데이터 로딩이 끝나기 전에는 **월드에 들어가면 안 됨**.
- 로비는 “게이트키퍼” 역할.

#### Step 5) DBAgent에 데이터 요청

- 코드: `GameServer/ClientPacketHandler.EnterGame.cpp`

전송 패킷:
- `S2S_REQ_LOAD_PLAYER_DATA` (스탯)
- `S2S_REQ_ITEMS_LOAD` (아이템)
- `S2S_REQ_QUICKSLOT_LOAD` (퀵슬롯)

각 패킷에는 `gamesessionid`가 포함되어 **응답 라우팅**에 사용됨.

#### Step 6) DBAgent 응답 처리

- 코드: `GameServer/S2SPacketHandler.cpp`

1) `gamesessionid`로 세션 찾기.
2) `pendingChannelId`로 해당 로비 찾기.
3) 로비의 JobQueue에 `OnStatLoaded` / `OnItemsLoaded` / `OnQuickSlotsLoaded` 등록.

#### Step 7) 로비에서 데이터 적용

- 코드: `GameServer/LobbyRoom.cpp`

**OnStatLoaded**
- Player StatInfo 적용.
- `RefreshStats()`로 파생 스탯 계산.
- Redis에 Prime.
- `statLoaded=true`.

**OnItemsLoaded**
- 인벤토리 로딩 및 장착 슬롯 정규화.
- 중복 장착 발견 시 강제 해제 + DB/Redis 동기화.
- `RefreshStats()` 재계산.
- Redis Prime + 클라에 `S_ITEM_LIST` 전송.
- `itemsLoaded=true`.

**OnQuickSlotsLoaded**
- Redis Prime.
- 클라에 `S_QUICKSLOT_LIST` 전송.
- `quickLoaded=true`.

#### Step 8) 모든 로딩 완료 시 월드 입장

- `TryEnterWorldIfReady` 조건:
  - player 존재 + itemsLoaded + statLoaded + quickLoaded
- 로비에서 Player 분리 (`Detach`).
- `RoomManager`로 GameRoom 찾기/생성.
- Pending Enter 상태 제거.
- GameRoom에 Enter 요청.

GameRoom 입장 후:
- `S_ENTER_GAME` 전송.
- AOI 전체 스냅샷 전송.

---

## 6) 세션 관리 구조 (GameSessionManager)

### 6.1 이중 맵 구조

- `_bySessionId` : `sessionId -> Session`
- `_byPlayerId` : `playerId -> Session`
- `_playerIdBySessionId` : `sessionId -> playerId`

장점:
- 로그인 전에는 sessionId만 알고 있음.
- 로그인 후에는 playerId 기반 기능(파티, 귓말 등)에 즉시 접근 가능.
- disconnect 시에도 playerId를 안전하게 찾을 수 있음.

관련 코드: `GameServer/GameSessionManager.cpp`

### 6.2 이름 캐싱과 중복 처리

- `_nameByPlayerId`, `_playerIdByName`, `_ambiguousNames` 구조 사용.
- 같은 이름이 중복되면 `_ambiguousNames`에 넣어 모호성 방지.
- 채팅이나 파티 초대 시 이름 기반 검색이 가능해짐.

관련 코드: `GameServer/GameSessionManager.cpp`

---

## 7) 세션 종료 처리 (Disconnect)

### 7.1 흐름 요약

`PlayerSession::OnDisconnected`에서 수행:

1) `GameSessionManager`에서 세션 제거.
2) 맵 변경 상태 취소.
3) PlayerId 조회 후 **데이터 저장 플래그(DIRTY)** 설정.
4) AutoCommitService에 즉시 저장 요청.
5) GameRoom에서 플레이어 제거.
6) 인스턴스 멤버십 정리.

관련 코드: `GameServer/PlayerSession.cpp`

### 7.2 왜 즉시 저장을 요청하나?

- 서버가 강제 종료되거나 크래시되면 데이터가 날아갈 수 있음.
- Disconnect는 가장 중요한 저장 시점 중 하나.

관련 코드:
- `GameServer/PersistenceService.h`
- `GameServer/AutoCommitService.h`

---

## 8) 맵/채널 이동 FSM (세션 상태 머신)

### 8.1 MapChange FSM 상태

- `MAP_CHANGE_NONE`
- `MAP_CHANGE_WAITING_ACK`
- `MAP_CHANGE_SWITCHING`

세션은 맵 이동 중일 때 다른 요청을 차단합니다.

관련 코드: `GameServer/PlayerSession.h`

### 8.2 MapChange 흐름

1) 클라이언트가 `C_MAP_CHANGE_REQ` 전송.
2) 서버가 토큰 생성 (`MakeMapChangeToken`).
3) `S_MAP_CHANGE_BEGIN` 전송.
4) 클라이언트가 로딩 후 `C_MAP_CHANGE_ACK` 전송.
5) 토큰 검증 후 `TransferMapChangeById` 수행.
6) `S_MAP_CHANGE_END` 전송.
7) FSM을 `NONE`으로 복귀.

관련 코드:
- `GameServer/ClientPacketHandler.MapChange.cpp`
- `GameServer/ClientPacketHandler.MapChangeUtil.cpp`
- `GameServer/PlayerSession.cpp`

### 8.3 왜 토큰이 필요한가?

- 이동 요청 스푸핑 방지.
- 중복/순서 꼬임 방지.
- “서버가 발급한 이동만 인정” 구조.

---

## 9) 초보자를 위한 흐름 요약 다이어그램

### 9.1 로그인 인증 흐름 (LoginServer + DBAgent)

```
Client -> LoginServer : C_LOGIN(userId, password)
LoginServer -> DBAgent : S2S_REQ_LOGIN(name)
DBAgent -> DB : SELECT playerId FROM Players WHERE name = ?
DBAgent -> LoginServer : S2S_RES_LOGIN(success, playerId, playerSessionId)
LoginServer -> Redis : SET token -> playerId (TTL 300)
LoginServer -> Client : S_LOGIN(success, token, serverList)
```

### 9.2 게임 입장 흐름 (GameServer + Lobby + DBAgent)

```
Client -> GameServer : C_ENTER_GAME(token, channelId, mapId)
GameServer -> Redis : GET token
GameServer -> LobbyRoom : EnterGame()
GameServer -> DBAgent : S2S_REQ_LOAD_PLAYER_DATA / ITEMS / QUICKSLOT
DBAgent -> GameServer : S2S_RES_* (with gamesessionid)
GameServer -> LobbyRoom : OnStatLoaded / OnItemsLoaded / OnQuickSlotsLoaded
LobbyRoom -> GameRoom : Enter()
GameRoom -> Client : S_ENTER_GAME + AOI snapshot
```

### 9.3 상세 시퀀스 다이어그램 (스레드/컨텍스트 포함)

아래 다이어그램은 **“어느 스레드/컨텍스트에서 실행되는지”**를 강조합니다.  
실제 코드에서는 네트워크 스레드 → JobQueue → Actor 스레드로 작업이 이동합니다.

#### 9.3.1 로그인 성공 시퀀스 (LoginServer + DBAgent)

```
Client          LoginServer(Net)     DBAgent(Net)      DBAgent(Job)        DB              Redis
  | C_LOGIN ----->|                  |                |                  |                |
  |               | validate userId  |                |                  |                |
  |               | S2S_REQ_LOGIN --->|               |                  |                |
  |               |                  | enqueue job    |                  |                |
  |               |                  |--------------->| Prepare/Execute  |                |
  |               |                  |                |--------------->  |                |
  |               |                  |                |<---------------  |                |
  |               |                  | S2S_RES_LOGIN  |                  |                |
  |               |<-----------------|                |                  |                |
  |               | create token     |                |                  |                |
  |               | SET token->id --------------------------------------->|                |
  | S_LOGIN <-----|                                                     (TTL 300s)         |
```

실패 분기 요약:
- `userId`가 빈 문자열이면 LoginServer에서 즉시 실패.
- DBAgent 연결이 끊겨있으면 LoginServer에서 즉시 실패.
- DB에서 유저가 없으면 `success=false`로 `S_LOGIN` 전송.

#### 9.3.2 게임 입장 시퀀스 (토큰 검증 + 로비 게이트)

```
Client          GameServer(Net)     Redis         PlayerSession(Actor)   LobbyRoom(Actor)   DBAgent(Net/Job)   GameRoom(Actor)
  | C_ENTER_GAME --->|              |             |                     |                  |                 |
  |                 | GET token --->|             |                     |                  |                 |
  |                 |<---- playerId |             |                     |                  |                 |
  |                 | Post(...) ----------------->| BindPlayerId        |                  |                 |
  |                 |                             | SetPendingEnter     |                  |                 |
  |                 |                             | Lobby.EnterGame --->| Create/Reuse     |                 |
  |                 |                             |                     | flags=false      |                 |
  |                 | S2S_REQ_* -------------------------------> DBAgent | DB load jobs     |                 |
  |                 |<------------------------------ S2S_RES_* (gamesessionid)               |                 |
  |                 | route by sessionId + pendingChannelId --->| OnStat/Items/QS             |
  |                 |                                             flags true if done         |
  |                 |                                             TryEnterWorldIfReady ----->| Enter()
  |                 |                                                                         | S_ENTER_GAME
  |<----------------|                                                                         | AOI snapshot
```

핵심 포인트:
- **Redis 토큰 검증 실패 시 즉시 Disconnect** (보안 게이트).
- **Pending Enter**는 DB 응답을 올바른 로비로 보내기 위한 라우팅 키.
- LobbyRoom은 **데이터가 모두 준비될 때까지** 월드 입장을 막는다.

#### 9.3.3 맵/채널 이동 시퀀스 (FSM + 토큰 검증)

```
Client           GameServer(Net)    PlayerSession(Actor)     GameRoom(Actor)
  | C_MAP_CHANGE_REQ -->|           |                       |
  |                    | Post(...) ->| TryBeginMapChange     |
  |                    |            | Send S_MAP_CHANGE_BEGIN
  |<-------------------|            |                       |
  | (load scene)       |            |                       |
  | C_MAP_CHANGE_ACK -->|           |                       |
  |                    | Post(...) ->| TryConsumeMapChangeAck
  |                    |            | Push to room -------->| TransferMapChangeById
  |                    |            |                       | Send S_MAP_CHANGE_END
  |<-------------------|            |                       |
```

핵심 포인트:
- `TryBeginMapChange`가 성공해야만 이동이 시작됨.
- `C_MAP_CHANGE_ACK`는 **토큰이 일치**할 때만 처리.
- 이동 중에는 중복 요청을 무시하여 상태 꼬임을 방지.

#### 9.3.4 실패/예외 분기 다이어그램 (요청 거절/무시/끊김)

아래는 정상 시퀀스가 아니라 **실패 분기**만 모아서 요약한 다이어그램입니다.  
실제 코드에서 “어떤 경우에 응답이 없거나, 바로 끊기는지”를 초보자가 이해하기 쉽게 분리했습니다.

##### (A) 로그인 실패 분기

```
Client -> LoginServer : C_LOGIN
  |-- userId empty ----------------> [Handle_C_LOGIN returns false] (응답 없음 가능)
  |-- DBAgent 연결 없음 -----------> [Handle_C_LOGIN returns false] (응답 없음 가능)
  |-- DBAgent DB 조회 실패 --------> S2S_RES_LOGIN(success=false)
  |                                  LoginServer -> Client : S_LOGIN(success=false)
```

설명:
- `userId`가 비어 있거나 DBAgent가 끊긴 경우, **현재 구현은 실패 응답을 보내지 않고 핸들러가 false를 반환**합니다.
- DB에서 유저를 찾지 못하면 `S_LOGIN(success=false)`가 전송됩니다.

##### (B) 게임 입장 실패 분기

```
Client -> GameServer : C_ENTER_GAME(token)
  |-- token 없음/만료 -------------> GameServer : Disconnect("Invalid Token")
  |-- DB 로드 실패 ----------------> TODO 처리 (현재는 실패 응답 없음)
```

설명:
- Redis에 토큰이 없으면 즉시 `Disconnect`가 호출됩니다.
- DB 로드 실패는 `S2SPacketHandler`에 TODO만 있어 **현재는 클라 실패 응답이 미구현**입니다.

##### (C) 맵/채널 이동 실패 분기

```
Client -> GameServer : C_MAP_CHANGE_REQ
  |-- 이미 MapChanging --------> 요청 무시

Client -> GameServer : C_MAP_CHANGE_ACK(token)
  |-- token mismatch ----------> ACK 무시 (상태 유지 가능)
  |-- player/room 없음 --------> CancelMapChange()
```

설명:
- 이동 중 추가 요청은 무시되어 **중복 이동으로 상태 꼬임**을 막습니다.
- 잘못된 토큰 ACK는 무시되어 **맵 변경 상태가 유지될 수 있음**(현재 타임아웃 없음).
- 현재 룸이 없거나 플레이어 ID가 없으면 `CancelMapChange()`로 상태를 해제합니다.

### 9.4 실제 로그 예시 (실제 코드 문자열 기반)

로그는 **빌드 옵션/로그 레벨/접속 순서**에 따라 다를 수 있습니다.  
아래는 “코드에 실제로 존재하는 printf/cout 문자열”을 기준으로 구성한 예시입니다.

#### 9.4.1 로그인 성공 예시

```
 [Login] Request DB Verification for: Alice
 [DB] Login Success! Name: Alice ID: 1001
 [DEBUG] Handle_S2S_RES_LOGIN Called. Success: 1
 🔑 [Redis] Token Saved: Token_1001_1700000000_1234 -> ID: 1001
 [Login] Success! User: 1001
```

#### 9.4.2 로그인 실패 예시 (유저 없음)

```
 [Login] Request DB Verification for: GhostUser
 [DB] User Not Found: GhostUser
 [Login] Failed (Invalid ID/PW)
```

#### 9.4.3 토큰 검증 실패 예시 (GameServer)

```
 [EnterGame] Invalid Token: Token_9999_1700000000_7777
```

#### 9.4.4 맵 이동 완료 예시 (GameRoom)

```
 [MapChange-END] Player 1001 -> Map 2 (Inst=0) Token=123456789 Channel=1
```

---

## 10) 현재 구현 상태와 제한 사항

### 10.1 현재 구현됨

- ID 기반 로그인 검증 (비밀번호 미사용)
- Redis 토큰 발급/검증
- 토큰 기반 GameServer 입장
- 로비 게이트(데이터 로딩 후 월드 입장)
- 세션 바인딩/해제
- 맵/채널 이동 FSM

### 10.2 미구현 / TODO

- 비밀번호 검증 로직 없음
- 신규 계정 생성 로직 없음
- DB 실패 시 클라이언트 응답 처리 미완
- 서버 리스트는 하드코딩

코드 근거:
- `LoginServer/ClientPacketHandler.cpp` (password 미사용)
- `DBAgent/DBAgentPacketHandler.cpp` (CreateAccount TODO)
- `LoginServer/S2SPacketHandler.cpp` (server list 하드코딩)
- `GameServer/S2SPacketHandler.cpp` (실패 처리 TODO)

---

## 11) 초보자를 위한 "왜 이렇게 설계했는가" 정리

### 11.1 인증과 게임 로직을 분리하는 이유

- 로그인은 보안/계정 검증이 핵심이고,
- 게임 서버는 실시간 로직 처리에 집중해야 합니다.
- 둘을 분리하면 **확장성 + 보안 + 성능**이 좋아집니다.

### 11.2 토큰 캐시의 이유

- DB에서 매번 유효성 검사하면 느림.
- 토큰은 **한 번 발급하고 Redis에서 빠르게 검증**하는 방식이 이상적.

### 11.3 로비 게이트의 이유

- 게임 입장 전에 데이터가 로딩되지 않으면
  - “빈 인벤토리”, “스탯 0”, “퀵슬롯 없음” 같은 버그 발생.
- 로비는 데이터를 모두 모으는 **동기화 버퍼** 역할.

### 11.4 세션 매니저의 이유

- “네트워크 세션”과 “게임 플레이어”는 별개입니다.
- 따라서 `sessionId ↔ playerId`를 명확히 바인딩해야
  - 채팅, 파티, 이동, 저장이 안정적으로 동작합니다.

---

## 12) 코드 탐색 가이드 (빠른 점프용)

### LoginServer
- `LoginServer/ClientPacketHandler.cpp` : C_LOGIN 처리
- `LoginServer/S2SPacketHandler.cpp` : 토큰 발급 + Redis 저장
- `LoginServer/LoginSessionManager.cpp` : 클라 세션 관리
- `LoginServer/LoginServer.cpp` : 서비스/포트 구성

### DBAgent
- `DBAgent/DBAgentPacketHandler.cpp` : 로그인 DB 조회
- `DBAgent/GameSession.h/.cpp` : JobQueue를 통한 DB 처리

### GameServer
- `GameServer/ClientPacketHandler.EnterGame.cpp` : token 검증 + DB 로딩 요청
- `GameServer/S2SPacketHandler.cpp` : DB 응답 처리
- `GameServer/LobbyRoom.cpp` : 로비 게이트 + 데이터 적용
- `GameServer/GameRoom.EnterLeave.cpp` : S_ENTER_GAME 전송
- `GameServer/PlayerSession.h/.cpp` : 세션 상태, 맵 변경 FSM
- `GameServer/GameSessionManager.cpp` : 세션 바인딩 관리

### Core
- `ServerCore/RedisManager.h` : Redis 키-밸류/TTL 사용
- `ServerCore/JobQueue.h` : JobQueue 설계
- `Common/Protobuf/bin/Protocol.proto` : 패킷 정의

---

## 13) 마무리 요약 (핵심만 6줄)

1) 로그인은 LoginServer가 처리하고, DB 검증은 DBAgent가 수행한다.
2) 로그인 성공 시 토큰을 Redis에 저장하고, 클라이언트에 `S_LOGIN`을 보낸다.
3) GameServer는 `C_ENTER_GAME`에서 토큰을 검증한 뒤 playerId를 세션에 바인딩한다.
4) 로비에서 스탯/인벤/퀵슬롯을 모두 로드해야 월드로 입장한다.
5) 세션 매니저는 sessionId와 playerId를 양방향으로 관리한다.
6) Disconnect 시점에 데이터 저장을 즉시 요청하여 안전성을 확보한다.
