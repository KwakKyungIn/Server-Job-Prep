# 🖧 Select 모델 정리

## 1. Select 모델이란?

- **Blocking/Non-Blocking 소켓의 한계**를 보완하기 위해 등장한 멀티플렉싱(Multiplexing) 모델.  
- 여러 개의 소켓을 **동시에 감시**하여 "읽을 준비가 되었는지", "쓸 준비가 되었는지", "예외가 발생했는지"를 확인할 수 있다.  
- 즉, **하나의 스레드로 다수의 소켓 I/O를 관리 가능** → 대규모 클라이언트 처리의 기초.  

---

## 2. 왜 필요한가?

### 문제점 (Non-Blocking 단독 사용 시)
```cpp
// Non-blocking socket
char buffer[1024];
int recvLen = recv(sock, buffer, sizeof(buffer), 0);
if (recvLen == SOCKET_ERROR) {
    if (WSAGetLastError() == WSAEWOULDBLOCK) {
        // 아직 데이터가 없음 → 반복적으로 계속 확인해야 함
    }
}
````

* 모든 소켓을 **계속 반복해서 체크**해야 함 (busy-wait) → CPU 사용률 급증.

---

### 해결책 (Select)

* `select()`는 **운영체제가 대신 감시**해줌.
* "준비된 소켓"만 알려주므로 불필요한 반복 검사 줄어듦.
* 즉, **효율적인 이벤트 감지** 가능.

---

## 3. 기본 함수 원리

```cpp
int select(
    int nfds,              // 검사할 소켓 개수 (윈도우에서는 무시됨)
    fd_set* readfds,       // 읽기 이벤트 감시 집합
    fd_set* writefds,      // 쓰기 이벤트 감시 집합
    fd_set* exceptfds,     // 예외 이벤트 감시 집합
    const struct timeval* timeout // 타임아웃
);
```

* `fd_set`: 소켓 집합(최대 FD\_SETSIZE개)
* `timeout`:

  * `NULL` → 무한 대기
  * `0` → 바로 리턴 (비동기처럼 동작)
  * 특정 값 → 지정 시간만 대기

---

## 4. 사용 예시

### (1) 서버 기본 구조

```cpp
#include <WinSock2.h>
#include <iostream>
using namespace std;

int main() {
    WSADATA wsa;
    WSAStartup(MAKEWORD(2,2), &wsa);

    SOCKET listenSock = socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(9000);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    bind(listenSock, (sockaddr*)&serverAddr, sizeof(serverAddr));
    listen(listenSock, SOMAXCONN);

    fd_set readSet, copySet;
    FD_ZERO(&readSet);
    FD_SET(listenSock, &readSet);

    while (true) {
        copySet = readSet; // select는 집합을 수정하므로 복사 필요
        int eventCount = select(0, &copySet, nullptr, nullptr, nullptr);

        for (int i = 0; i < eventCount; i++) {
            SOCKET sock = copySet.fd_array[i];
            if (sock == listenSock) {
                // 새 클라이언트 접속
                SOCKET client = accept(listenSock, nullptr, nullptr);
                FD_SET(client, &readSet);
                cout << "New client connected!" << endl;
            } else {
                // 클라이언트 데이터 수신
                char buffer[1024];
                int len = recv(sock, buffer, sizeof(buffer), 0);
                if (len <= 0) {
                    // 연결 끊김
                    closesocket(sock);
                    FD_CLR(sock, &readSet);
                    cout << "Client disconnected" << endl;
                } else {
                    buffer[len] = '\0';
                    cout << "Recv: " << buffer << endl;
                }
            }
        }
    }
    WSACleanup();
}
```

---

### (2) timeout 활용

```cpp
struct timeval tv;
tv.tv_sec = 5;
tv.tv_usec = 0;

int ret = select(0, &copySet, nullptr, nullptr, &tv);
if (ret == 0) {
    cout << "5초 동안 이벤트 없음" << endl;
}
```

---

## 5. 장단점

### ✅ 장점

* Non-blocking의 busy-wait 문제 해결.
* 하나의 스레드로 다수 소켓 감시 가능.
* 구현 난이도가 낮음 → 학습용, 소규모 서버에 적합.

### ❌ 단점

* `fd_set`의 크기 제한 (`FD_SETSIZE`, 보통 64\~1024).
* 소켓 개수가 많아질수록 성능 저하 (매번 전체 소켓 집합 복사 & 검사).
* 대규모 서버에는 부적합 → epoll(Linux), IOCP(Windows) 같은 고성능 모델 필요.

---

## 6. 서버 프로그래밍에서 Select의 의미

* **입문 단계의 핵심**: "다수의 클라이언트 연결을 한 스레드로 관리할 수 있다"는 개념을 배우는 것.
* 실무 MMORPG 서버에서는 거의 쓰지 않지만, **이후 IOCP, epoll을 이해하는 기초**가 된다.

---

## 🔚 요약

* **Blocking**: 한 소켓 → 대기.
* **Non-Blocking**: 모든 소켓 → 계속 체크 (CPU 낭비).
* **Select**: OS가 대신 감시 → 준비된 소켓만 알려줌.
* 다만 **성능 제한**이 있으므로, 실제 대규모 서버에서는 IOCP, epoll로 넘어가야 한다.
