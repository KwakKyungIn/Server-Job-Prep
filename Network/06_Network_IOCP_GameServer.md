# ⚙️ Windows I/O Completion Port (IOCP) — 게임 서버를 위한 최종 완성형 I/O 모델

> **IOCP**는 윈도우에서 제공하는 **완료 통지 기반(Completion-based)** 비동기 I/O 큐입니다.  
> 수천~수만 개의 소켓 I/O를 **커널이 수행**하고, **완료 이벤트만**을 고성능 큐로 모아 **워커 스레드**에게 전달합니다.  
> 대규모 게임 서버(특히 MMORPG)의 **핵심 인프라**로 쓰이며, Overlapped I/O의 **정석적인 상위 구조**입니다.

---

## 🧭 왜 IOCP인가

- **진짜 비동기**: `WSARecv/WSASend/AcceptEx` 등 I/O는 커널이 수행 → 사용자 스레드는 즉시 복귀.
- **확장성**: 이벤트 핸들 64개 제한(WSAEventSelect) 없음. O(N) 스캔(Select) 없음.
- **스레드 스로틀링**: 포트에 설정한 **concurrency**만큼 커널이 워커를 깨움 → 과도한 컨텍스트 스위칭 방지.
- **단일 완료 큐**: 모든 소켓/파일 핸들의 완료를 한 큐로 집중 → **작업 분배/스케줄링** 용이.
- **게임 서버 친화**: per-connection 상태머신 + 송수신 큐 + 타이머/잡큐를 IOCP 루프에 자연스럽게 결합 가능.

> 비교:  
> - `select()/WSAEventSelect` = *readiness(준비됨 통지)*, 매번 핸들 재등록/스캔, 스케일 한계.  
> - **IOCP** = *completion(완료 통지)*, 커널이 끝낸 일만 꺼내 처리 → 캐시 친화/낭비 최소.

---

## 🧱 핵심 구성요소

### 1) Completion Port 생성/연결
- `CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, concurrency);` → **포트 생성**
- `CreateIoCompletionPort((HANDLE)socket, iocp, (ULONG_PTR)completionKey, 0);` → **핸들을 포트에 연결**
  - `completionKey`는 보통 **연결 컨텍스트(Session*)** 포인터를 전달한다.

### 2) Overlapped I/O 게시
- `WSARecv/WSASend/AcceptEx/ConnectEx` 호출 시 `LPWSAOVERLAPPED` 제공
- 반환이 `SOCKET_ERROR && GetLastError()==WSA_IO_PENDING`이면 **정상적으로 비동기 진행 중**
- **중요**: `OVERLAPPED`와 버퍼는 **완료까지 살아 있어야 한다**(힙/멤버로 관리)

### 3) 완료 수거 루프
- `GetQueuedCompletionStatus(iocp, &bytes, &key, &pOverlapped, INFINITE);`
  - 완료 시: `bytes(전송/수신 바이트)`, `key(세션 포인터)`, `pOverlapped(어느 I/O인지)` 제공
  - 실패: 반환 FALSE → `GetLastError()`로 원인(연결 리셋 등) 확인
- 고성능 버전: `GetQueuedCompletionStatusEx()`로 **배치 수거** 가능

### 4) PostQueuedCompletionStatus
- 사용자 작업(예: **잡/타이머 이벤트**)을 IOCP 큐에 **인위적 완료 이벤트**로 푸시
- 워커 스레드가 **I/O와 동일한 경로**로 처리함 → **동일 일관성/락 정책** 유지

---

## 🧬 IOCP 스레드 모델 설계

- **워커 스레드 수**: 보통 **CPU 논리코어 수**(혹은 2×) 수준.  
  - `CreateIoCompletionPort`의 `NumberOfConcurrentThreads`는 **동시 실행 상한**(0=시스템 기본값=코어 수).
- **역할 분리** (권장)
  - **Network I/O 워커**: IOCP 루프만 처리(수신/송신 완료, accept 완료)  
  - **게임 로직/DB 워커**: 별도 스레드풀 + 큐(필요 시 `PostQueuedCompletionStatus`로 IOCP로 결과 전달)
- **락 전략**
  - per-connection 단위로 **단일 스레드 소유권**을 유지하려면, 완료 이벤트를 **해당 세션 큐**로 보내고 **한 번에 하나**만 처리.
  - 또는 **lock-free MPSC** 큐와 **원자적 플래그**로 송신 파이프라인을 보장.

---

## 🧩 세션(연결) 컨텍스트 설계

```cpp
struct IoCtx {           // I/O 단위 컨텍스트 (OVERLAPPED 반드시 별도!)
    OVERLAPPED ov{};
    enum { IO_RECV, IO_SEND, IO_ACCEPT } type;
    WSABUF buf{};
    char storage[8192];  // 수신/송신/accept용 버퍼
};

struct Session {
    SOCKET s = INVALID_SOCKET;
    std::atomic<int> ref{1};        // 수명 관리 (I/O 진행 중 +1)
    std::mutex sendLock;
    std::deque<std::vector<char>> sendQ;
    bool sending = false;            // 현재 진행 중인 WSASend 여부
    // 수신 링버퍼/파서 등…
};
````

* **포인트**

  * **I/O마다 OVERLAPPED는 고유**해야 함(동시에 두 I/O가 같은 OVERLAPPED 쓰면 터짐).
  * **ref-count**로 세션 수명을 보장: I/O 게시 시 +1, 완료 시 -1, 0이 되면 delete.
  * **sendQ**: **한 번에 하나의 WSASend**만 진행 → 순서 보장.

---

## 🏗️ 서버 부팅 절차 (수신/송신/AcceptEx)

1. **Winsock 초기화**

```cpp
WSADATA w; WSAStartup(MAKEWORD(2,2), &w);
```

2. **IOCP 생성**

```cpp
HANDLE iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, 0 /*기본=코어수*/);
```

3. **리스닝 소켓 + AcceptEx 준비**

```cpp
SOCKET ls = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, nullptr, 0, WSA_FLAG_OVERLAPPED);
sockaddr_in addr{}; addr.sin_family=AF_INET; addr.sin_port=htons(9000); addr.sin_addr.s_addr=htonl(INADDR_ANY);
bind(ls, (sockaddr*)&addr, sizeof(addr));
listen(ls, SOMAXCONN);

// IOCP에 리스너도 연결(accept 완료를 IOCP로 받기 위해)
CreateIoCompletionPort((HANDLE)ls, iocp, (ULONG_PTR)0 /*리슨 키*/, 0);

// AcceptEx 함수 포인터 조회
GUID guidAcceptEx = WSAID_ACCEPTEX;
LPFN_ACCEPTEX AcceptEx = nullptr;
DWORD bytes=0;
WSAIoctl(ls, SIO_GET_EXTENSION_FUNCTION_POINTER, &guidAcceptEx, sizeof(guidAcceptEx),
         &AcceptEx, sizeof(AcceptEx), &bytes, nullptr, nullptr);

// 미리 다수의 AcceptEx 게시(백로그)
const int kPreAccept = 512;
std::vector<IoCtx*> acceptIo;
acceptIo.reserve(kPreAccept);
for (int i=0;i<kPreAccept;i++) {
    auto* io = new IoCtx{};
    io->type = IoCtx::IO_ACCEPT;
    DWORD recvBytes = 0;
    // AcceptEx는 새 소켓도 미리 생성해 전해줘야 함
    SOCKET as = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, nullptr, 0, WSA_FLAG_OVERLAPPED);

    // 주소 정보 버퍼(로컬+원격 주소) 요구: 최소 2*(sizeof(sockaddr_in)+16)
    io->buf.buf = io->storage;
    io->buf.len = sizeof(io->storage);
    ZeroMemory(&io->ov, sizeof(io->ov));
    // AcceptEx(리스너, 신규소켓, 데이터버퍼, 수신바이트0, 주소버퍼, 주소버퍼, out bytes, overlapped)
    BOOL ok = AcceptEx(ls, as, io->buf.buf, 0,
        sizeof(sockaddr_in)+16, sizeof(sockaddr_in)+16,
        &recvBytes, &io->ov);
    if (!ok && WSAGetLastError() != WSA_IO_PENDING) {
        closesocket(as); delete io; continue;
    }
    // 새 소켓 핸들을 ov 내부에 저장할 필요가 있으면 IoCtx에 필드 추가
    *((SOCKET*)&io->storage[0]) = as; // (예시) 헤더로 숨겨두기
    acceptIo.push_back(io);
}
```

4. **워커 스레드 풀 생성 (IOCP 루프)**

```cpp
auto worker = [iocp]() {
    while (true) {
        DWORD bytes=0; ULONG_PTR key=0; OVERLAPPED* ov=nullptr;
        BOOL ok = GetQueuedCompletionStatus(iocp, &bytes, &key, &ov, INFINITE);
        if (ov == nullptr && ok) {
            // 종료 신호 등 커스텀
            break;
        }
        // 에러 여부
        DWORD err = ok ? 0 : GetLastError();

        // completionKey로 세션/리스너 구분
        if (key == 0) {
            // 리슨 관련(IoCtx::IO_ACCEPT)
            auto* io = CONTAINING_RECORD(ov, IoCtx, ov);
            SOCKET as = *((SOCKET*)&io->storage[0]);

            if (err != 0) { closesocket(as); delete io; continue; }

            // SO_UPDATE_ACCEPT_CONTEXT: 리슨 정보 연결
            setsockopt(as, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT, (char*)&ls, sizeof(ls));
            // (선택) TCP_NODELAY, KEEPALIVE, 버퍼 크기 등 설정

            // 세션 생성 + IOCP 연결
            auto* sess = new Session{};
            sess->s = as;
            CreateIoCompletionPort((HANDLE)as, iocp, (ULONG_PTR)sess, 0);

            // 첫 수신 게시
            auto* rx = new IoCtx{};
            rx->type = IoCtx::IO_RECV;
            rx->buf.buf = rx->storage; rx->buf.len = sizeof(rx->storage);
            DWORD flags=0, recvd=0;
            int r = WSARecv(as, &rx->buf, 1, &recvd, &flags, &rx->ov, nullptr);
            if (r==SOCKET_ERROR && WSAGetLastError()!=WSA_IO_PENDING) {
                closesocket(as); delete rx; delete sess;
            }

            // 다음 AcceptEx 재게시(백로그 유지)
            ZeroMemory(&io->ov, sizeof(io->ov));
            DWORD dummy=0;
            SOCKET nas = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, nullptr, 0, WSA_FLAG_OVERLAPPED);
            *((SOCKET*)&io->storage[0]) = nas;
            BOOL ok2 = AcceptEx(ls, nas, io->buf.buf, 0,
                sizeof(sockaddr_in)+16, sizeof(sockaddr_in)+16, &dummy, &io->ov);
            if (!ok2 && WSAGetLastError()!=WSA_IO_PENDING) {
                closesocket(nas); delete io; // 백로그 하나 줄어듦
            }
            continue;
        }

        // 세션 I/O
        auto* sess = (Session*)key;
        auto* io   = CONTAINING_RECORD(ov, IoCtx, ov);

        if (err != 0 || bytes == 0) {
            // 종료/에러
            closesocket(sess->s);
            delete io; delete sess;
            continue;
        }

        if (io->type == IoCtx::IO_RECV) {
            // 수신 데이터 처리(패킷 파싱/명령)
            // … 처리 후 다음 수신 게시
            auto* rx = io; // 재사용(주의: 동시에 재사용 금지. 여기선 완료 후 즉시 재게시)
            ZeroMemory(&rx->ov, sizeof(rx->ov));
            rx->buf.buf = rx->storage; rx->buf.len = sizeof(rx->storage);
            DWORD flags=0, recvd=0;
            int r = WSARecv(sess->s, &rx->buf, 1, &recvd, &flags, &rx->ov, nullptr);
            if (r==SOCKET_ERROR && WSAGetLastError()!=WSA_IO_PENDING) {
                closesocket(sess->s); delete rx; delete sess;
            }
        } else if (io->type == IoCtx::IO_SEND) {
            // 다음 송신이 있으면 이어서 게시
            std::unique_lock lk(sess->sendLock);
            if (!sess->sendQ.empty()) {
                auto data = std::move(sess->sendQ.front()); sess->sendQ.pop_front();
                auto* tx = io; // 재사용
                ZeroMemory(&tx->ov, sizeof(tx->ov));
                memcpy(tx->storage, data.data(), data.size());
                tx->buf.buf = tx->storage; tx->buf.len = (ULONG)data.size();
                int r = WSASend(sess->s, &tx->buf, 1, nullptr, 0, &tx->ov, nullptr);
                if (r==SOCKET_ERROR && WSAGetLastError()!=WSA_IO_PENDING) {
                    lk.unlock(); closesocket(sess->s); delete tx; delete sess;
                }
            } else {
                sess->sending = false;
                delete io; // 보내기 컨텍스트 반환(풀 사용 권장)
            }
        }
    }
};

// 워커 실행
SYSTEM_INFO si; GetSystemInfo(&si);
int threads = (int)si.dwNumberOfProcessors;
std::vector<std::thread> pool;
for (int i=0;i<threads;i++) pool.emplace_back(worker);
for (auto& t: pool) t.join();
```

> 실전 팁
>
> * **AcceptEx 백로그**는 **항상 채워** 두세요(네트워크 스파이크 흡수).
> * `WSARecv`는 **항상 1개 이상 outstanding**(파이프라인 유지).
> * `WSASend`는 **동시에 1개**만 → 전송 순서/혼잡 제어에 유리.
> * **부분 전송/수신**을 기본 가정(패킷 프레이밍/링버퍼 필수).
> * 에러 코드 예: `WSAECONNRESET(10054)`, `ERROR_NETNAME_DELETED(64)`.

---

## 🧪 송신 파이프라인(순서 보장) 패턴

```cpp
void Send(Session* s, const void* p, size_t n) {
    std::vector<char> buf((const char*)p, (const char*)p+n);
    std::unique_lock lk(s->sendLock);
    if (s->sending) {
        s->sendQ.push_back(std::move(buf));
        return;
    }
    s->sending = true;

    auto* tx = new IoCtx{};
    tx->type = IoCtx::IO_SEND;
    memcpy(tx->storage, buf.data(), buf.size());
    tx->buf.buf = tx->storage; tx->buf.len = (ULONG)buf.size();
    int r = WSASend(s->s, &tx->buf, 1, nullptr, 0, &tx->ov, nullptr);
    if (r==SOCKET_ERROR && WSAGetLastError()!=WSA_IO_PENDING) {
        lk.unlock(); closesocket(s->s); delete tx; delete s;
    }
}
```

* **아이디어**: 송신 중이 아닐 때만 실제 `WSASend` 호출 → 완료 시 큐를 비우며 연속 송신.
* **장점**: 전송 순서 보장, 운영체제 호출 수 절약(묶음 전송 전략도 쉽게 추가 가능).

---

## 🧰 고급 옵션과 최적화

* **`GetQueuedCompletionStatusEx`**: 한 번의 호출로 **여러 완료**를 수거(배치 처리 → 캐시 효율↑).
* **`SetFileCompletionNotificationModes`**

  * `FILE_SKIP_COMPLETION_PORT_ON_SUCCESS`: **동기 완료** 시 IOCP로 통지하지 않음 → 큐 압력↓
  * 사용 시 **동기/비동기 둘 다** 처리 경로를 구현해야 함(난이도↑).
* **소켓 옵션**

  * `TCP_NODELAY`: 지연 ACK/Nagle 완화 → **인터랙티브 패킷**에 유리
  * `SO_KEEPALIVE`: 장기 연결 감지
  * `SO_RCVBUF/SO_SNDBUF`: 워크로드에 맞춰 조정(큰 패킷/대역폭).
* **메모리 풀/Lookaside**

  * `IoCtx`, 버퍼를 **풀링**해 힙 단편화/할당 오버헤드 감소.
* **수명/취소**

  * `CancelIoEx((HANDLE)s, &ov)`: 특정 I/O 취소
  * 닫기 직전 모든 outstanding I/O를 취소하거나, **ref-count==0**까지 대기
* **유저 잡 통합**

  * `PostQueuedCompletionStatus(iocp, 0, keyJob, nullptr)` → 워커가 동일 큐에서 잡 실행
  * **타이머**: 별도 타이머 스레드가 기한 도래 시 IOCP로 포스트

---

## 🧯 오류 & 종료 처리 원칙

* **bytes==0**: **정상 종료(half-close)** 로 간주 → 세션 종료 루틴 진입
* **GQCS FALSE**: `GetLastError()`로 구체 코드 확인 후 종료/재시도
* **중복 종료 방지**: 세션에 **원자적 state**(Open/Closing/Closed)와 **ref-count**로 안전 종료
* **콜백 재진입 금지**: 같은 세션에 대해 동시에 다수 워커가 접근하지 않도록 **큐/락/플래그**로 직렬화

---

## 🔒 패킷 프레이밍(Sticky Packet) 기본

TCP는 **메시지 경계가 없다** → 수신 버퍼에서

1. **헤더(길이/타입)** 를 파싱
2. **본문 길이만큼** 모였을 때 **하나의 패킷**으로 처리
3. 남은 데이터는 링버퍼에 보존

> IOCP는 **완료 시점만** 제공하며, **패킷 경계**는 **애플리케이션 레벨**에서 해결해야 함.

---

## 📦 최소 동작 예제: IOCP 에코 서버(요약본)

> 학습 목적으로 **핵심 흐름**만 유지한 요약버전입니다(실전용은 위 설계 패턴을 따르세요).

```cpp
// build: /D_WIN32_WINNT=0x0600, link ws2_32.lib, mswsock.lib
#include <winsock2.h>
#include <mswsock.h>
#include <windows.h>
#include <iostream>
#include <vector>
#include <deque>
#include <mutex>
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "mswsock.lib")

struct Io { OVERLAPPED ov{}; WSABUF buf; char data[8192]; enum { R, S, A } type; };
struct Sess {
    SOCKET s=INVALID_SOCKET;
    std::mutex m; std::deque<std::vector<char>> q; bool sending=false;
};

int main() {
    WSADATA w; WSAStartup(MAKEWORD(2,2), &w);
    HANDLE cp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, 0, 0, 0);

    SOCKET ls = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, 0, 0, WSA_FLAG_OVERLAPPED);
    sockaddr_in a{}; a.sin_family=AF_INET; a.sin_port=htons(9000); a.sin_addr.s_addr=htonl(INADDR_ANY);
    bind(ls, (sockaddr*)&a, sizeof(a)); listen(ls, SOMAXCONN);
    CreateIoCompletionPort((HANDLE)ls, cp, 0, 0);

    GUID gid = WSAID_ACCEPTEX; LPFN_ACCEPTEX AcceptEx=0; DWORD b=0;
    WSAIoctl(ls, SIO_GET_EXTENSION_FUNCTION_POINTER, &gid, sizeof(gid), &AcceptEx, sizeof(AcceptEx), &b, 0, 0);

    auto postAccept = [&](Io* io) {
        ZeroMemory(&io->ov, sizeof(io->ov)); io->type = Io::A;
        SOCKET as = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, 0, 0, WSA_FLAG_OVERLAPPED);
        *reinterpret_cast<SOCKET*>(io->data) = as;
        io->buf.buf = io->data; io->buf.len = sizeof(io->data);
        DWORD rcv=0;
        BOOL ok = AcceptEx(ls, as, io->buf.buf, 0, sizeof(sockaddr_in)+16, sizeof(sockaddr_in)+16, &rcv, &io->ov);
        return ok || WSAGetLastError()==WSA_IO_PENDING;
    };

    const int N=512; std::vector<Io*> accepts; accepts.reserve(N);
    for (int i=0;i<N;i++) { auto* io=new Io{}; postAccept(io); accepts.push_back(io); }

    SYSTEM_INFO si; GetSystemInfo(&si);
    int T = si.dwNumberOfProcessors;
    std::vector<std::thread> ths;
    for (int i=0;i<T;i++) ths.emplace_back([&]{
        while (true) {
            DWORD bytes=0; ULONG_PTR key=0; OVERLAPPED* ov=0;
            BOOL ok = GetQueuedCompletionStatus(cp, &bytes, &key, &ov, INFINITE);
            DWORD err = ok?0:GetLastError();
            if (!ov) break; // 종료

            Io* io = CONTAINING_RECORD(ov, Io, ov);
            if (io->type == Io::A) {
                SOCKET as = *reinterpret_cast<SOCKET*>(io->data);
                if (err) { closesocket(as); postAccept(io); continue; }
                setsockopt(as, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT, (char*)&ls, sizeof(ls));
                Sess* s = new Sess{};
                s->s = as; CreateIoCompletionPort((HANDLE)as, cp, (ULONG_PTR)s, 0);

                // 첫 Recv
                ZeroMemory(&io->ov, sizeof(io->ov)); io->type = Io::R;
                io->buf.buf = io->data; io->buf.len = sizeof(io->data);
                DWORD f=0;
                int r = WSARecv(as, &io->buf, 1, 0, &f, &io->ov, 0);
                if (r==SOCKET_ERROR && WSAGetLastError()!=WSA_IO_PENDING) { closesocket(as); delete s; delete io; }
                // Accept 백로그 유지
                auto* nx = new Io{}; postAccept(nx);
            } else if (io->type == Io::R) {
                Sess* s = (Sess*)key;
                if (err || bytes==0) { closesocket(s->s); delete s; delete io; continue; }
                // 에코 송신
                std::lock_guard lk(s->m);
                if (s->sending) {
                    s->q.emplace_back(io->data, io->data+bytes);
                    // 다음 Recv는 별도 Io 필요(실전: 풀에서 하나 더 꺼내기)
                } else {
                    s->sending = true;
                    io->type = Io::S;
                    io->buf.buf = io->data; io->buf.len = bytes;
                    int r = WSASend(s->s, &io->buf, 1, 0, 0, &io->ov, 0);
                    if (r==SOCKET_ERROR && WSAGetLastError()!=WSA_IO_PENDING) { closesocket(s->s); delete s; delete io; }
                }
                // 다음 Recv 게시(실전: 다른 Io 객체 사용)
                auto* rx=new Io{}; ZeroMemory(&rx->ov,sizeof(rx->ov)); rx->type=Io::R;
                rx->buf.buf = rx->data; rx->buf.len = sizeof(rx->data);
                DWORD f=0; int r = WSARecv(s->s, &rx->buf, 1, 0, &f, &rx->ov, 0);
                if (r==SOCKET_ERROR && WSAGetLastError()!=WSA_IO_PENDING) { closesocket(s->s); delete s; delete rx; }
            } else if (io->type == Io::S) {
                Sess* s = (Sess*)key;
                if (err) { closesocket(s->s); delete s; delete io; continue; }
                std::lock_guard lk(s->m);
                if (!s->q.empty()) {
                    auto msg = std::move(s->q.front()); s->q.pop_front();
                    memcpy(io->data, msg.data(), msg.size());
                    io->buf.buf=io->data; io->buf.len=(ULONG)msg.size();
                    int r = WSASend(s->s, &io->buf, 1, 0, 0, &io->ov, 0);
                    if (r==SOCKET_ERROR && WSAGetLastError()!=WSA_IO_PENDING) { closesocket(s->s); delete s; delete io; }
                } else {
                    s->sending = false; delete io;
                }
            }
        }
    });

    for (auto& t: ths) t.join();
    WSACleanup();
}
```

---

## 🧠 실전 체크리스트

* [ ] **핸들-IOCP 연결**: 리스너/클라 소켓 모두 `CreateIoCompletionPort` 호출
* [ ] **OVERLAPPED 수명**: I/O 완료 전까지 절대 파괴 금지(풀/힙)
* [ ] **AcceptEx 백로그**: 충분히 게시하여 스파이크 흡수
* [ ] **수신 파이프라인**: 항상 최소 1개 이상 수신 outstanding
* [ ] **송신 직렬화**: 한 번에 한 개 `WSASend`만, 나머지는 큐
* [ ] **패킷 프레이밍**: 헤더+바디 파서/링버퍼 구현
* [ ] **에러/종료 처리**: bytes==0 → 정상 종료, 에러코드 매핑
* [ ] **스레드 수**: 코어 수 기준(작업 성격에 따라 조정)
* [ ] **메모리 풀**: IoCtx/버퍼 풀링으로 힙 오버헤드 감소
* [ ] **잡/타이머 통합**: `PostQueuedCompletionStatus`로 동일 루트에서 처리

---

## ✅ 요약

* IOCP는 **대규모 동시접속**을 위한 윈도우 **최상위 I/O 모델**이다.
* **핸들 연결 → Overlapped 게시 → 완료 큐 수거**가 기본 골격이며,
  **per-connection 상태머신/송신 큐/프레이밍**이 실전 핵심이다.
* **스레드 스로틀링, 배치 수거, 메모리 풀, 잡 통합**으로 더 높은 성능/안정성을 달성하자.



