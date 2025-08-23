# 🌐 TCP vs UDP (소켓 프로그래밍 관점 정리)

게임 서버를 포함한 네트워크 프로그래밍에서 가장 기초적이면서 중요한 부분이 **TCP와 UDP**의 차이를 이해하는 것이다.  
둘 다 IP 위에서 동작하는 **전송 계층 프로토콜**이지만, 특성과 활용 목적이 완전히 다르다.

---

## 🧭 핵심 차이 한눈에 보기

| 구분 | TCP (Transmission Control Protocol) | UDP (User Datagram Protocol) |
|---|---|---|
| 연결(Connection) | 연결 지향적 (3-way handshake) | 비연결형 (연결 절차 없음) |
| 신뢰성(Reliability) | 데이터 **순서 보장**, **재전송 보장** | 보장 없음 (손실/순서 뒤바뀜 가능) |
| 흐름 제어 | 있음 (슬라이딩 윈도우, 혼잡 제어) | 없음 |
| 속도 | 상대적으로 느림 (오버헤드 존재) | 빠름 (헤더 작고 오버헤드 적음) |
| 메시지 단위 | 스트림(Stream) 기반 (경계 없음) | 메시지(Datagram) 기반 (경계 유지) |
| 사용 예시 | 웹(HTTP/HTTPS), 파일 전송, DB 연결, 채팅 | 게임 실시간 위치/상태, 스트리밍, DNS 쿼리 |

---

## 1) TCP (연결 지향, 신뢰성 보장)

### 특징
- **3-way handshake**를 통해 연결을 맺고, 연결 종료 시에도 절차 필요(4-way).
- **순서 보장**: 데이터가 네트워크에서 뒤섞여 도착해도 순서대로 재조립.
- **재전송**: 손실된 패킷은 자동으로 재전송.
- **흐름 제어 / 혼잡 제어**: 송신자와 수신자의 처리 속도에 맞춰 전송량 조절.
- **스트림 지향**: 연속된 바이트 스트림으로 취급, 메시지 경계가 없음.

### 예시 코드 (TCP 서버)
```cpp
int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
bind(serverSocket, (sockaddr*)&addr, sizeof(addr));
listen(serverSocket, SOMAXCONN);

int clientSocket = accept(serverSocket, (sockaddr*)&clientAddr, &len);
send(clientSocket, "Hello TCP", 9, 0);
recv(clientSocket, buffer, sizeof(buffer), 0);
````

---

## 2) UDP (비연결형, 빠른 전송)

### 특징

* **연결 절차 없음**: `socket` → `bind` 후 바로 `sendto`/`recvfrom` 사용 가능.
* **신뢰성 없음**: 패킷 손실 가능, 순서도 뒤바뀔 수 있음.
* **메시지 지향**: 보낸 메시지 단위 그대로 도착(경계 유지).
* **오버헤드 작음**: 헤더가 8바이트로 매우 작음 → 빠르고 가볍다.
* **멀티캐스트/브로드캐스트** 지원.

### 예시 코드 (UDP 서버)

```cpp
int udpSocket = socket(AF_INET, SOCK_DGRAM, 0);
bind(udpSocket, (sockaddr*)&addr, sizeof(addr));

recvfrom(udpSocket, buffer, sizeof(buffer), 0, (sockaddr*)&clientAddr, &len);
sendto(udpSocket, "Hello UDP", 9, 0, (sockaddr*)&clientAddr, len);
```

---

## 3) 게임 서버에서의 활용

* **TCP**

  * 로그인, 채팅, 거래, 아이템 정보 전송 등 **정확성이 중요한 데이터**에 적합.
  * 연결 상태 관리, 세션 관리가 필요.
* **UDP**

  * 캐릭터 위치, 총알/스킬 좌표, 실시간 동기화 같은 **빠른 전송이 중요한 데이터**에 적합.
  * 패킷 손실 시 최신 값으로 덮어쓰면 되므로 신뢰성보다 속도가 더 중요.

> 대형 MMORPG나 FPS 서버는 보통 **TCP + UDP 혼용**
>
> * TCP: 신뢰성 있는 제어/데이터
> * UDP: 실시간 전송 (RTT 최소화)

---

## 4) 정리 & 면접 포인트

* TCP는 **신뢰성 보장**을 위해 오버헤드가 많아 **속도가 느린 편**.
* UDP는 **빠르지만 신뢰성이 없다** → **애플리케이션 계층에서 보완** 가능 (재전송 구현, ACK, 시퀀스 번호).
* TCP는 **스트림 지향**, UDP는 **메시지 지향** → 프로그래밍 시 처리 방식 달라짐.
* **게임 서버**에서는 두 방식을 혼합하는 경우가 많다.

---

## 🔚 요약

* TCP: 신뢰성, 순서 보장, 연결 지향. (무거움)
* UDP: 빠름, 경량, 신뢰성 없음, 메시지 단위.
* 서버 프로그래밍에서는 **용도에 맞춰 선택**하고, 필요한 경우 직접 보완 로직을 작성해야 한다.

