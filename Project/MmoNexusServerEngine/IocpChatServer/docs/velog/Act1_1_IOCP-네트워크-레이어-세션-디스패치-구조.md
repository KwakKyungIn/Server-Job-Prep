# 연결 폭주에서도 Accept가 밀리지 않게 한 IOCP 세션 디스패치 — Overlapped 완료 통지 기반 네트워크 레이어

## 요약하자면
- 무엇을 해결: 접속 폭주 시 Accept/Recv/Send가 서로 막히는 상황을 줄였습니다.
- 어떻게 해결: IOCP 완료 통지를 관문으로 두고, `IocpCore → IocpEvent → Listener/Session`으로 디스패치 경로를 고정했습니다.
- 결과/효과: 블로킹 구간을 제거했고, 종료/에러 처리가 분리되어 원인 추적이 쉬워졌습니다.

---

## 1. 필요했던 이유
IOCP는 “요청을 던지고 완료 통지를 기다리는 방식”입니다. 문제는 이 완료 통지가 언제, 어떤 순서로 돌아올지 예측하기 어렵다는 데 있습니다. 연결(Accept)과 수신(Recv), 송신(Send)이 한꺼번에 몰리는 순간, 처리 흐름이 조금만 꼬여도 체감 지연이 크게 늘어납니다.

“왜 IOCP냐”를 정리하면 이렇습니다. Windows 환경에서는 IOCP가 사실상 표준 고성능 비동기 I/O 모델입니다. 반면 epoll이나 io_uring은 Linux 전용이라 선택지 자체가 달랐습니다. 또 IOCP는 “완료 통지 기반”이라, 요청을 걸어둔 뒤 완료 시점에 콜백처럼 결과가 돌아옵니다. 이 특성 때문에 **요청 순서와 완료 순서가 일치하지 않을 수 있고**, 동시에 여러 이벤트가 겹쳐 들어옵니다. 그래서 상태 관리가 조금만 느슨해도 Accept/Recv/Send가 서로 얽히며 지연이 체감되기 쉽습니다. 이 부분이 IOCP에서 특히 까다로운 지점이었습니다.

증상은 이런 식으로 나타납니다. 예를 들어 동시에 100개 접속이 들어오면, 앞선 몇 개는 즉시 연결되지만 뒤쪽 접속은 `OnConnected` 로그가 몇 초 뒤에 찍히는 식으로 지연됩니다. 사용자는 “접속은 됐는데 서버가 멈춘 것 같다”는 느낌을 받습니다.

문제의 본질은 완료 통지의 분기점이 분산되어 있다는 점입니다. 어떤 이벤트가 어디에서 처리되는지 흐름이 분리되어 있으면, 한쪽 지연이 다른 쪽까지 전파되기 쉽습니다. 결국 “디스패치 경로를 한 줄로 고정”하는 게 필요했습니다.

요구사항은 다음과 같이 정했습니다.
- **정확성(끊김 없는 수명주기)**이 최우선이며, 그 다음이 **성능(대량 접속)**, 마지막이 **운영 안정성(예외 처리 단순화)**입니다.

제약은 명확했습니다. Windows IOCP 기반이어야 했고, 서버 코어는 Game/Login/DBAgent에서 공용으로 재사용돼야 했습니다.

## 2. 첫 시도와 실패
처음에는 AcceptEx 실패 시 무조건 재등록하는 방식으로 “끊김을 제거”하려 했습니다. 그런데 서버 종료 시점에 `CloseAccept()`가 호출되어도 재등록이 반복되면서 스택 오버플로우가 발생했습니다.

원인은 단순했습니다. 소켓이 `INVALID_SOCKET` 상태인데도 `RegisterAccept()`가 계속 재귀 호출되던 구조였습니다. 이때 깨달은 점은 명확합니다. **재등록 루프에는 종료 상태를 감지하는 탈출 조건이 필수**라는 것입니다.

## 3. 어떻게 풀었나
핵심은 “완료 통지의 관문을 하나로 만든다”는 생각이었습니다. 완료 통지는 반드시 `IocpCore`에서 꺼내고, `IocpEvent.owner`를 기준으로 처리 대상이 결정되도록 고정했습니다.

Before/After로 보면 의도가 더 선명합니다.

기존 방식(분산된 처리): 이벤트 완료 지점에서 곧바로 처리하면, 어느 지점에서 막히는지 추적이 어렵습니다.\n
```cpp
// AcceptEx 완료 → 콜백에서 직접 처리
void OnAcceptComplete()
{
    session->Init();  // 병목 위치 추적이 어려움
    RegisterAccept(); // 즉시 재무장
}
```

새 방식(단일 관문): IOCP 완료 통지에서 owner로 위임해 처리 경로를 고정합니다.\n
```cpp
// IOCP 완료 통지 → owner에게 위임
void IocpCore::Dispatch()
{
    iocpEvent->owner->Dispatch(iocpEvent, numOfBytes);
}
```

3단계로 요약하면 다음과 같습니다.
1) 선등록: AcceptEx/Recv/Send를 미리 걸어둡니다.
2) 완료 통지: IOCP 큐에서 완료 이벤트를 꺼냅니다.
3) 재무장: 처리 후 즉시 다시 등록해 파이프라인을 유지합니다.

### 3.1 흐름 요약 다이어그램
```text
① 선등록 (AcceptEx N개 / Recv / Send)
          │
          ▼
② 완료 통지 (IOCP 큐에서 pop)
          │
          ▼
③ 재무장 (RegisterAccept/Recv/Send)
          ▲
          └──────────── 반복
```

이 구조에서 Listener는 Accept를 맡고, Session은 Connect/Recv/Send를 맡습니다. 즉, “누가 무엇을 처리하는지”가 고정되도록 나눴습니다.

## 4. 구현의 핵심
IOCP 완료 통지를 단순 이벤트 수신으로 보지 않고, **세션 수명주기를 묶는 중심 축**으로 둡니다. 그래서 각 스니펫은 “어디서 분기하고, 어디서 재무장하는지”가 보이도록 구성했습니다.

스니펫 #1: IOCP 이벤트가 어떤 타입인지, 누가 처리할지 명확히 담습니다.
```cpp
class IocpEvent : public OVERLAPPED
{
public:
    explicit IocpEvent(EventType type);
    void Init();

public:
    EventType     eventType;
    IocpObjectRef owner;
};
```

스니펫 #2: IOCP에서 이벤트를 받으면, owner에게 즉시 위임해 처리 경로를 단일화합니다.
```cpp
DWORD numOfBytes = 0;
ULONG_PTR key = 0;
IocpEvent* iocpEvent = nullptr;

if (::GetQueuedCompletionStatus(_iocpHandle, OUT &numOfBytes, OUT &key,
    OUT reinterpret_cast<LPOVERLAPPED*>(&iocpEvent), timeoutMs))
{
    IocpObjectRef iocpObject = iocpEvent->owner;
    iocpObject->Dispatch(iocpEvent, numOfBytes);
}
```

스니펫 #3: AcceptEx 재무장 시 종료 상태를 먼저 확인해 재귀 폭주를 차단합니다.
```cpp
if (_socket == INVALID_SOCKET)
    return; // 종료 중이면 재등록 금지

SessionRef session = _service->CreateSession();
acceptEvent->Init();
acceptEvent->session = session;
SocketUtils::AcceptEx(_socket, session->GetSocket(),
    session->_recvBuffer.WritePos(), 0, ... , acceptEvent);
```

스니펫 #4: 이벤트 타입별로 처리하고, 완료 후 즉시 재무장합니다.
```cpp
switch (iocpEvent->eventType)
{
case EventType::Connect:    ProcessConnect();        break;
case EventType::Disconnect: ProcessDisconnect();     break;
case EventType::Recv:       ProcessRecv(numOfBytes); break;
case EventType::Send:       ProcessSend(numOfBytes); break;
}
```

## 5. 경계 조건과 실패 케이스
이 섹션은 “언제 발생하는가 → 어떻게 처리하는가” 흐름으로 정리했습니다.

- `Recv` 완료 바이트가 0인 경우는 상대가 정상 종료(graceful shutdown)했을 때 자주 나타납니다. 이때는 **데이터가 더 들어오지 않는 상태**이므로 즉시 `Disconnect()`를 호출해 세션 수명주기를 종료합니다.\n
- RecvBuffer 범위 초과나 파싱 실패는 패킷이 깨졌거나 악성 입력이 들어온 경우에 발생합니다. 이때는 **부분 처리를 시도하지 않고** 바로 끊어서 상태 꼬임을 방지합니다.\n
- `Send` 완료 바이트가 0으로 돌아오는 경우는 연결이 이미 끊긴 상태에서 송신이 등록되었을 가능성이 큽니다. 이 경우에도 즉시 종료 처리로 수명주기를 정리합니다.\n
- 서버 종료 중 AcceptEx가 계속 재등록되면 재귀적으로 누적되어 스택 오버플로우가 발생합니다. 이를 막기 위해 `_socket == INVALID_SOCKET` 체크로 **종료 상태를 감지하면 재무장을 중단**합니다.\n
- `GetQueuedCompletionStatus`가 실패하더라도 OVERLAPPED가 유효한 경우가 있습니다. 그래서 **오류 판단과 후속 처리는 Session/Listener 내부에서 책임지도록** 분리했습니다.

## 6. 트레이드오프
선택지를 비교하면 결정이 더 분명해집니다.

- 대안 1: **Lock-free 큐로 디스패치** → 이론상 성능은 올라가지만, 구현 복잡도와 디버깅 난이도가 급격히 증가합니다.\n  선택: **owner 기반 고정 경로**로 단순성을 우선했습니다. 이벤트가 어디로 가는지 추적이 쉬워집니다.
- 대안 2: **AcceptEx 동적 조절**(접속량에 따라 등록 개수 조정) → 메모리는 절약되지만, 타이밍 제어와 튜닝 비용이 커집니다.\n  선택: **AcceptEx 선등록 고정값**으로 안정성을 우선했습니다. 접속 피크에서 지연이 줄어드는 효과가 큽니다.
- 대안 3: **오류 시 재시도 중심** → 연결 안정성은 높을 수 있지만, 종료 시나리오에서 무한 재등록 위험이 커집니다.\n  선택: **종료 조건 우선**으로 끊김이 확정된 경우 즉시 정리하도록 했습니다.

결론적으로, 이 편에서는 성능 최적화보다 **재현 가능성과 수정 난이도**를 우선했습니다.

## 7. 테스트/측정
- 재현 시나리오: 서버 기동 → 더미 클라이언트 다중 접속 → 접속/해제 반복.
- 확인 포인트: Accept 재무장 누락 여부, Recv 0 처리 로그, 종료 시 스택 오버플로우 재발 여부.
- 관찰 포인트: 접속 피크에서 Accept 큐가 비지 않는지, 세션 수명주기가 꼬이지 않는지 로그로 확인했습니다.
- 수치: 현재는 수치가 없습니다. 추후 동시 접속 수/Accept 지연 로그/에러 빈도를 지표화할 계획입니다.

## 8. 회고와 다음 편
- 배운 점: IOCP에서 “재등록 루프”는 성능보다 **정확성/안정성의 핵심 지점**이라는 것을 체감했습니다.
- 다음 개선: 에러 케이스별 로깅을 분리하고, 실패 유형을 지표로 남길 계획입니다.
- 다음 편 예고: 패킷 헤더/ID 매핑으로 **역직렬화와 핸들러 디스패치** 흐름을 정리하겠습니다.
