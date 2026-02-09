# 역순/변조 패킷이 로직까지 올라오지 않게 한 패킷 파이프라인 — 헤더/ID 매핑, 역직렬화, 핸들러 디스패치

## 요약하자면
- 무엇을 해결: RecvBuffer에 누적되는 바이트 스트림을 "패킷 단위"로 안정적으로 자르고, 타입별 핸들러로 빠르게 분기했습니다.
- 어떻게 해결: `PacketSession::OnRecv`에서 경계를 조립하고, `GPacketHandler[id]` + 템플릿 역직렬화(`CRC -> Seq -> XOR -> Parse`)로 공통 진입 경로를 고정했습니다.
- 결과/효과: 네트워크 처리와 게임 로직 처리의 경계가 분명해졌고, 이동/전투/거래 요청을 같은 방식으로 처리할 수 있게 됐습니다.

---

## 1. 필요했던 이유
핵심은 "IOCP Recv 완료 통지"가 곧 "완성된 패킷"을 의미하지 않는다는 점이었습니다.

실제 Recv 완료는 아래처럼 다양한 형태로 들어옵니다.

- 하나의 패킷이 여러 Recv로 쪼개져 들어오는 경우
- 여러 패킷이 한 Recv에 붙어서 들어오는 경우
- 깨진 데이터/재전송/역순 패킷이 섞이는 경우

이 상태에서 수신 지점마다 직접 파싱하거나, 패킷마다 처리 경로가 다르면 금방 흐름이 꼬입니다. 그래서 이 편의 목표는 아래 한 줄이었습니다.

- "수신 바이트 -> 패킷 경계 조립 -> 타입 분기 -> 공통 검증/역직렬화 -> 로직 큐 위임"을 한 경로로 고정한다.

---

## 2. 첫 시도와 실패
처음에는 수신 직후에 패킷별 로직으로 바로 들어가려는 방식이었습니다.

```cpp
// 초기 형태(의도): 수신 즉시 게임 로직 호출
void OnRecvPacket(BYTE* buffer, int32 len)
{
    if (header->id == PKT_C_MOVE)
        HandleMoveDirectly();
    else if (header->id == PKT_C_SKILL)
        HandleSkillDirectly();
}
```

이 접근의 문제는 두 가지였습니다.

- 패킷 경계 조립/검증/역직렬화가 핸들러마다 흩어져 중복이 늘어났습니다.
- 로직 진입 지점이 세션/룸 컨텍스트를 섞어 쓰면서 동기화 이슈 추적이 어려웠습니다.

결국 방향을 바꿨습니다.

- 파싱/검증은 공통 템플릿으로 모으고,
- 핸들러에서는 바로 게임 상태를 건드리지 않고 액터 큐(`PostRoom`, `Push`)로 넘기게 정리했습니다.

---

## 3. 어떻게 풀었나
핵심은 "패킷 파이프라인을 4단계로 고정"한 것입니다.

1. `PacketSession::OnRecv`에서 `PacketHeader.size` 기준으로 패킷 경계를 조립한다.
2. 조립된 패킷은 `ClientPacketHandler::HandlePacket`에서 `header->id`로 O(1) 분기한다.
3. 타입 템플릿에서 `CRC -> Seq -> XOR -> ParseFromArray` 순서로 공통 검증/역직렬화를 수행한다.
4. 최종 핸들러(`Handle_C_MOVE` 등)는 룸 액터 큐로 넘겨 로직을 직렬 처리한다.

### 3.1 흐름 요약 다이어그램
```text
[Client]
   │  (encrypted protobuf body + PacketHeader)
   ▼
[IOCP Recv Complete]
   ▼
[Session::ProcessRecv]
   ▼
[PacketSession::OnRecv]
   ├─ header 미완성/패킷 미완성 => 대기
   └─ 완성 패킷 => OnRecvPacket(buffer, size)
                     ▼
               [ClientPacketHandler::HandlePacket]
                     ▼
               GPacketHandler[packetId]
                     ▼
         HandlePacket<T>()
         (CRC -> Seq -> XOR -> Parse)
                     ▼
               Handle_C_XXX(...)
                     ▼
            session->PostRoom(...)
                     ▼
              GameRoom Actor Queue
```

---

## 4. 구현의 핵심
패킷 파이프라인의 핵심 코드는 5지점에 있습니다.

- `ServerCore/Session.h` — `PacketHeader`
- `ServerCore/Session.cpp` — `PacketSession::OnRecv`
- `GameServer/PlayerSession.cpp` — `OnRecvPacket`
- `GameServer/ClientPacketHandler.h` — `GPacketHandler`, `HandlePacket<T>()`
- `GameServer/ClientPacketHandler.GamePlay.cpp` — `Handle_C_MOVE`

스니펫 #1: 고정 헤더를 두고 파이프라인의 최소 공통 정보를 유지합니다.
```cpp
struct PacketHeader
{
    uint16 size;
    uint16 id;
    uint32 crc;
    uint32 seq;
};
```

스니펫 #2: 바이트 스트림을 패킷 단위로 자르고, 완성된 단위만 훅으로 올립니다.
```cpp
int32 PacketSession::OnRecv(BYTE* buffer, int32 len)
{
    int32 processLen = 0;

    while (true)
    {
        int32 dataSize = len - processLen;
        if (dataSize < sizeof(PacketHeader))
            break;

        PacketHeader header = *(reinterpret_cast<PacketHeader*>(&buffer[processLen]));
        if (dataSize < header.size)
            break;

        OnRecvPacket(&buffer[processLen], header.size);
        processLen += header.size;
    }

    return processLen;
}
```

스니펫 #3: 핸들러 테이블로 빠르게 분기하고, 정의되지 않은 ID는 기본 핸들러로 흡수합니다.
```cpp
void ClientPacketHandler::Init()
{
    for (int32 i = 0; i < UINT16_MAX; i++)
        GPacketHandler[i] = Handle_INVALID;

    GPacketHandler[PKT_C_MOVE]  = [](PacketSessionRef& s, BYTE* b, int32 l) {
        return HandlePacket<Protocol::C_MOVE>(Handle_C_MOVE, s, b, l);
    };
    GPacketHandler[PKT_C_SKILL] = [](PacketSessionRef& s, BYTE* b, int32 l) {
        return HandlePacket<Protocol::C_SKILL>(Handle_C_SKILL, s, b, l);
    };
}
```

스니펫 #4: 역직렬화 전 공통 검증을 고정 순서로 수행합니다.
```cpp
template<typename PacketType, typename ProcessFunc>
static bool HandlePacket(ProcessFunc func, PacketSessionRef& session, BYTE* buffer, int32 len)
{
    PacketHeader* header = reinterpret_cast<PacketHeader*>(buffer);
    int32 dataSize = len - sizeof(PacketHeader);

    uint32 calcCrc = Crc32::Compute(buffer + sizeof(PacketHeader), dataSize);
    if (header->crc != calcCrc)
        return false;

    if (session->CheckRecvSeq(header->seq) == false)
        return false;

    XorCrypt(buffer + sizeof(PacketHeader), dataSize);

    PacketType pkt;
    if (pkt.ParseFromArray(buffer + sizeof(PacketHeader), dataSize) == false)
        return false;

    return func(session, pkt);
}
```

스니펫 #5: 핸들러는 룸 액터 큐로 넘겨서 로직을 직렬화합니다.
```cpp
bool ClientPacketHandler::Handle_C_MOVE(PacketSessionRef& session, Protocol::C_MOVE& pkt)
{
    auto ps = static_pointer_cast<PlayerSession>(session);
    if (!ps || ps->IsMapChanging())
        return true;

    const uint64 playerId = ps->GetPlayerId_AnyThread();
    if (playerId == 0)
        return true;

    ps->PostRoom([playerId, pkt](PlayerSessionRef self, RoomActorRef room) mutable
    {
        if (!room || room->GetKind() != RoomKind::Game)
            return;

        auto gr = static_pointer_cast<GameRoom>(room);
        gr->Push([gr, self, playerId, pkt]() mutable
        {
            gr->HandleMoveById(self, playerId, pkt);
        });
    });

    return true;
}
```

---

## 5. 경계 조건과 실패 케이스
이 구간은 "언제 발생하는가 -> 현재 어떻게 처리되는가"로 정리했습니다.

- 헤더 미완성/패킷 미완성
  - 발생: 분할 수신으로 `PacketHeader` 또는 본문이 덜 들어왔을 때
  - 처리: `PacketSession::OnRecv`가 `break` 후 다음 Recv를 기다립니다.

- 연결 종료(Recv 0)
  - 발생: 상대 graceful shutdown
  - 처리: `Session::ProcessRecv`에서 `Disconnect(L"Recv 0")` 호출로 세션 종료합니다.

- 무결성/재전송 문제(CRC, Seq)
  - 발생: 본문 변조/손상, 이미 처리한 seq 재수신
  - 처리: `HandlePacket<T>()`에서 `false` 반환으로 해당 패킷 처리 중단합니다.

- 정의되지 않은 packet id
  - 발생: 미등록/오입력 id
  - 처리: `Handle_INVALID` 경로로 떨어져 `false` 반환합니다.

- 맵 이동 중 또는 playerId 미바인딩 상태
  - 발생: 맵 전환 타이밍, 로그인 미완료 상태
  - 처리: 각 핸들러에서 조기 `return true`로 무시합니다.

- 현재 보강이 필요한 지점
  - `PacketSession::OnRecv`에 `header.size` 최소값 검증(`>= sizeof(PacketHeader)`)이 없어 비정상 헤더 방어가 약합니다.
  - `GPacketHandler[UINT16_MAX]` 배열은 최대 id 경계 처리(특히 `65535`)를 한 번 더 명시적으로 점검할 필요가 있습니다.
  - `HandlePacket`의 `false` 반환이 세션 단에서 적극적인 대응(로그/차단)까지 연결되지는 않습니다.

---

## 6. 트레이드오프

- 대안 1: `switch-case` 직접 분기
  - 선택: 배열 기반 핸들러 테이블(`GPacketHandler[id]`)
  - 이유: 분기 비용이 예측 가능하고 확장 시 패턴 유지가 쉽습니다.

- 대안 2: 핸들러별 개별 파싱/검증
  - 선택: 템플릿 공통 파싱(`HandlePacket<T>()`)
  - 이유: `CRC/Seq/XOR/Parse`를 한 번에 통일해 중복과 누락 가능성을 줄였습니다.

- 대안 3: 핸들러에서 즉시 게임 상태 변경
  - 선택: `PostRoom -> RoomActor::Push`로 위임
  - 이유: 네트워크 스레드와 게임 로직 스레드 경계를 분리해 동시성 리스크를 낮췄습니다.

- 대안 4: 강한 암호화/서명 체계
  - 선택: XOR + CRC + Seq
  - 이유: 구현 복잡도와 비용을 낮춘 대신, 보안 강도는 제한적입니다(운영 단계에서 강화 필요).

---

## 7. 테스트/측정
- 재현 시나리오
  - 분할 수신: 큰 패킷을 쪼개 전송해 경계 조립 확인
  - 역순/재전송: 같은 `seq` 재전송으로 드롭 여부 확인
  - 변조: body 일부 비트 수정 후 CRC 불일치 처리 확인
  - 미등록 ID: 등록되지 않은 ID 패킷 입력 시 기본 핸들러 경로 확인

- 확인 포인트
  - `PacketSession::OnRecv`가 부분 패킷에서 `processLen`을 과소/과대 처리하지 않는지
  - 핸들러 진입 전 공통 검증이 누락 없이 실행되는지
  - 로직 처리가 RoomActor 큐로만 들어가는지

- 관찰 포인트
  - 현재는 운영 지표(패킷 드롭 건수, CRC 실패율, seq 재수신율) 집계 코드가 부족합니다.

- 수치
  - 아직 수치화된 결과는 없습니다.
  - 다음 단계에서 `HandlePacket` 실패 원인별 카운터를 넣고 지표화할 계획입니다.

---

## 8. 회고와 다음 편
- 배운 점
  - 패킷 파이프라인에서 가장 중요한 것은 "빠른 분기"보다 "경계와 검증의 일관성"이었습니다.
  - 특히 로직 스레드로의 위임 경계를 초기에 고정해두면, 이후 기능(파티/거래/던전)이 늘어나도 흐름이 덜 흔들립니다.

- 다음 개선
  - `header.size`/`id` 경계 검증을 더 명시적으로 보강
  - 실패 반환(`false`)의 후속 정책(로그/차단/지표)을 표준화
  - PacketGenerator와 연결해 핸들러 매핑 생성 자동화 범위를 확대

- 다음 편 예고
  - 다음 글에서는 네트워크 스레드와 로직 워커를 어떻게 분리했는지, 그리고 JobQueue/GlobalQueue가 어떤 병목을 줄였는지를 다룹니다.
  - 특히 "핸들러에서 왜 바로 처리하지 않고 큐로 넘기는가"를 스레드 모델 관점에서 정리하겠습니다.
