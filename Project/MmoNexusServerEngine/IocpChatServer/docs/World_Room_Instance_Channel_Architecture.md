# World/Room/Instance/Channel Architecture and Flow

이 문서는 월드/룸/인스턴스/채널 구조를 **신입 서버 프로그래머가 바로 이해할 수 있도록** 아주 자세하게 설명합니다.  
특히 “룸이 무엇을 소유하는지”, “어떤 액터가 어떤 책임을 가지는지”, “어떤 흐름으로 이동/입장/정리가 되는지”를 코드 근거 중심으로 정리했습니다.

> 제목만 영어, 본문은 한국어로 작성합니다.

---

## 0) 문서 범위와 목표

- **대상 기능**: `docs/Portfolio_FeatureList.md`의 2) 월드/룸/인스턴스/채널 구조 파트.
- **핵심 목표**:
  - “룸/로비/인스턴스/채널이 실제로 어떻게 나뉘는지” 이해하기
  - “누가 무엇을 소유/관리하는지”를 명확하게 서술
  - “이동/입장/정리 흐름”을 단계별로 풀어서 설명
- **근거 코드 범위**:
  - `GameServer/RoomManager.*`, `GameServer/GameRoom.*`, `GameServer/LobbyRoom.*`
  - `GameServer/InstanceActor.*`, `GameServer/InstanceManagerCore.*`
  - `GameServer/ClientPacketHandler.MapChange*.cpp`, `GameServer/ClientPacketHandler.Dungeon.cpp`
  - `GameServer/DataManager.*`, `GameServer/GameMap.*`, `GameServer/SpatialGrid.*`, `GameServer/Zone.h`

---

## 1) 전체 구조 한눈에 보기

### 1.1 핵심 개념 요약

- **Channel(채널)**: 같은 월드 맵을 여러 개로 “복제”한 논리 구분자.
- **Room(게임 룸)**: 실제 게임 공간. 채널 + 맵 + 인스턴스 조합으로 유일.
- **Lobby(로비)**: 월드 입장 전에 데이터 준비를 위한 대기 공간. 채널당 1개.
- **Instance(인스턴스 던전)**: 파티 전용 공간. `instanceId > 0`인 GameRoom.

### 1.2 RoomKey로 보는 구조

RoomManager는 다음 키로 방을 관리합니다:

```
RoomKey = { channelId, mapId, instanceId }
```

- **channelId**: 같은 맵이라도 채널이 다르면 다른 룸
- **mapId**: 같은 채널이라도 맵이 다르면 다른 룸
- **instanceId**:
  - `0`이면 일반 월드
  - `>0`이면 인스턴스 던전

관련 코드: `GameServer/RoomManager.h`

간단 코드 스니펫:

```cpp
struct RoomKey
{
    int32 channelId;
    int32 mapId;
    int64 instanceId = 0;
};
```

왜 중요한가: **RoomKey가 곧 “룸의 주소 체계”**이기 때문에, 채널/맵/인스턴스의 개념을 한 번에 이해할 수 있습니다.

---

## 2) 초보자를 위한 용어 설명

### 2.1 RoomActor (액터 기반 룸)

- `RoomActor`는 **룸 계열 객체가 공통으로 가지는 인터페이스**입니다.
- 핵심 메서드는 `Push()`이며, 외부 요청을 **JobQueue로 밀어넣는 역할**을 합니다.
- 이런 구조 덕분에 룸 내부 데이터는 한 스레드에서만 안전하게 변경됩니다.

관련 코드: `GameServer/RoomActor.h`

간단 코드 스니펫:

```cpp
enum class RoomKind : uint8_t { Lobby = 0, Game = 1 };

class RoomActor
{
public:
    virtual RoomKind GetKind() const = 0;
    virtual void Push(std::function<void()> fn) = 0;
};
```

왜 중요한가: RoomActor는 **“룸은 반드시 JobQueue로만 건드린다”**는 규칙을 코드 레벨에서 강제합니다.

### 2.2 LobbyRoom vs GameRoom

- **LobbyRoom**: 데이터 로딩이 끝날 때까지 대기시키는 논리 공간.
- **GameRoom**: 실제 월드/던전 공간. 좌표, 몬스터, 플레이어가 존재.

둘 다 `RoomActor`를 상속하고, 내부에 JobQueue를 가지고 있습니다.

### 2.3 InstanceActor / InstanceManagerCore

- **InstanceActor**: 인스턴스 던전 관련 작업을 **순차 처리**하는 액터.
- **InstanceManagerCore**: 실제 데이터(인스턴스 목록, 멤버)를 관리하는 순수 코어.

Actor는 “스레드 안전한 실행 흐름”을 담당하고, Core는 “데이터 구조”를 담당합니다.

관련 코드: `GameServer/InstanceActor.h`, `GameServer/InstanceManagerCore.*`

---

## 3) 소유 관계(Ownership) 정리 (중요)

아래는 “무엇이 무엇을 소유하는지”를 명시적으로 정리한 부분입니다.

### 3.1 RoomManager가 소유하는 것

- `Map<RoomKey, GameRoom>`: 모든 게임 룸 (월드 + 인스턴스)
- `HashMap<channelId, LobbyRoom>`: 채널별 로비

즉, **RoomManager는 모든 룸의 “레지스트리(목록)” 역할**을 합니다.

관련 코드: `GameServer/RoomManager.h/.cpp`

간단 코드 스니펫:

```cpp
Map<RoomKey, std::shared_ptr<GameRoom>> _rooms;
HashMap<int32, std::shared_ptr<LobbyRoom>> _lobbies;
```

왜 중요한가: RoomManager는 **모든 룸의 소유자(레지스트리)**이고, 룸 생성/조회는 반드시 여기서만 일어납니다.

### 3.2 LobbyRoom이 소유하는 것

- `_players`: 로딩 대기 중인 플레이어 슬롯
- 각 슬롯은 `player`, `itemsLoaded`, `statLoaded`, `quickLoaded` 상태를 가짐

즉, **LobbyRoom은 “입장 준비 중인 플레이어”의 소유자**입니다.

관련 코드: `GameServer/LobbyRoom.h/.cpp`

간단 코드 스니펫:

```cpp
struct Pending
{
    PlayerRef player;
    bool itemsLoaded = false;
    bool statLoaded = false;
    bool quickLoaded = false;
};
HashMap<uint64, Pending> _players;
```

왜 중요한가: 로비는 **플레이어 소유권 + 로딩 상태**를 함께 들고 있어, “로딩 완료 전 입장 차단”을 보장합니다.

### 3.3 GameRoom이 소유하는 것

- `_players`: 현재 방에 있는 플레이어 맵
- `_monsters`: 현재 방에 있는 몬스터 맵
- `_grid`: AOI(시야 처리)용 SpatialGrid
- `_map`: GameMap (네비메쉬, 맵 크기)
- `_battle`: 전투 계산 시스템
- `_jobQueue`: 방 내부 로직 처리 큐

즉, **GameRoom은 “실제 게임 공간의 모든 객체와 로직”을 소유**합니다.

관련 코드: `GameServer/GameRoom.h`, `GameServer/GameRoom.LifeTime.cpp`

간단 코드 스니펫:

```cpp
Map<uint64, PlayerRef>  _players;
Map<uint64, MonsterRef> _monsters;
SpatialGrid             _grid;
GameMapRef              _map;
std::unique_ptr<BattleSystem> _battle;
```

왜 중요한가: GameRoom이 **실제 게임 공간의 모든 객체와 시스템을 소유**한다는 것을 명확히 보여줍니다.

### 3.4 PlayerSession이 소유/참조하는 것

- `currentRoom`: 현재 플레이어가 소속된 RoomActor (weak_ptr)
- `mapChangeState`: 맵 이동 FSM 상태
- `pendingEnter`: 로비 게이트를 위한 임시 컨텍스트

즉, **세션은 “플레이어가 어떤 룸에 있는지”를 기억하지만, 룸을 소유하지 않습니다.**

관련 코드: `GameServer/PlayerSession.h/.cpp`

간단 코드 스니펫:

```cpp
std::weak_ptr<RoomActor> _currentRoom;
std::atomic<bool>  _pendingEnterActive{ false };
std::atomic<int32> _pendingEnterChannelId{ 0 };
```

왜 중요한가: 세션은 “룸을 소유하지 않고 참조”하며, 라우팅/입장 컨텍스트만 기억한다는 점이 드러납니다.

### 3.5 InstanceActor / InstanceManagerCore

- InstanceActor는 JobQueue + InstanceManagerCore를 소유.
- InstanceManagerCore는 인스턴스 데이터(파티/멤버/instanceId)를 소유.

즉, **인스턴스 던전의 “논리 상태”는 InstanceManagerCore가 소유**합니다.

관련 코드: `GameServer/InstanceActor.h`, `GameServer/InstanceManagerCore.*`

간단 코드 스니펫:

```cpp
HashMap<uint64, int64> _partyToInstance;
HashMap<int64, InstanceInfo> _instances;
HashMap<uint64, int64> _playerToInstance;
```

왜 중요한가: 인스턴스 코어는 **파티/인스턴스/플레이어 역인덱스**로 빠른 조회를 보장합니다.

---

## 4) 채널/로비/게임룸 구조 상세

### 4.1 채널 구조

- 채널은 **RoomKey의 첫 번째 축**입니다.
- 같은 mapId라도 channelId가 다르면 완전히 다른 GameRoom.
- LobbyRoom도 채널당 1개씩 존재.

관련 코드:
- `RoomKey{ channelId, mapId, instanceId }`
- `RoomManager::GetOrCreateLobby(channelId)`

### 4.2 로비 구조 (채널당 1개)

- 로비는 “입장 게이트” 역할.
- 플레이어는 로그인 직후 로비에서 대기.
- **DB 로딩 완료 후에만 월드 룸으로 이동**.

관련 코드:
- `LobbyRoom::EnterGame`, `TryEnterWorldIfReady`
- `GameServer/ClientPacketHandler.EnterGame.cpp`

간단 코드 스니펫:

```cpp
return (it->second.player != nullptr)
    && it->second.itemsLoaded
    && it->second.statLoaded
    && it->second.quickLoaded;
```

왜 중요한가: “모든 로딩 완료 전 입장 불가”라는 로비 게이트의 핵심 조건이 이 한 줄에 있습니다.

### 4.3 게임 룸 구조 (맵 단위)

- `RoomManager::GetOrCreateRoom`이 필요할 때 생성.
- `DataManager::IsValidMapId`로 유효성 검사 후, 잘못된 맵이면 기본 맵으로 보정.
- 맵 크기/네비메쉬/관심 반경 등은 `DataManager`의 `MapConfig`로 초기화.
- 설정 파일이 없을 경우 기본값으로 fallback 초기화.
- 내부에는 Grid, Player/Monster 맵이 존재.

관련 코드:
- `GameRoom::Init`, `GameRoom::Update`
- `GameServer/DataManager.h`

간단 코드 스니펫:

```cpp
_grid.Init(0, 0, config->sizeX, config->sizeY, config->aoiCellSize);
_interestRadius = config->interestRadius;
```

왜 중요한가: 맵 설정값이 **AOI 성능과 시야 반경**에 직접 영향을 준다는 것을 보여줍니다.

---

## 5) 맵/채널 이동 핸드셰이크

### 5.1 기본 개념

맵 이동은 “플레이어를 현재 방에서 빼고, 다른 방으로 안전하게 옮기는 작업”입니다.  
이를 위해 **서버가 토큰을 발급**하고, 클라이언트의 ACK를 받아야만 실제 이동이 이루어집니다.

### 5.2 흐름 요약

1) 클라이언트가 `C_MAP_CHANGE_REQ` 전송.
2) 서버가 토큰 생성 → `S_MAP_CHANGE_BEGIN` 발송.
3) 클라이언트가 씬 로딩 완료 후 `C_MAP_CHANGE_ACK` 전송.
4) 서버가 토큰 검증 후 `TransferMapChangeById` 수행.

관련 코드:
- `GameServer/ClientPacketHandler.MapChange.cpp`
- `GameServer/GameRoom.MapChange.cpp`

간단 코드 스니펫:

```cpp
const uint64 token = MapChangeUtil::MakeMapChangeToken(playerId, self->GetSessionId());
if (!self->TryBeginMapChange(token, targetChannelId, targetMapId, targetInstanceId, spawn))
    return;
```

왜 중요한가: 맵 이동은 **토큰 + FSM**으로 보호되어 위조/중복 이동을 방지합니다.

### 5.3 TransferMapChangeById 내부 구조 (핵심)

`TransferMapChangeById`는 여러 액터를 순서대로 거치는 **Job Chain** 구조입니다.

1) **현재 GameRoom**: 플레이어 제거(Leave)
2) **LobbyRoom**: 임시 소유권 이전(Adopt)
3) **새 GameRoom**: 실제 입장(EnterMapChange)
4) **Session Actor**: MapChange 상태 해제
5) **LobbyRoom**: 최종 detach

관련 코드: `GameServer/GameRoom.MapChange.cpp`

이렇게 중간에 로비를 거치는 이유는 **"룸 소유권 안전 전환"** 때문입니다.

간단 코드 스니펫:

```cpp
lobby->Push([lobby, newRoom, session, player, pid]() mutable
{
    lobby->Adopt(player, true);
    newRoom->Push([newRoom, lobby, session, player, pid]() mutable
    {
        newRoom->EnterMapChange(session, player);
        session->Post([newRoom](PlayerSessionRef s)
        {
            s->SetCurrentRoom(newRoom);
            s->EndMapChange();
        });
        lobby->Push([lobby, pid]() { lobby->Detach(pid); });
    });
});
```

왜 중요한가: 룸 소유권이 **GameRoom → Lobby → NewRoom** 순서로 안전하게 이동함을 보여주는 핵심 코드입니다.

### 5.4 채널 이동도 같은 FSM을 사용

- 채널 이동은 `C_CHANNEL_CHANGE_REQ`로 들어오며, 맵은 그대로 두고 **채널만 바꾸는 맵 이동**입니다.
- 내부적으로는 **맵 이동 FSM과 동일한 토큰/ACK 구조**를 사용합니다.
- 결국 `TransferMapChangeById`로 들어가며, 룸 키가 `channelId`만 다른 새로운 방으로 이동합니다.

관련 코드:
- `GameServer/ClientPacketHandler.MapChange.cpp` (`Handle_C_CHANNEL_CHANGE_REQ`)

---

## 6) 인스턴스 던전 구조 (파티 기반)

### 6.1 핵심 요약

- 인스턴스 던전은 **파티 단위**로만 생성.
- 인스턴스는 `instanceId > 0`인 GameRoom.
- InstanceActor가 생성/종료를 담당.

### 6.2 던전 입장 흐름

1) `C_DUNGEON_ENTER_REQ` 수신.
2) 던전 맵 ID 유효성 검사 (`DataManager::IsDungeonMapId`).
3) PartyActor에서 파티 상태 검사.
4) InstanceActor에서 `CreateOrGetForParty`로 인스턴스 생성.
5) 파티원 모두에게 맵 이동 시작 패킷 전송.
6) 실제 인스턴스 룸은 **맵 이동 시점에 `GetOrCreateRoom`으로 생성**됨.

관련 코드: `GameServer/ClientPacketHandler.Dungeon.cpp`

간단 코드 스니펫:

```cpp
InstanceActor::Instance().Core().CreateOrGetForParty(
    partyId, channelId, dungeonMapId, members, inst);
```

왜 중요한가: 인스턴스는 **파티 단위로만 생성/조회**되고, 생성 로직은 InstanceActor가 단일화합니다.

### 6.3 던전 퇴장 흐름

1) `C_DUNGEON_EXIT_REQ` 수신.
2) 파티 상태 확인.
3) InstanceActor에서 `CloseForParty` 수행.
4) 인스턴스 룸에 `MarkClosing(true)`.
5) 파티원 전원 복귀 처리.

관련 코드: `GameServer/ClientPacketHandler.Dungeon.cpp`

### 6.4 인스턴스 데이터 구조

InstanceManagerCore가 관리하는 주요 필드:

- `instanceId`: 유니크 인스턴스 ID (시간+시퀀스 조합)
- `channelId`, `mapId`: 어느 채널/맵의 인스턴스인지
- `partyId`: 어느 파티 소속인지
- `members`: 인스턴스 안에 있는 플레이어 목록
- `createdTick`: 생성 시각 (타임아웃 체크)

관련 코드: `GameServer/InstanceManagerCore.h/.cpp`

---

## 7) 룸 수명 주기와 자동 정리

### 7.1 Update 호출 흐름

- GameServer 메인 루프에서 `RoomManager::UpdateAll()` 호출.
- 모든 GameRoom에 `Update()` Job을 Push.

관련 코드: `GameServer/GameServer.cpp`, `RoomManager::UpdateAll`

### 7.2 인스턴스 룸 정리 흐름

1) InstanceActor가 인스턴스 종료 시 `room->MarkClosing(true)`.
2) RoomManager가 주기적으로 `ShouldPurge()` 체크.
3) 빈 인스턴스 룸 + Closing 상태 + 유예시간 경과 → 삭제.

관련 코드:
- `GameRoom::ShouldPurge` (`GameRoom.LifeTime.cpp`)
- `RoomManager::PurgeInstanceRooms`

간단 코드 스니펫:

```cpp
if (!IsInstanceRoom()) return false;
if (!IsClosing()) return false;
if (GetPlayerCountApprox() != 0) return false;
```

왜 중요한가: 인스턴스 룸 정리는 **“인스턴스 + Closing + 비어있음”** 조건이 모두 충족될 때만 가능합니다.

유예시간 정책:
- 플레이어가 나간 직후 바로 삭제하지 않고 **10초 대기**.
- 네트워크 지연/재접속 같은 상황을 고려한 안정 장치.

---

## 8) 구성 요소별 상세 설명 (액터/기능)

### 8.1 RoomManager

- 모든 Room을 찾고/생성하는 **레지스트리**.
- 룸 키 기준으로 룸을 만든다.
- UpdateAll에서 룸 업데이트 + 인스턴스 정리 수행.

### 8.2 LobbyRoom

- 플레이어의 “입장 전 대기 공간”.
- DB 로딩 플래그를 관리.
- 준비 완료 시 월드 룸으로 이동시킨다.

### 8.3 GameRoom

- 실제 게임 플레이 공간.
- 플레이어/몬스터/투사체가 존재.
- Grid(AOI), 전투 계산, 이동 검증 등을 담당.

### 8.4 InstanceActor / InstanceManagerCore

- 인스턴스 생성/종료/타임아웃을 중앙에서 처리.
- 파티 단위 인스턴스 매핑 유지.

### 8.5 PartyActor / PartyManagerCore

- 파티 관리 및 던전 입장 상태 트랜잭션 담당.
- `DungeonState`로 ENTERING/IN_DUNGEON/EXITING 구분.

관련 코드: `GameServer/PartyActor.h`, `GameServer/PartyManagerCore.h`

---

## 9) 시퀀스 다이어그램 (상세)

### 9.1 채널/룸 생성 흐름

```
Client -> GameServer : C_ENTER_GAME
GameServer -> RoomManager : GetOrCreateLobby(channelId)
GameServer -> RoomManager : GetOrCreateRoom(channelId, mapId, instanceId=0)
RoomManager -> GameRoom : Init(map config)
```

#### 9.1.1 로비 게이트 상세 시퀀스

표기 규칙: `Net`은 IO 스레드, `Actor/JobQueue`는 해당 객체 전용 작업 큐에서 순차 실행됨.

```
Client            GameServer(Net/IO)   PlayerSession(Actor/JobQueue)  LobbyRoom(Actor/JobQueue)   DBAgent(Net/JobQueue)   GameRoom(Actor/JobQueue)
  | C_ENTER_GAME -->|               |                    |                  |                  |
  |                 | Redis GET     |                    |                  |                  |
  |                 | Post(...) ---->| BindPlayerId       |                  |                  |
  |                 |                | SetPendingEnter    |                  |                  |
  |                 |                | Lobby.EnterGame -->| Create/Reuse     |                  |
  |                 |                |                    | flags=false      |                  |
  |                 | S2S_REQ_* ------------------------------------------->| DB load jobs     |
  |                 |<------------------------------------- S2S_RES_*       |                  |
  |                 | route by sessionId/pendingChannelId ->| OnStat/Items/QS |
  |                 |                                         flags true     |
  |                 |                                         TryEnterWorld  |
  |                 |------------------------------------------------------>| Enter()
  |<------------------------------------------------------ S_ENTER_GAME + AOI
```

포인트:
- **로비는 “모든 로딩이 끝날 때까지” 플레이어를 소유**한다.
- 모든 로딩 플래그가 true가 되어야만 GameRoom 입장 가능.

### 9.2 맵 이동 핸드셰이크 흐름

```
Client -> GameServer : C_MAP_CHANGE_REQ
GameServer -> Client : S_MAP_CHANGE_BEGIN(token)
Client -> GameServer : C_MAP_CHANGE_ACK(token)
GameRoom -> LobbyRoom -> NewRoom : TransferMapChangeById (Job Chain)
GameServer -> Client : S_MAP_CHANGE_END
```

#### 9.2.1 맵 이동 Job Chain 상세

표기 규칙: `Net`은 IO 스레드, `Actor/JobQueue`는 해당 객체 전용 작업 큐에서 순차 실행됨.

```
Client             GameServer(Net/IO)   PlayerSession(Actor/JobQueue)   GameRoom(Actor/JobQueue)   LobbyRoom(Actor/JobQueue)   NewRoom(Actor/JobQueue)
  | C_MAP_CHANGE_REQ -->|            |                      |                |                  |
  |                      | Post(...) ->| TryBeginMapChange   |                |                  |
  |<---------------------| S_MAP_CHANGE_BEGIN                |                |                  |
  | (load scene)         |                                     |              |                  |
  | C_MAP_CHANGE_ACK --->|            |                      |                |                  |
  |                      | Post(...) ->| TryConsumeAck        |                |                  |
  |                      |------------>| TransferMapChangeById |-------------->| Adopt(player)
  |                      |             |                      |                |---> EnterMapChange
  |                      |             |                      |                |<--- session EndMapChange
  |<---------------------| S_MAP_CHANGE_END                   |                |                  |
```

포인트:
- **GameRoom -> LobbyRoom -> NewRoom**으로 소유권이 순차 이동.
- 세션의 `currentRoom`은 이동 단계마다 안전하게 갱신됨.

### 9.3 던전 인스턴스 생성 흐름 (파티)

표기 규칙: `Actor/JobQueue`는 해당 액터의 작업 큐에서 순차 실행됨.

```
Client -> GameServer(Net/IO) : C_DUNGEON_ENTER_REQ
GameServer -> PartyActor(Actor/JobQueue) : 파티 상태 검증
PartyActor -> InstanceActor(Actor/JobQueue) : CreateOrGetForParty
GameServer -> PartyMembers : S_DUNGEON_ENTER_RES + MapChangeBegin
PartyMembers -> GameServer(Net/IO) : C_MAP_CHANGE_ACK
GameRoom(Actor/JobQueue) -> RoomManager : GetOrCreateRoom(instanceId>0) (이 시점에 실제 룸 생성)
```

### 9.4 실패/예외 분기 요약

- **채널 이동 중 중복 요청**: 무시
- **인스턴스 종료 중 재입장**: party 상태 검사에서 실패
- **MapChange 토큰 불일치**: 이동 취소 혹은 무시

### 9.5 실제 로그 예시 (코드 기반)

아래는 코드에 실제로 존재하는 문자열을 기반으로 구성한 예시입니다.

#### 9.5.1 GameRoom 초기화 로그

```
 [GameRoom] Init with Config - MapId: 1, Size: (200, 200), Cell: 64, Nav: Load
```

#### 9.5.2 인스턴스 룸 정리 로그

```
[RoomManager] Purged instance room: ch=1 map=2001 inst=123456789
```

#### 9.5.3 로비 중복 입장 방어 로그

```
 [Lobby] EnterGame blocked - MapChanging in progress: 1001
```

---

## 10) 현재 구현 상태와 TODO

- 채널/로비/게임룸 구조: **완료**
- 맵/채널 이동 핸드셰이크: **완료**
- 파티 기반 인스턴스 던전: **부분 구현**
- 인스턴스 자동 정리: **완료**

TODO/제한 사항:
- 강제 귀환/강퇴 처리 일부 TODO (던전 관련)
- 던전 실패 처리 및 예외 응답 더 필요

---

## 11) 왜 이렇게 설계했는가 (설계 이유)

### 11.1 채널 분리 이유

- 같은 맵을 여러 채널로 분산하여 **접속 분산** 가능.
- 구조적으로는 “같은 맵을 복제한 다른 방”이므로 구현이 단순.

### 11.2 로비 게이트의 이유

- 데이터가 다 로딩되기 전에 월드 입장하면 오류 발생.
- 로비가 “동기화 버퍼” 역할을 수행.

### 11.3 인스턴스 액터 분리 이유

- 인스턴스 생성/종료는 여러 스레드에서 동시에 요청될 수 있음.
- Actor 구조로 묶으면 **경쟁 상태 방지**.

---

## 12) 코드 탐색 가이드 (빠른 점프)

- `GameServer/RoomManager.*` : Room 레지스트리
- `GameServer/LobbyRoom.*` : 로비 게이트
- `GameServer/GameRoom.*` : 월드/인스턴스 룸
- `GameServer/GameRoom.MapChange.cpp` : 맵 이동 Job Chain
- `GameServer/InstanceActor.*` : 인스턴스 액터
- `GameServer/InstanceManagerCore.*` : 인스턴스 데이터 구조
- `GameServer/ClientPacketHandler.Dungeon.cpp` : 던전 입장/퇴장
- `GameServer/DataManager.*` : 맵 설정/타입

---

## 13) 마무리 요약 (6줄)

1) Room은 `channelId + mapId + instanceId`로 구분된다.
2) LobbyRoom은 입장 전 플레이어를 소유하고, 준비가 되면 GameRoom으로 이동시킨다.
3) GameRoom은 플레이어/몬스터/그리드/전투 시스템을 소유한다.
4) 맵 이동은 토큰 기반 핸드셰이크와 Job Chain으로 안전하게 처리된다.
5) 인스턴스 던전은 파티 단위로 생성되며 InstanceActor가 관리한다.
6) 인스턴스 룸은 Closing + 빈 룸 + 유예시간 조건을 만족하면 자동 정리된다.
