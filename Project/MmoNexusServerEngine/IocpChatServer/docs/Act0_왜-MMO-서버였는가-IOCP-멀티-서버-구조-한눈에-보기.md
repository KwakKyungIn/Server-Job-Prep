# Act 0. 왜 MMO 서버였는가 — IOCP 멀티 서버 구조 한눈에 보기
_IOCP MMO 서버 제작기 — 권위·동시성·정합성 기록_

## 핵심 요약
- 제가 MMO 서버를 선택한 이유는 “동시성, 서버 권위, 정합성”을 한 번에 다뤄볼 수 있기 때문이었습니다.
- 이 프로젝트는 Login / Game / DBAgent를 분리하고, IOCP와 JobQueue/RoomActor로 네트워크와 로직의 경계를 명확히 나눴습니다.
- 로그인, 이동/AOI, 저장/커밋에 더해 파티/거래 같은 게임 기능도 같은 원칙(액터 직렬화 + 안전한 커밋 경계)으로 연결했습니다.

---

## 1. 왜 하필 MMO 서버였나
저는 서버를 공부할 때 “기술 이름을 아는 것”보다 “어디에서 망가지고, 어떻게 버티게 할지”를 직접 겪어보는 편이 더 오래 남는다고 느꼈습니다. MMO 서버는 그 점에서 좋은 연습장이었습니다.

제가 MMO 서버를 선택한 이유는 아래 세 가지를 한 시스템 안에서 동시에 다뤄야 하기 때문입니다.

- 동시성: 접속 수와 이벤트가 늘어나면, 네트워크 처리와 로직 처리를 분리하지 않으면 금방 병목이 생깁니다.
- 서버 권위: 이동이나 전투처럼 치팅 여지가 있는 입력은 서버가 최종 판단을 내려야 안정적입니다.
- 정합성: 아이템, 인벤토리, 거래처럼 상태가 섞이는 구간은 “언제 느슨하게 처리하고, 언제 단단하게 묶을지”가 중요합니다.

그래서 이번 프로젝트의 목표는 “기능을 많이 넣는 것”보다는, 위 세 가지를 피하지 않고 끝까지 연결해보는 것이었습니다.

---

## 2. 프로젝트 한눈에 보기
아래 그림은 현재 서버 구성을 한 장으로 요약한 것입니다. 핵심은 역할을 나누고, 경계를 분명히 하는 것입니다.

```text
[ Client ]
   │
   │ login / enter / move / skill / party / trade
   ▼
[ LoginServer ]  ──S2S──>  [ DBAgent ]  ──>  [ SQL DB ]
      │                          │
      └─────────────── Redis ────┘
                     (token / dirty)
                        │
                        ▼
                   [ GameServer ]
                      │
                      │ IOCP Dispatch
                      ▼
                 JobQueue / GlobalQueue
                      ▼
             RoomActor(Lobby/Game)에서 직렬 처리
```

각 서버의 역할을 짧게 정리하면 아래와 같습니다.

- LoginServer: 로그인 검증 결과를 토큰으로 바꾸고, 게임 서버로 넘길 준비를 합니다.
- GameServer: 이동 검증, AOI 동기화, 전투, 파티/거래 같은 실시간 로직을 담당합니다.
- DBAgent: DB 접근과 트랜잭션 커밋 경계를 담당합니다.
- Redis: 토큰 검증과 런타임 변경(Dirty 대상)을 빠르게 모으는 캐시 계층입니다.

그리고 제가 끝까지 지키려고 한 규칙은 아래 한 줄로 요약됩니다.

- “수신은 빠르게, 로직은 큐에서, 커밋은 경계를 나눠서.”

이 원칙은 아래처럼 아주 단순한 형태로 표현할 수 있습니다.

```cpp
// 네트워크 스레드: 수신/완료 통지를 빠르게 처리
while (running) {
  iocp.Dispatch(10);
}

// 로직 스레드: 큐에 쌓인 일을 처리
ThreadManager::DoGlobalQueueWork();
```

---

## 3. 대표 흐름 4개
이 시리즈는 이후에 각 기술을 더 깊게 다루겠지만, 0편에서는 “정말로 이어져 있는가”만 간단한 스니펫으로 보여드리겠습니다.

### 3.1 로그인 → 토큰 → 게임 입장
로그인은 인증 결과를 토큰으로 바꾸고, 게임 서버는 그 토큰을 다시 검증하는 구조로 묶었습니다.

```cpp
// LoginServer (간략화)
if (dbVerified) {
  std::string token = MakeToken(playerId);
  redis.Set(token, std::to_string(playerId), /*ttlSec=*/300);
  SendLoginOk(token, serverList);
}
```

```cpp
// GameServer 진입부 (간략화)
auto playerIdStr = redis.Get(token);
if (playerIdStr.empty()) {
  Disconnect();
  return;
}

uint64_t playerId = std::stoull(playerIdStr);

session->Post([=] {
  BindPlayer(playerId);
  lobby->Push([=] { lobby->EnterGame(session, playerId, spawnPos); });
});
```

### 3.2 이동 권위 검증 → NavMesh 보정 → AOI 동기화
이동은 “검증 → 보정 → 전파” 순서를 지켜서, 서버가 승인한 상태만 퍼지도록 구성했습니다.

```cpp
// 이동 처리 흐름 (간략화)
if (!IsSeqNewer(seq, lastSeq))
  return;

float dt = ClampDtSec(clientTimeMs, lastClientTimeMs);
PositionInfo clamped = ClampBySpeed(curPos, reqPos, dt, moveSpeed);

PositionInfo fixed;
if (!nav.ValidateMove(curPos, clamped, fixed))
  return;

if (ShouldUpdateAOI(player))
  UpdateAOI(player);

BroadcastMoveToVisible(player->VisibleSet(), fixed);
```

### 3.3 Redis Write-Back → AutoCommit → DB 트랜잭션 커밋
저장은 런타임에는 가볍게 누적하고, 커밋 구간에서는 단단하게 묶는 방향으로 정리했습니다.

```cpp
// 런타임 변경은 Redis에 누적 + Dirty 마킹
persistence.UpdatePlayerCore(pid, level, hp, totalExp, /*markDirty=*/true);
persistence.UpdateInventoryItem(pid, itemUid, templateId, slot, count, equipped, /*markDirty=*/true);
```

```cpp
// AutoCommit 워커 (간략화)
for (uint64_t pid : CollectDirtyPlayers()) {
  S2S_REQ_SAVE_PLAYER_CORE coreReq;
  S2S_REQ_SAVE_INVENTORY invReq;

  if (BuildSnapshot_PlayerCore(pid, coreReq)) Send(coreReq);
  if (BuildSnapshot_Inventory(pid, invReq))  Send(invReq);
}
```

```cpp
// DBAgent 트랜잭션 구간 (간략화)
conn.Begin();
bool ok = true;

ok &= SavePlayerCore(coreReq);
ok &= SaveInventory(invReq);

if (ok) conn.Commit();
else    conn.Rollback();
```

### 3.4 게임 기능 예시: 파티/거래도 같은 원칙으로 묶기
파티와 거래처럼 “여러 사람의 상태가 동시에 바뀌는 기능”은 특히 직렬화 단위를 분명히 하는 것이 중요했습니다. 저는 “세션에서 바로 처리하지 않고, 액터 큐로 넘긴다”는 원칙을 그대로 적용했습니다.

```cpp
// 거래 요청/확정: 룸 액터에서 직렬 처리 (간략화)
session->PostRoom([=](auto self, auto room) {
  auto gr = static_pointer_cast<GameRoom>(room);
  gr->Push([=] {
    gr->HandleTradeConfirmById(self, playerId, pkt);
  });
});
```

```cpp
// 파티 채팅/브로드캐스트: 파티 액터에서 중계 (간략화)
PartyActor::Instance().Push([=] {
  auto partyId = core.GetPartyIdByPlayerId(senderId);
  for (auto memberId : core.GetMembers(partyId)) {
    if (auto s = sessionManager->FindByPlayerId(memberId)) {
      s->Post([=] { s->Send(partyChatNtf); });
    }
  }
});
```

---

## 4. 0편에서 제가 가장 보여드리고 싶었던 것
정리하면, 저는 이 프로젝트를 통해 아래 질문에 스스로 답해보고 싶었습니다.

- “왜 MMO였는가?”에 대해: 동시성, 권위, 정합성을 한 시스템 안에서 동시에 다루고 싶었습니다.
- “프로젝트를 한눈에 보면 무엇이 보이는가?”에 대해: 서버 역할 분리와 경계 설정, 그리고 액터 기반 직렬화라는 일관된 규칙이 보이도록 만들고 싶었습니다.

이 시리즈는 다음 편부터 그 규칙을 한 조각씩 더 또렷하게 보여드리는 방향으로 이어가겠습니다.

---

## Next: Act 1 예고
다음 글에서는 IOCP Dispatch가 어디까지 책임지고, 어디서부터 로직으로 넘기는지를 더 명확한 흐름도로 정리해보겠습니다. 특히 “수신 버퍼 → 패킷 헤더 → 핸들러 → JobQueue” 구간을 집중해서 다룰 예정입니다.
