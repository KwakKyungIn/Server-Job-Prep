# 🖧 Blocking Socket vs Non-Blocking Socket

네트워크 프로그래밍에서 **소켓(socket)**은 입출력(I/O)의 기본 단위다.  
소켓은 데이터를 송수신할 때 **동작 방식**에 따라 **Blocking**과 **Non-Blocking**으로 나눌 수 있다.  

---

## 1) Blocking Socket (기본값)

### 특징
- 소켓 함수 호출(`recv`, `accept`, `connect` 등)이 **데이터를 받을 때까지 무한 대기**한다.
- 데이터가 없으면 함수가 리턴하지 않음 → 해당 스레드가 **멈춘다**.
- 가장 단순하고 구현이 쉽지만, 성능 면에서 비효율적일 수 있음.
- 서버에서 클라이언트 수가 많아지면, 각 요청마다 스레드를 만들어야 하는 문제가 발생.

### 예시 (Blocking recv)
```cpp
char buffer[1024];
int recvLen = recv(clientSocket, buffer, sizeof(buffer), 0);
// 데이터가 도착할 때까지 여기서 멈춤!
````

---

## 2) Non-Blocking Socket

### 특징

* 소켓 함수 호출 시, **즉시 리턴**한다.
* 데이터가 없을 경우 → `-1` 반환, `WSAEWOULDBLOCK` (윈도우) 또는 `EWOULDBLOCK` (리눅스) 오류 코드 발생.
* **스레드가 멈추지 않고 다른 작업을 계속 수행 가능**.
* 다만, 지속적으로 반복 체크(폴링, busy-wait)가 필요해 CPU 낭비가 발생할 수 있음 → 보통 `select`, `epoll`, `IOCP` 같은 모델과 함께 사용.

### 예시 (Non-Blocking recv)

```cpp
u_long mode = 1; 
ioctlsocket(clientSocket, FIONBIO, &mode); // Non-blocking 모드 설정

char buffer[1024];
int recvLen = recv(clientSocket, buffer, sizeof(buffer), 0);
if (recvLen == SOCKET_ERROR)
{
    int err = WSAGetLastError();
    if (err == WSAEWOULDBLOCK) {
        // 받을 데이터 없음 (즉시 리턴)
    }
}
```

---

## 3) 비교 정리

| 구분      | Blocking Socket | Non-Blocking Socket  |
| ------- | --------------- | -------------------- |
| 함수 호출   | 데이터 도착할 때까지 대기  | 데이터 없으면 즉시 리턴        |
| 구현 난이도  | 쉬움              | 상대적으로 복잡             |
| CPU 사용률 | 낮음 (스레드 대기)     | 높을 수 있음 (바쁘게 체크해야 함) |
| 적합한 상황  | 소규모 연결, 간단한 서버  | 대규모 연결, 고성능 서버       |

---

## 4) 서버 프로그래밍에서의 활용

* **Blocking Socket**

  * 간단한 테스트, 소규모 서버 (예: 학습용 TCP 채팅 서버)
  * 클라이언트 수가 적을 때는 오히려 직관적이고 관리하기 편함
* **Non-Blocking Socket**

  * 대규모 접속 처리 시 필수
  * 보통 단독으로 쓰이지 않고 `select`, `epoll`, `kqueue`, `IOCP`와 함께 사용

---

## 🔚 요약

* **Blocking**: 데이터가 올 때까지 멈춤 (간단하지만 비효율적).
* **Non-Blocking**: 데이터 없으면 즉시 리턴 (효율적이지만 관리가 필요).
* 실무에서는 \*\*Non-Blocking + 이벤트 기반 모델(IOCP 등)\*\*을 조합해서 대규모 서버를 만든다.

