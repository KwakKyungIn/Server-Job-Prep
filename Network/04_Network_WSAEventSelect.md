# 🔔 WSAEventSelect 모델 (Winsock 이벤트 기반 멀티플렉싱)

> **WSAEventSelect**는 윈도우 전용 비동기 네트워킹 모델로, 소켓에 **이벤트 오브젝트(커널 이벤트)**를 연결해두고  
> `WSAWaitForMultipleEvents()`로 **여러 소켓의 네트워크 이벤트(FD_READ, FD_WRITE, …)** 를 한 번에 감시하는 방식입니다.  
> 핵심은 **“준비됨(ready) 알림”을 받아서 non-blocking I/O를 수행**하는 **readiness 모델**이라는 점입니다.

---

## 1) 왜 WSAEventSelect인가?

- `select()` 대비
  - 장점: `fd_set`을 매번 재구성할 필요가 없음, **커널 이벤트**로 기다려 **불필요한 폴링 감소**.
  - 단점: **동시에 기다릴 수 있는 이벤트 핸들 수가 64개**(`MAXIMUM_WAIT_OBJECTS`)로 제한. 대규모 서버엔 부적합.
- **IOCP** 대비
  - IOCP는 **완료 통지(completion)** 모델로, 대규모/고성능에 적합.
  - WSAEventSelect는 **준비 통지(readiness)** 모델로, 구조가 단순하고 학습/소규모에 적합.

---

## 2) 큰 그림 (동작 원리)

1. `WSACreateEvent()`로 **수동 리셋(manual-reset)** 이벤트 생성  
2. `WSAEventSelect(sock, hEvent, FD_READ | FD_WRITE | FD_CLOSE | …)`  
   - 호출 시 해당 소켓은 **자동으로 non-blocking** 모드로 전환됨
3. 메인 루프에서 `WSAWaitForMultipleEvents()`로 **여러 이벤트**를 대기
4. 깨어나면 `WSAEnumNetworkEvents(sock, hEvent, &ne)`로 **무슨 이벤트가 발생했는지** 질의  
   - 이 호출은 **네트워크 이벤트 상태를 초기화**(이벤트 오브젝트도 nonsignaled로) 해줌
5. 이벤트 종류별로 **반복해서 I/O 수행**  
   - 예: `FD_READ`면 `recv()`를 **EWOULDBLOCK**이 날 때까지 계속 호출
6. 소켓·이벤트 정리 시 `closesocket()`과 `WSACloseEvent()`를 각각 호출

> ⚠️ 준비 알림을 받았다고 **I/O가 꼭 1번에 끝나는 건 아님**. 내부 버퍼 상황에 따라 다시 `WSAEWOULDBLOCK`이 나올 수 있어 **루프**로 처리해야 함.

---

## 3) 감시 가능한 이벤트(일부)

- `FD_ACCEPT` : `listen` 소켓에 새 연결 도착
- `FD_CONNECT`: `connect()` 완료(성공/실패 모두 통지)
- `FD_READ`   : 수신 버퍼에 읽을 데이터 있음
- `FD_WRITE`  : 송신이 가능(이전 `WSAEWOULDBLOCK` 이후 재시도 타이밍)
- `FD_CLOSE`  : 연결 종료(정상/비정상 모두). 일반적으로 **마무리 recv(0) 확인 후 정리**

> 그 외 `FD_OOB`, QoS 관련 등도 있으나 일반 서버에선 위 5개가 핵심.

---

## 4) 서버 구조 예제 (에코 서버 스케치)

> **주의**: 한 쓰레드가 동시에 기다릴 수 있는 이벤트는 최대 **64개**.  
> 보통 `listen` 1개 + `client` 최대 63개로 샤딩(분할)하거나, 다중 쓰레드로 풀을 구성합니다.

```cpp
// build: link with ws2_32.lib
#include <winsock2.h>
#include <vector>
#include <string>
#include <iostream>
#pragma comment(lib, "ws2_32.lib")

struct SockEnt {
    SOCKET   sock;
    WSAEVENT evt;
    bool     isListen = false;
};

int main() {
    WSADATA wsa;  WSAStartup(MAKEWORD(2,2), &wsa);

    SOCKET listenSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    sockaddr_in addr{}; 
    addr.sin_family = AF_INET;
    addr.sin_port   = htons(9000);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    bind(listenSock, (sockaddr*)&addr, sizeof(addr));
    listen(listenSock, SOMAXCONN);

    // 이벤트 생성 및 관심 이벤트 설정
    WSAEVENT listenEvt = WSACreateEvent();
    WSAEventSelect(listenSock, listenEvt, FD_ACCEPT | FD_CLOSE);

    std::vector<SockEnt> ents;
    ents.push_back({listenSock, listenEvt, true});

    while (true) {
        // 1) 이벤트 핸들 배열 구성
        std::vector<HANDLE> hs; hs.reserve(ents.size());
        for (auto& e : ents) hs.push_back(e.evt);

        // 2) 대기 (무한대기)
        DWORD idx = WSAWaitForMultipleEvents(
            (DWORD)hs.size(), hs.data(), FALSE, WSA_INFINITE, FALSE);

        if (idx == WSA_WAIT_FAILED) break;

        // 3) 어떤 이벤트가 깨어났는지 확인 (여러 개일 수도 있어 반복 처리)
        DWORD start = idx - WSA_WAIT_EVENT_0; // 깨어난 인덱스
        for (DWORD i = start; i < hs.size(); ++i) {
            // signaled 확인
            DWORD r = WSAWaitForMultipleEvents(1, &hs[i], TRUE, 0, FALSE);
            if (r != WSA_WAIT_EVENT_0) continue;

            auto& ent = ents[i];
            WSANETWORKEVENTS ne{};
            // 4) 어떤 네트워크 이벤트가 발생했는지 열거
            WSAEnumNetworkEvents(ent.sock, ent.evt, &ne);

            if (ent.isListen) {
                // FD_ACCEPT 처리
                if (ne.lNetworkEvents & FD_ACCEPT) {
                    while (true) {
                        SOCKET c = accept(ent.sock, nullptr, nullptr);
                        if (c == INVALID_SOCKET) {
                            if (WSAGetLastError() == WSAEWOULDBLOCK) break;
                            std::cerr << "accept error\n"; break;
                        }
                        WSAEVENT ev = WSACreateEvent();
                        // 클라 소켓은 READ/WRITE/CLOSE 감시
                        WSAEventSelect(c, ev, FD_READ | FD_WRITE | FD_CLOSE);
                        ents.push_back({c, ev, false});
                        std::cout << "client accepted\n";
                    }
                }
                if (ne.lNetworkEvents & FD_CLOSE) {
                    std::cout << "listen socket closed?\n";
                }
            } else {
                // 클라이언트 소켓 처리
                if (ne.lNetworkEvents & FD_READ) {
                    // recv를 EWOULDBLOCK까지 반복
                    while (true) {
                        char buf[1024];
                        int n = recv(ent.sock, buf, sizeof(buf), 0);
                        if (n > 0) {
                            // echo back (보낼 수 있을 때까지 시도)
                            int sent = 0;
                            while (sent < n) {
                                int s = send(ent.sock, buf + sent, n - sent, 0);
                                if (s > 0) { sent += s; }
                                else if (s == SOCKET_ERROR && WSAGetLastError() == WSAEWOULDBLOCK) {
                                    // FD_WRITE가 올 때 나머지 전송 재시도(보통은 send 큐/버퍼를 두고 관리)
                                    break;
                                } else {
                                    // 오류 → 정리
                                    closesocket(ent.sock);
                                    WSACloseEvent(ent.evt);
                                    ents.erase(ents.begin() + i);
                                    --i;
                                    goto next_event; // 바깥 for 정렬 때문에 빠져나감
                                }
                            }
                        } else if (n == 0) {
                            // 상대가 종료
                            closesocket(ent.sock);
                            WSACloseEvent(ent.evt);
                            ents.erase(ents.begin() + i);
                            --i;
                            break;
                        } else {
                            if (WSAGetLastError() == WSAEWOULDBLOCK) break;
                            // 기타 오류 → 정리
                            closesocket(ent.sock);
                            WSACloseEvent(ent.evt);
                            ents.erase(ents.begin() + i);
                            --i;
                            break;
                        }
                    }
                }
                if (ne.lNetworkEvents & FD_WRITE) {
                    // 이전에 WSAEWOULDBLOCK났던 송신 재시도 (일반적으로는 송신 큐에서 빼며 전송)
                    // 여기서는 데모라 생략
                }
                if (ne.lNetworkEvents & FD_CLOSE) {
                    closesocket(ent.sock);
                    WSACloseEvent(ent.evt);
                    ents.erase(ents.begin() + i);
                    --i;
                }
            }
        next_event:;
        }
    }

    // 정리
    for (auto& e : ents) {
        closesocket(e.sock);
        WSACloseEvent(e.evt);
    }
    WSACleanup();
    return 0;
}
````

> 포인트
>
> * `WSAEnumNetworkEvents()`는 **네트워크 이벤트 상태를 리셋**하므로, 별도의 `WSAResetEvent()`는 보통 필요 없음.
> * `recv()`/`send()`는 **반복 호출**해 버퍼를 비우거나 채워야 함(끝 조건: 에러 `WSAEWOULDBLOCK`, 0, 기타 오류).
> * `FD_ACCEPT/FD_READ`는 **루프**로 **여러 건**을 한 번에 처리해야 누락이 없음.

---

## 5) 고급 팁 & 흔한 실수

### ✅ 올바른 패턴

* **읽기**: `FD_READ` 수신 → `recv()`를 **EWOULDBLOCK**까지 반복
* **쓰기**: `send()`가 `WSAEWOULDBLOCK`이면 **송신 큐**에 남기고 **FD\_WRITE**가 올 때 재시도
* **Accept**: `FD_ACCEPT`에서 `accept()`를 **EWOULDBLOCK**까지 반복
* **Close**: `FD_CLOSE`에서 정리(필요시 `recv(…,0)`로 확인). `WSACloseEvent()` **누락 금지**

### ❌ 흔한 실수

* **이벤트 64개 제한** 무시 → 한 쓰레드가 너무 많은 소켓 감시
  → 소켓을 **샤딩**하거나 **스레드를 여러 개** 두어 분산
* **준비 알림 1회 = I/O 1회**로 오해
  → 항상 **루프**로 처리해야 함
* **여러 소켓에 같은 이벤트 핸들** 공유
  → 소켓마다 **별도 WSAEVENT** 필요
* **WSAEventSelect 호출 후에도 blocking 호출 사용**
  → 소켓은 **non-blocking**으로 바뀌므로 blocking 호출을 기대하면 안 됨
* **핸들 누수**: `closesocket()`만 하고 `WSACloseEvent()`를 빼먹음

---

## 6) Select, WSAEventSelect, IOCP 비교 요약

| 모델                 | 타입         | 장점                   | 단점                      | 규모   |
| ------------------ | ---------- | -------------------- | ----------------------- | ---- |
| `select()`         | readiness  | 구현 단순, 학습 쉬움         | `fd_set` 재구성, 성능/스케일 한계 | 소규모  |
| **WSAEventSelect** | readiness  | 커널 이벤트 대기, 불필요 폴링 감소 | **이벤트 64개 제한**, 대규모 부적합 | 소\~중 |
| IOCP               | completion | 대규모/고성능, 스레드풀 연동     | 구현 난도 높음                | 대규모  |

---

## 7) 언제 쓰나?

* 학습 단계/프로토타입, 또는 **수십 개 수준의 연결** 처리
* 윈도우 전용 환경에서 **이벤트/핸들 기반** 아키텍처가 이미 있는 경우
* MMORPG 같은 **수천\~수만 연결** 목표라면 **IOCP**가 정석

---

## 8) 트러블슈팅 체크리스트

* `WSAEventSelect(sock, nullptr, 0)`으로 **비동기 해제**하면 소켓은 **다시 blocking** 상태로
* `FD_CONNECT` 발생 시 `ne.iErrorCode[FD_CONNECT_BIT]` 확인 (연결 실패도 이 경로로 옴)
* CPU 스파이크 시:

  * 루프에서 **이벤트 없이** `send/recv` 재시도하는 폴링이 있는지 확인
  * **타임아웃**을 적절히 사용하고, **이벤트 재설정** 로직 점검
* **모니터링 단위**를 64개씩 샤딩 → 각 워커가 `WSAWaitForMultipleEvents`로 담당

---

## 9) 미니 예: 클라이언트 연결 (FD\_CONNECT/FD\_WRITE)

```cpp
SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
WSAEVENT ev = WSACreateEvent();
WSAEventSelect(s, ev, FD_CONNECT | FD_WRITE | FD_CLOSE);

sockaddr_in srv{}; srv.sin_family = AF_INET;
srv.sin_port = htons(9000);
srv.sin_addr.s_addr = inet_addr("127.0.0.1");

// non-blocking이므로 바로 완료되지 않을 수 있음
int r = connect(s, (sockaddr*)&srv, sizeof(srv));
if (r == SOCKET_ERROR && WSAGetLastError() != WSAEWOULDBLOCK) {
    // 즉시 실패
}

// 이벤트 대기 후
WSANETWORKEVENTS ne{};
WSAWaitForMultipleEvents(1, &ev, TRUE, INFINITE, FALSE);
WSAEnumNetworkEvents(s, ev, &ne);

if (ne.lNetworkEvents & FD_CONNECT) {
    int ec = ne.iErrorCode[FD_CONNECT_BIT];
    if (ec == 0) {
        // 연결 성공 → FD_WRITE로 초기 전송 타이밍 잡기
    } else {
        // 연결 실패(ec에 에러 코드)
    }
}
```

---

## 🔚 요약

* **WSAEventSelect = readiness 모델 + 커널 이벤트**
* `WSAEventSelect()`를 호출하면 소켓은 **non-blocking**으로 바뀜
* `WSAWaitForMultipleEvents()`로 여러 소켓의 이벤트를 기다리고,
  `WSAEnumNetworkEvents()`로 **어떤 이벤트인지**를 열거한 뒤,
  **반복 I/O**(EWOULDBLOCK까지) 수행
* 이벤트 64개 제한 때문에 **샤딩/워커 분할**이 필요하며,
  **대규모 서버**는 **IOCP**로 가는 것이 정석
