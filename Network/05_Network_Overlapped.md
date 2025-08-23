# ⚙️ Winsock Overlapped I/O — Event 기반 & Callback 기반

> **Overlapped I/O**는 윈도우에서 제공하는 **비동기 소켓 I/O** 방식입니다.  
> 호출 스레드를 막지 않고(=non-blocking과 다름), 커널이 I/O를 진행해 **완료 시점에 알림**을 받습니다.  
> 알림 방식은 두 가지:
>
> 1) **Event 기반**: `OVERLAPPED.hEvent`를 신호로 받아 `WSAGetOverlappedResult()`로 결과 수거  
> 2) **Callback 기반(Completion Routine)**: 완료 시 커널이 **콜백(APC)** 을 호출 → 스레드는 **alertable wait**가 필요

---

## 🧭 언제 Overlapped를 쓰나?

- `select()`/`WSAEventSelect`보다 **진짜 비동기**(I/O를 커널이 수행, 완료 알림만 받음).  
- **IOCP**(I/O Completion Ports)와 같은 **완료 통지 모델**의 기초이자 근간.  
- 소규모/학습 단계: Overlapped **event/callback**으로 구조 이해 → **확장 시 IOCP**로 자연스럽게 이행.

> 📌 **주의**: 대규모 서버(수천~수만 연결)는 **IOCP**가 정석입니다. Overlapped 이벤트는 **핸들 64개 제한**(샤딩 필요), 콜백은 **alertable wait** 관리가 번거롭습니다.

---

## 🧱 준비 사항 (공통)

1. **Overlapped 소켓 생성**
1. ```cpp
   SOCKET s = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, nullptr, 0, WSA_FLAG_OVERLAPPED);
   ```

2. **비동기 호출**

   * `WSARecv()` / `WSASend()`에 `LPWSAOVERLAPPED` 전달
   * 반환값이 `SOCKET_ERROR`이고 `WSAGetLastError() == WSA_IO_PENDING` → 정상적으로 **백그라운드 진행 중**

3. **중요 규칙 (수명과 재진입)**

   * `OVERLAPPED` **구조체와 버퍼는 I/O 완료까지 유효**해야 함(스택 변수 금지 → per-connection 힙/멤버 보관).
   * **완료 순서는 보장되지 않음** → per-connection 상태머신/큐 필요.
   * **부분 전송/부분 수신**을 가정하고 루프/큐로 처리.

---

## 1) Event 기반 Overlapped

### 동작 흐름

1. 소켓당 `OVERLAPPED`와 **이벤트**(보통 `WSACreateEvent()`로 manual-reset 생성) 준비
2. `WSARecv/WSASend` 호출 시 `overlapped.hEvent = hEvent` 설정
3. `WSAWaitForMultipleEvents()`(또는 `WaitForSingleObject`)로 **완료 이벤트 대기**
4. 깨어나면 `WSAGetOverlappedResult()`로 **전송/수신된 바이트 수**와 **상태** 수거
5. `WSAResetEvent(hEvent)` 후 **다음 I/O** 재게시

> ⚠️ 한 스레드가 `WaitFor*`로 기다릴 수 있는 이벤트 핸들은 **최대 64개**입니다. 연결 수가 많다면 **소켓을 샤딩**하거나 **스레드/그룹**을 분리하세요.

### 예제: Overlapped 수신 → 에코 전송

```cpp
// build: link ws2_32.lib
#include <winsock2.h>
#include <mswsock.h>
#include <vector>
#include <iostream>
#pragma comment(lib, "ws2_32.lib")

struct Conn {
    SOCKET s;
    WSAEVENT ev;
    WSAOVERLAPPED ovRecv{};
    WSAOVERLAPPED ovSend{};
    WSABUF rbuf, sbuf;
    char rdata[4096];
    char sdata[4096];
    bool recvPending = false;
    bool sendPending = false;
};

bool PostRecv(Conn& c) {
    ZeroMemory(&c.ovRecv, sizeof(c.ovRecv));
    c.ovRecv.hEvent = c.ev;
    c.rbuf.buf = c.rdata;
    c.rbuf.len = (ULONG)sizeof(c.rdata);
    DWORD flags = 0, recvd = 0;
    int r = WSARecv(c.s, &c.rbuf, 1, &recvd, &flags, &c.ovRecv, nullptr);
    if (r == 0) { // 동기 완료
        // recvd 바이트 수신됨 → 즉시 처리
        return true;
    }
    if (r == SOCKET_ERROR && WSAGetLastError() == WSA_IO_PENDING) {
        c.recvPending = true;
        return true; // 비동기 진행 중
    }
    return false; // 오류
}

bool PostSend(Conn& c, const char* data, int len) {
    memcpy(c.sdata, data, len);
    c.sbuf.buf = c.sdata;
    c.sbuf.len = len;
    ZeroMemory(&c.ovSend, sizeof(c.ovSend));
    c.ovSend.hEvent = c.ev;
    DWORD sent = 0;
    int r = WSASend(c.s, &c.sbuf, 1, &sent, 0, &c.ovSend, nullptr);
    if (r == 0) { // 동기 완료
        return true;
    }
    if (r == SOCKET_ERROR && WSAGetLastError() == WSA_IO_PENDING) {
        c.sendPending = true;
        return true; // 비동기 진행
    }
    return false;
}

int main() {
    WSADATA w; WSAStartup(MAKEWORD(2,2), &w);

    SOCKET ls = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, nullptr, 0, WSA_FLAG_OVERLAPPED);
    sockaddr_in a{}; a.sin_family = AF_INET; a.sin_port = htons(9000); a.sin_addr.s_addr = htonl(INADDR_ANY);
    bind(ls, (sockaddr*)&a, sizeof(a));
    listen(ls, SOMAXCONN);

    // 데모: 동기 accept 후, 클라이언트 1개만 Overlapped 이벤트로 처리
    SOCKET s = accept(ls, nullptr, nullptr);
    u_long nonblk = 0; ioctlsocket(s, FIONBIO, &nonblk); // Overlapped는 non-blocking이 필수는 아니지만, 데모에선 0 유지

    Conn c{};
    c.s = s;
    c.ev = WSACreateEvent();

    PostRecv(c);

    while (true) {
        WSAEVENT evs[1] = { c.ev };
        DWORD idx = WSAWaitForMultipleEvents(1, evs, FALSE, WSA_INFINITE, FALSE);
        if (idx == WSA_WAIT_FAILED) break;

        // 수거
        WSANETWORKEVENTS dummy; // 사용 안 함
        // 이벤트는 manual-reset → 반드시 리셋
        WSAResetEvent(c.ev);

        // RECV 완료 확인
        DWORD xfer = 0; DWORD flags = 0;
        BOOL ok = WSAGetOverlappedResult(c.s, &c.ovRecv, &xfer, FALSE, &flags);
        if (ok && xfer > 0) {
            // 에코 전송
            PostSend(c, c.rdata, (int)xfer);
            // 다음 수신 게시
            PostRecv(c);
        } else if (!ok) {
            int e = WSAGetLastError();
            if (e == WSA_IO_INCOMPLETE) {
                // 아직 미완료 → 다음 루프에서 다시 수거
            } else {
                // 연결 종료/오류
                break;
            }
        }

        // SEND 완료 확인
        xfer = 0; flags = 0;
        ok = WSAGetOverlappedResult(c.s, &c.ovSend, &xfer, FALSE, &flags);
        if (!ok) {
            int e = WSAGetLastError();
            if (e != WSA_IO_INCOMPLETE) break;
        }
    }

    WSACloseEvent(c.ev);
    closesocket(s); closesocket(ls);
    WSACleanup();
}
```

> 팁
>
> * `WSAGetOverlappedResult(..., fWait=FALSE, ...)`는 **즉시 수거** 시도. 완료가 아니면 `WSA_IO_INCOMPLETE`.
> * 이벤트는 `WSACreateEvent()`로 만들면 **manual-reset**이므로, 처리 후 **반드시 `WSAResetEvent()`**.

---

## 2) Callback 기반 Overlapped (Completion Routine)

### 동작 흐름

1. `WSARecv/WSASend` 호출 시 `lpCompletionRoutine`에 **콜백**을 전달
2. 호출 스레드는 **alertable wait** 상태여야 콜백이 실행됨

   * `SleepEx(INFINITE, TRUE)`
   * `WaitForSingleObjectEx(..., TRUE)` / `WaitForMultipleObjectsEx(..., TRUE)` 등
3. I/O 완료 시 커널이 스레드에 **APC**를 큐잉 → 스레드가 alertable이면 콜백 즉시 실행

### 콜백 시그니처

```cpp
void CALLBACK IoCompleted(
    DWORD dwError, DWORD cbTransferred,
    LPWSAOVERLAPPED lpOverlapped, DWORD dwFlags);
```

### 예제: 콜백으로 수신 → 다시 게시

```cpp
#include <winsock2.h>
#include <mswsock.h>
#include <iostream>
#pragma comment(lib, "ws2_32.lib")

struct Conn {
    SOCKET s;
    WSAOVERLAPPED ovRecv{};
    WSABUF rbuf;
    char rdata[4096];
};

void CALLBACK RecvCompleted(DWORD err, DWORD xfer, LPWSAOVERLAPPED lp, DWORD flags) {
    auto* c = CONTAINING_RECORD(lp, Conn, ovRecv); // ovRecv의 소유 객체 복구
    if (err != 0 || xfer == 0) {
        std::cout << "closed or error: " << err << "\n";
        return;
    }
    // 수신 처리 (예: 로그)
    std::cout << "recv " << xfer << " bytes\n";

    // 다음 수신 게시
    ZeroMemory(&c->ovRecv, sizeof(c->ovRecv));
    c->rbuf.buf = c->rdata; c->rbuf.len = sizeof(c->rdata);
    DWORD flags2 = 0, recvd = 0;
    int r = WSARecv(c->s, &c->rbuf, 1, &recvd, &flags2, &c->ovRecv, RecvCompleted);
    if (r == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING) {
        std::cout << "WSARecv error\n";
    }
}

int main() {
    WSADATA w; WSAStartup(MAKEWORD(2,2), &w);

    SOCKET ls = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, nullptr, 0, WSA_FLAG_OVERLAPPED);
    sockaddr_in a{}; a.sin_family = AF_INET; a.sin_port = htons(9001); a.sin_addr.s_addr = htonl(INADDR_ANY);
    bind(ls, (sockaddr*)&a, sizeof(a));
    listen(ls, SOMAXCONN);

    SOCKET s = accept(ls, nullptr, nullptr);

    Conn* c = new Conn{};
    c->s = s;
    c->rbuf.buf = c->rdata; c->rbuf.len = sizeof(c->rdata);
    DWORD flags = 0, recvd = 0;

    // 첫 recv 게시 (콜백)
    int r = WSARecv(c->s, &c->rbuf, 1, &recvd, &flags, &c->ovRecv, RecvCompleted);
    if (r == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING) {
        std::cout << "WSARecv post error\n";
        return 0;
    }

    // 콜백이 실행되려면 "alertable wait"가 필요
    while (true) {
        DWORD wr = SleepEx(INFINITE, TRUE); // TRUE → alertable
        if (wr == 0) {
            // 타이머 만료(이 케이스에선 없음)
        } else if (wr == WAIT_IO_COMPLETION) {
            // 콜백 하나 실행됨 → 루프 계속(여러 콜백이 올 수 있음)
        } else {
            break; // 기타
        }
    }
}
```

> 팁
>
> * **콜백은 호출 스레드 컨텍스트**에서 실행 → 공유 자원 보호(락/lock-free) 설계 필요.
> * `SleepEx/Wait*Ex`에서 `bAlertable=TRUE`를 잊으면 **콜백이 절대 실행되지 않음**.

---

## 🔍 공통 패턴 (실무 체크리스트)

* **Per-connection 컨텍스트**
  `struct Connection { SOCKET; OVERLAPPED(여러 개); 버퍼; 상태; 참조카운트; }` 를 힙에 두고, I/O 당 **고유 OVERLAPPED** 사용.
* **읽기 루프**

  * 수신 완료 → **처리 → 다음 `WSARecv` 즉시 게시**(항상 파이프라인 유지).
* **쓰기 큐**

  * `WSASend`는 부분 전송 가능 → **송신 큐**에서 나눠 보내고 **다음 send**를 **완료 시점**에만 게시.
* **취소/종료**

  * 소켓 닫기 전에 `CancelIoEx((HANDLE)s, &ov)`로 개별 I/O 취소 가능.
  * 닫기/취소 시 **콜백이 올 수 있으므로** 컨텍스트 **참조 카운트**로 수명 보장.
* **에러 처리**

  * 게시 직후 `WSA_IO_PENDING`은 정상.
  * 완료 시 `dwError != 0`/`cbTransferred == 0`이면 **연결 종료**로 판단.

---

## ⚠️ 흔한 실수 & 함정

1. **OVERLAPPED/버퍼 수명**

   * 함수 로컬로 만들고 리턴 → 아직 I/O 진행 중인데 메모리 파괴 → **즉시 크래시**.
2. **이벤트 리셋 누락**(Event 기반)

   * `WSACreateEvent()`는 **manual-reset** → 처리 후 `WSAResetEvent()` 필수.
3. **동시 I/O 당 OVERLAPPED 재사용**

   * 같은 `OVERLAPPED`를 recv/send 둘 다에 재사용 → **데이터 경합/메모리 오염**.
4. **alertable wait 미구현**(Callback 기반)

   * `SleepEx(..., TRUE)` 등 없으면 **콜백이 영원히 실행되지 않음**.
5. **부분 전송/수신 무시**

   * `WSASend/WSARecv`는 **항상 전체 길이를 보장하지 않음** → 루프/큐로 보완.

---

## 📊 모델 비교 한눈에

| 모델                       | 통지 방식      | 대기 수단                                   | 장점                | 단점                    | 규모   |
| ------------------------ | ---------- | --------------------------------------- | ----------------- | --------------------- | ---- |
| `select()`               | readiness  | `select()`                              | 단순                | 매 호출 재구성, 스케일 한계      | 소    |
| `WSAEventSelect`         | readiness  | `WSAWaitForMultipleEvents`              | 구조 단순, 학습 쉬움      | 이벤트 64 제한             | 소\~중 |
| **Overlapped(Event)**    | completion | (W)WaitFor\* + `WSAGetOverlappedResult` | 진짜 비동기, 설계 유연     | 64 제한, 수거/리셋 관리       | 중    |
| **Overlapped(Callback)** | completion | **alertable wait** + 콜백                 | 이벤트 핸들 불필요, 반응 빠름 | alertable 관리, 스레드 안전성 | 중    |
| **IOCP**                 | completion | `GetQueuedCompletionStatus`             | 최고 스케일/성능         | 구현 난이도 높음             | 대    |

---

## ✅ 요약

* Overlapped는 **비동기 완료 통지** 모델(이벤트/콜백)이며, **IOCP의 기초**.
* **Event 기반**: `OVERLAPPED.hEvent` + `WSAGetOverlappedResult()` + **리셋 주의**.
* **Callback 기반**: `lpCompletionRoutine` + **alertable wait** 필수.
* 실무에서는 **per-connection 컨텍스트/큐/상태머신**으로 **부분 I/O**와 **수명**을 안전하게 다룸.

