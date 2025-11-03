## 1. OSI 7계층에 대해 설명해보세요

<details close>
<summary><strong>💡 자세한 정답</strong></summary>

<div style="background-color:#f6f8fa; padding:15px; border-radius:10px;">

## 💡 **OSI 7계층 (Open Systems Interconnection Model)**

> 네트워크 통신 과정을 7단계로 나눈 개념적 모델로,
> 각 계층은 명확한 역할을 가지고 상하 계층 간 인터페이스를 통해 데이터를 전달합니다.

---

|  계층 |             이름            | 주요 역할                              | 대표 프로토콜 / 예시               |
| :-: | :-----------------------: | :--------------------------------- | :------------------------- |
|  7  |  **응용 계층 (Application)**  | 사용자와 직접 상호작용. 애플리케이션 서비스 제공.       | HTTP, FTP, SMTP, DNS       |
|  6  |  **표현 계층 (Presentation)** | 데이터 인코딩, 암호화, 압축 처리.               | JPEG, MPEG, SSL/TLS        |
|  5  |    **세션 계층 (Session)**    | 통신 세션 관리 (연결 수립, 유지, 종료).          | NetBIOS, RPC, SSL          |
|  4  |   **전송 계층 (Transport)**   | 데이터의 신뢰성 보장, 세그먼트 단위 전송, 흐름/오류 제어. | TCP, UDP                   |
|  3  |   **네트워크 계층 (Network)**   | 경로 설정(라우팅), IP 주소 관리.              | IP, ICMP, ARP, RIP         |
|  2  | **데이터 링크 계층 (Data Link)** | 프레임 단위 전송, MAC 주소 기반 통신, 오류 검출.    | Ethernet, PPP, Switch, MAC |
|  1  |    **물리 계층 (Physical)**   | 비트 전송, 전기적/기계적 신호 정의.              | 케이블, 허브, 리피터               |

---

### ⚙️ **1️⃣ 데이터 흐름 예시**

```
[송신 측]
응용 → 표현 → 세션 → 전송 → 네트워크 → 데이터링크 → 물리  
[수신 측]
물리 → 데이터링크 → 네트워크 → 전송 → 세션 → 표현 → 응용
```

→ 상위 계층의 데이터는 하위 계층으로 내려가며 **캡슐화(encapsulation)**
→ 수신 측은 반대로 **역캡슐화(decapsulation)** 과정을 거침.

---

### ⚙️ **2️⃣ 각 계층의 핵심 요약**

| 계층      | 주요 기능      | 키워드              |
| ------- | ---------- | ---------------- |
| 7 응용    | 사용자 서비스    | “사용자와 가장 가까운 계층” |
| 6 표현    | 형식 변환, 암호화 | “데이터 표현 방식 통일”   |
| 5 세션    | 연결 관리      | “통신의 시작과 끝 관리”   |
| 4 전송    | 신뢰성 보장     | “TCP/UDP 핵심 계층”  |
| 3 네트워크  | 주소 및 라우팅   | “IP 기반 경로 설정”    |
| 2 데이터링크 | 오류 제어, MAC | “스위치, MAC 주소”    |
| 1 물리    | 신호 전송      | “전선, 전기적 신호”     |

---

### ⚙️ **3️⃣ 트랜스포트 계층 예시**

```cpp
// TCP 예시 (신뢰성 보장)
send(), recv(), connect() → 연결 기반

// UDP 예시 (비연결형)
sendto(), recvfrom() → 빠르지만 신뢰성 없음
```

---

### ⚙️ **4️⃣ 세션/표현 계층 실사용 예**

* 세션 계층: SSL/TLS, RPC — “연결 유지 및 보안 세션 관리”
* 표현 계층: 데이터 암호화(SSL), 인코딩(JSON, XML, JPEG)

---

## 🎯 **면접용 요약**

> OSI 7계층은 네트워크 통신을 7단계로 나눈 개념 모델로,
> **상위는 사용자와 가까운 계층(응용/표현/세션)**,
> **하위는 데이터 전송 중심 계층(전송/네트워크/링크/물리)** 입니다.
>
> 전송 계층에서는 TCP/UDP,
> 세션 계층에서는 SSL/TLS가 대표적으로 사용됩니다.

---

### 🔥 꼬리질문 예상

1. **TCP와 UDP의 차이를 설명해보세요.**
   → 연결 지향 vs 비연결, 신뢰성 보장 vs 빠른 전송.

2. **OSI와 TCP/IP 모델의 차이는?**
   → TCP/IP는 4계층 구조(응용, 전송, 인터넷, 네트워크 접근).
   OSI는 이보다 세분화된 7계층 구조.

3. **세션 계층과 전송 계층의 차이는?**
   → 세션 계층은 연결 “관리”, 전송 계층은 데이터 “전달” 책임.

</details>


## 2. TCP와 UDP의 차이점은 무엇인가요?

<details close>
<summary><strong>💡 자세한 정답</strong></summary>

<div style="background-color:#f6f8fa; padding:15px; border-radius:10px;">

## 💡 **TCP vs UDP**

|         구분        | **TCP (Transmission Control Protocol)** | **UDP (User Datagram Protocol)** |
| :---------------: | :-------------------------------------- | :------------------------------- |
|     **연결 방식**     | 연결형 (Connection-oriented)               | 비연결형 (Connectionless)            |
|      **신뢰성**      | 높음 – 재전송, 순서 보장, 오류 제어                  | 낮음 – 보장 없음                       |
|     **순서 보장**     | 있음 (Sequence Number 사용)                 | 없음                               |
| **흐름 제어 / 혼잡 제어** | 있음 (Window, Congestion Control)         | 없음                               |
|       **속도**      | 느림 (제어 및 확인 과정 존재)                      | 빠름 (제어 과정 없음)                    |
|       **단위**      | 바이트 스트림 (연속된 데이터 흐름)                    | 메시지 단위 (Datagram)                |
|  **대표 프로토콜 / 용도** | HTTP, FTP, SMTP, Telnet                 | DNS, VoIP, 스트리밍, 게임 패킷           |
|     **헤더 크기**     | 20바이트 이상                                | 8바이트 (단순 구조)                     |

---

### ⚙️ **1️⃣ 연결 방식**

* **TCP**: 3-way handshake로 연결 수립, 4-way로 종료
  → 송신자와 수신자가 모두 준비된 상태에서만 통신
* **UDP**: handshake 없음, 바로 송신
  → “보내고 끝”, 수신 여부 확인 안 함

---

### ⚙️ **2️⃣ 신뢰성과 순서 보장**

```text
TCP:  패킷 손실 시 재전송 → 신뢰성 확보  
UDP:  손실 무시 → 속도 우선
```

```cpp
// TCP 예시 (신뢰성)
send(socket, data, size, 0);
recv(socket, buffer, size, 0);

// UDP 예시 (단순 송신)
sendto(sock, data, size, 0, (sockaddr*)&addr, len);
```

---

### ⚙️ **3️⃣ 속도와 효율성**

* **TCP**: 느리지만 신뢰성 필수 서비스에 적합
  → 예: 웹 통신(HTTP), 파일 전송(FTP), 이메일(SMTP)
* **UDP**: 빠르고 지연 최소화
  → 예: 실시간 스트리밍, 온라인 게임, DNS 조회

---

### ⚙️ **4️⃣ 전송 단위 차이**

* TCP는 **바이트 스트림 기반** → 데이터 경계 없음
  예: 여러 `send()`를 `recv()` 한 번으로 받을 수 있음
* UDP는 **메시지 단위 전송** → 패킷 단위 유지

---

## 🎯 **면접용 요약**

> **TCP는 신뢰성 중심, UDP는 속도 중심**입니다.
> TCP는 연결을 맺고 순서·재전송·흐름 제어로 안정적인 통신을 보장하지만 느립니다.
> UDP는 연결 없이 빠르게 전송하지만 순서 보장이나 재전송이 없습니다.
>
> → **TCP: 웹·파일 전송 / UDP: 게임·스트리밍**

---

### 🔥 꼬리질문 예상

1. **TCP의 3-way handshake 과정 설명해보세요.**
   SYN → SYN+ACK → ACK 순으로 연결 수립.

2. **UDP에서 패킷 손실 시 어떻게 처리하나요?**
   애플리케이션 레벨에서 직접 재전송 구현해야 함.

3. **게임 서버에서 UDP를 사용하는 이유는요?**
   약간의 손실보다 **지연(latency)** 이 더 중요하기 때문.

</details>

## 3. TCP의 3-way handshake 과정을 설명해보세요

<details close>
<summary><strong>💡 자세한 정답</strong></summary>

<div style="background-color:#f6f8fa; padding:15px; border-radius:10px;">

## 💡 **TCP 3-Way Handshake**

> TCP에서 **신뢰성 있는 연결을 수립하기 위한 3단계 절차**로,
> 클라이언트와 서버가 서로의 통신 가능 여부를 확인하고 **초기 시퀀스 번호(ISN)** 를 교환합니다.

---

### ⚙️ **1️⃣ 전체 흐름**

|       단계      | 송신자(클라이언트)                   | 수신자(서버)         | 의미                 |
| :-----------: | :--------------------------- | :-------------- | :----------------- |
|   **① SYN**   | SYN=1, Seq=x                 | 대기 중            | 연결 요청 (시작 번호 x 전송) |
| **② SYN+ACK** | SYN=1, ACK=1, Seq=y, Ack=x+1 | SYN 수신 후 응답     | 요청 수락 + 서버도 연결 요청  |
|   **③ ACK**   | ACK=1, Seq=x+1, Ack=y+1      | SYN+ACK 수신 후 응답 | 연결 확립 완료           |

---

### ⚙️ **2️⃣ 흐름 예시 다이어그램**

```
[Client]                                [Server]
   | -------- SYN (Seq=x) ------------>  |   // 연결 요청
   | <----- SYN+ACK (Seq=y, Ack=x+1) -- |   // 응답 + 요청 수락
   | -------- ACK (Seq=x+1, Ack=y+1) --> |   // 연결 확립 완료
```

→ 이 시점 이후, **양쪽 모두 연결이 성립**되어 데이터 송수신 가능.

---

### ⚙️ **3️⃣ 각 플래그의 의미**

|          플래그          | 의미                    |
| :-------------------: | :-------------------- |
| **SYN (Synchronize)** | 연결 요청 및 시퀀스 번호 동기화    |
| **ACK (Acknowledge)** | 상대방의 요청에 대한 응답(승인)    |
|        **Seq**        | 송신 측 데이터의 순서 정보       |
|        **Ack**        | 수신 측이 다음에 기대하는 Seq 번호 |

---

### ⚙️ **4️⃣ 4-Way Handshake (연결 종료)** *(참고)*

| 단계                 | 동작           |
| ------------------ | ------------ |
| ① FIN (클라이언트 → 서버) | 연결 종료 요청     |
| ② ACK (서버 → 클라이언트) | 종료 요청 수락     |
| ③ FIN (서버 → 클라이언트) | 서버도 종료 요청    |
| ④ ACK (클라이언트 → 서버) | 종료 수락, 연결 종료 |

---

## 🎯 **면접용 요약**

> TCP의 3-Way Handshake는 연결을 수립하기 위한 과정으로,
> **SYN → SYN+ACK → ACK** 순으로 진행됩니다.
>
> 1. 클라이언트가 SYN을 보내 연결 요청
> 2. 서버가 SYN+ACK로 응답
> 3. 클라이언트가 ACK로 확인
>
> 이 과정을 통해 양쪽 모두 통신 가능 여부와 초기 시퀀스를 동기화하고,
> 신뢰성 있는 연결이 확립됩니다.

---

### 🔥 꼬리질문 예상

1. **왜 2번이 아니라 3번이어야 하나요?**
   → 양쪽 모두 송·수신이 가능한지 “상호 확인”이 필요하기 때문.

2. **초기 시퀀스 번호(ISN)를 랜덤하게 설정하는 이유는요?**
   → 예측 공격(세션 하이재킹)을 방지하기 위해.

3. **3-Way Handshake 이후 바로 데이터 전송이 가능한가요?**
   → 예, 3번째 ACK 이후 연결이 확립되므로 바로 가능.

</details>

## 4. TCP의 4-Way Handshake(연결 종료)는 왜 4단계로 이루어지나요?

<details close>
<summary><strong>💡 자세한 정답</strong></summary>

<div style="background-color:#f6f8fa; padding:15px; border-radius:10px;">

## 💡 **TCP 4-Way Handshake (연결 종료 과정)**

> TCP 연결을 **양방향으로 안전하게 종료하기 위한 절차**로,
> 송신자와 수신자가 각각 **독립적으로 종료(half-close)** 를 수행하기 때문에
> 3단계가 아닌 **4단계(FIN → ACK → FIN → ACK)** 로 이루어집니다.

---

### ⚙️ **1️⃣ 전체 절차**

|     단계    | 송신자(Client) | 수신자(Server) | 설명                              |
| :-------: | :---------- | :---------- | :------------------------------ |
| **① FIN** | FIN=1       | 대기 중        | 클라이언트가 “데이터 다 보냈으니 연결 종료 요청”    |
| **② ACK** |             | ACK=1       | 서버가 FIN 수락, “아직 보낼 데이터 있을 수 있음” |
| **③ FIN** |             | FIN=1       | 서버도 송신 완료 후 종료 요청               |
| **④ ACK** | ACK=1       |             | 클라이언트가 종료 확인 후 연결 완전 종료         |

---

### ⚙️ **2️⃣ 흐름 다이어그램**

```
[Client]                                  [Server]
   | -------- FIN ------------>             |   // 클라이언트 송신 종료 요청
   | <------- ACK -------------             |   // 서버 수락 (아직 데이터 전송 가능)
   | <------- FIN -------------             |   // 서버도 송신 종료 요청
   | -------- ACK ------------>             |   // 클라이언트 수락 → 완전 종료
```

---

### ⚙️ **3️⃣ Half-Close 개념**

> TCP 연결은 **양방향(full-duplex)** 이므로
> 송신 방향과 수신 방향이 **독립적으로 종료**될 수 있습니다.

* **클라이언트 → 서버**: 더 이상 전송할 데이터 없음 → FIN
* **서버 → 클라이언트**: 아직 전송 중일 수 있음 → ACK 먼저 보냄
  → 나중에 자신도 전송 완료 시 FIN 송신

👉 즉, **송신과 수신 종료가 분리되어 두 번의 FIN이 필요**함.
이 때문에 총 4단계가 된다.

---

### ⚙️ **4️⃣ FIN과 ACK를 분리하는 이유**

1. **서버가 FIN 즉시 못 보낼 수도 있음**
   → 아직 전송 중인 데이터가 남아있을 수 있으므로,
   클라이언트의 FIN을 **ACK로만 우선 응답** 후,
   나중에 자신의 송신이 끝난 뒤 **FIN 전송**.

2. **양방향 종료의 독립성 유지**
   → TCP는 full-duplex이기 때문에 송신 방향만 닫거나 수신만 유지 가능.
   FIN과 ACK를 분리해야 이를 표현할 수 있음.

---

### ⚙️ **5️⃣ TIME_WAIT 상태 (참고)**

* 클라이언트는 마지막 ACK 이후 일정 시간(`2 * MSL`) 대기
  → 지연된 패킷이 네트워크에 남아있을 가능성 방지
  → 중복 FIN 재수신 대비

---

## 🎯 **면접용 요약**

> TCP의 연결 종료는 송신과 수신이 독립적으로 닫히기 때문에
> **FIN → ACK → FIN → ACK의 4단계**로 진행됩니다.
>
> 첫 번째 FIN/ACK은 한쪽 송신 종료,
> 두 번째 FIN/ACK은 반대쪽 송신 종료를 의미합니다.
>
> 이는 **Half-Close 구조** 덕분에 가능한 양방향 종료 방식이며,
> 각 방향의 전송이 완전히 끝났음을 명확히 보장하기 위함입니다.

---

### 🔥 꼬리질문 예상

1. **TIME_WAIT 상태는 왜 필요한가요?**
   → 지연된 패킷이 남아있을 때 잘못된 연결로 인식되는 걸 방지하기 위해.

2. **서버가 FIN 바로 보내지 않고 ACK 먼저 보내는 이유는요?**
   → 아직 보낼 데이터가 남아있을 수 있기 때문.

3. **Half-Close 후에도 데이터를 받을 수 있나요?**
   → 네. 한쪽이 FIN 보낸 후에도 반대쪽은 데이터 전송 가능.

</details>

## 5. TIME_WAIT 상태가 필요한 이유는 무엇인가요?

<details close>
<summary><strong>💡 자세한 정답</strong></summary>

<div style="background-color:#f6f8fa; padding:15px; border-radius:10px;">

## 💡 **TIME_WAIT 상태의 필요성**

> TCP 연결 종료 후, **마지막 ACK를 보낸 쪽(주로 클라이언트)** 이
> 일정 시간(`2 * MSL`, Maximum Segment Lifetime) 동안 **TIME_WAIT 상태로 대기**하는 이유는
> 네트워크 상의 지연 패킷 및 포트 재사용 문제를 안전하게 처리하기 위함입니다.

---

### ⚙️ **1️⃣ TIME_WAIT의 존재 목적**

|                 목적                | 설명                                                                      |
| :-------------------------------: | :---------------------------------------------------------------------- |
| **1. 지연된 패킷(Delayed Segment) 보호** | 이전 연결에서 남은 패킷이 네트워크를 돌아다니다가 새 연결에 잘못 도착하는 것을 방지                         |
|       **2. 연결 재사용 시 안전성 확보**      | 같은 포트 번호로 새로운 연결이 바로 생성될 때, 이전 세션의 데이터가 섞이지 않도록 함                       |
|       **3. 마지막 ACK 재전송 대비**       | FIN을 받은 상대가 “ACK을 못 받았다”고 판단하고 FIN을 다시 보낼 경우, 이를 정상적으로 처리하기 위해 일정 시간 대기 |

---

### ⚙️ **2️⃣ 예시 흐름**

```
[Client]                            [Server]
   | ------- FIN ------------>         | 
   | <------ FIN -------------         | 
   | ------- ACK ------------>         |  // 마지막 ACK 송신
   | ===== TIME_WAIT (2*MSL) =====>    |  // 대기 (지연 패킷 보호)
```

→ TIME_WAIT이 없다면?

* 서버가 FIN 재전송 시, 클라이언트는 이미 소켓을 닫아 응답 불가
* 이전 세션 패킷이 새 연결로 오인될 수 있음

---

### ⚙️ **3️⃣ MSL (Maximum Segment Lifetime)**

* 네트워크 상에서 **패킷이 살아있을 수 있는 최대 시간**
* TIME_WAIT은 **2 × MSL** 동안 유지
  → 왕복 시간(RTT) 고려
  → FIN 재전송 및 ACK 유실 대비

---

### ⚙️ **4️⃣ 서버에서의 고려 사항**

* 서버는 다수의 클라이언트와 통신 시 TIME_WAIT 누적 가능
* 리눅스 커널 옵션 예시:

  ```bash
  net.ipv4.tcp_tw_reuse = 1   # TIME_WAIT 소켓 재사용 허용 (조건부)
  net.ipv4.tcp_tw_recycle = 1 # (Deprecated, 비추천)
  ```

---

## 🎯 **면접용 요약**

> **TIME_WAIT은 TCP 연결을 완전히 종료하기 전 잠시 대기하는 상태**로,
> ① 지연된 패킷이 새 연결에 섞이는 것을 방지하고,
> ② 포트 재사용 시 충돌을 막기 위해 필요합니다.
>
> 즉, “**이전 연결의 잔재로부터 다음 연결을 보호하는 안전장치**”입니다.

---

### 🔥 꼬리질문 예상

1. **TIME_WAIT이 너무 많아지면 어떤 문제가 생기나요?**
   → 포트 고갈(especially short-lived connections).

2. **TIME_WAIT을 줄이는 방법은요?**
   → 서버 쪽에서는 `SO_REUSEADDR`, `SO_REUSEPORT` 옵션 사용.

3. **TIME_WAIT은 누가 가지나요?**
   → **마지막 ACK를 보낸 쪽**, 즉 **연결 종료를 능동적으로 수행한 측**이 가짐.

</details>

## 6. TCP의 흐름 제어(Flow Control)와 혼잡 제어(Congestion Control)의 차이점은 무엇인가요?

<details close>
<summary><strong>💡 자세한 정답</strong></summary>

<div style="background-color:#f6f8fa; padding:15px; border-radius:10px;">

## 💡 **TCP 흐름 제어 vs 혼잡 제어**

> TCP는 **신뢰성 있는 전송**을 위해 데이터를 무작정 보내지 않고,
> 수신자와 네트워크 상황에 맞게 전송 속도를 조절합니다.
> 이때 사용하는 두 핵심 메커니즘이 **흐름 제어**와 **혼잡 제어**입니다.

---

|      구분     | **흐름 제어 (Flow Control)**    | **혼잡 제어 (Congestion Control)**                             |
| :---------: | :-------------------------- | :--------------------------------------------------------- |
|    **목적**   | 수신자의 처리 능력 초과 방지            | 네트워크 내 혼잡(패킷 손실) 방지                                        |
|    **기준**   | **수신자 버퍼 크기**               | **네트워크 상태 (손실, RTT 등)**                                    |
|  **동작 위치**  | 송신자 ↔ 수신자 간                 | 송신자 ↔ 네트워크 전체                                              |
|  **핵심 변수**  | `Receiver Window (rwnd)`    | `Congestion Window (cwnd)`                                 |
| **주요 알고리즘** | Stop & Wait, Sliding Window | Slow Start, Congestion Avoidance, Fast Retransmit/Recovery |
| **문제 발생 시** | 수신자 버퍼 오버플로                 | 네트워크 패킷 손실, 지연 증가                                          |
|    **결과**   | 수신 측 안정성 확보                 | 전체 네트워크 효율성 유지                                             |

---

### ⚙️ **1️⃣ 흐름 제어 (Flow Control)**

> 수신자가 처리할 수 있는 양보다 **송신자가 빠르게 보내는 것을 방지**하는 기능.

* 수신자는 자신의 버퍼 여유 공간(`rwnd`)을 ACK에 포함시켜 송신자에게 전달
* 송신자는 해당 크기만큼만 데이터 전송
* 버퍼가 가득 차면 송신자 전송 일시 중단

```cpp
// 예시 개념
rwnd = 4096; // 수신 버퍼 크기
while (보낼_데이터 < rwnd)
    send(data);
```

📌 **핵심 키워드:**
“**수신자의 속도에 맞춰라.**”

---

### ⚙️ **2️⃣ 혼잡 제어 (Congestion Control)**

> 네트워크 전체의 **트래픽 과부하를 방지**하기 위한 제어 메커니즘.

* 송신자는 네트워크의 혼잡 상태를 예측해 전송 속도 조절
* 패킷 손실 발생 시 → 혼잡으로 간주하고 윈도우 축소
* 정상적으로 ACK 수신 시 → 점진적으로 윈도우 확장

#### 대표 알고리즘:

|              알고리즘              | 설명                        |
| :----------------------------: | :------------------------ |
|         **Slow Start**         | 전송 속도를 천천히 증가 (지수적 증가 시작) |
|    **Congestion Avoidance**    | 혼잡 임계치 도달 후 선형 증가         |
| **Fast Retransmit / Recovery** | 손실 감지 시 빠른 재전송 및 부분 회복    |

📌 **핵심 키워드:**
“**네트워크 상태에 맞춰라.**”

---

### ⚙️ **3️⃣ 두 제어의 관계**

```text
실제 전송 가능한 윈도우 크기 = min(rwnd, cwnd)
```

→ 즉,

* 수신자가 느리면 → **Flow Control이 제한**
* 네트워크가 혼잡하면 → **Congestion Control이 제한**

---

## 🎯 **면접용 요약**

> 흐름 제어는 **수신자의 버퍼 상태 기반 제어**,
> 혼잡 제어는 **네트워크 전체의 혼잡 상태 기반 제어**입니다.
>
> 전송량은 `rwnd`(수신자)와 `cwnd`(네트워크) 중 **작은 값에 의해 제한**되며,
> TCP는 이를 통해 수신 오버플로와 네트워크 혼잡을 모두 방지합니다.

---

### 🔥 꼬리질문 예상

1. **혼잡 제어 알고리즘 중 Slow Start는 어떻게 동작하나요?**
   → 시작 시 `cwnd=1`, ACK 받을 때마다 2배씩 증가하다 임계치 도달 시 선형 증가로 전환.

2. **Flow Control이 없으면 어떤 문제가 생기나요?**
   → 수신 버퍼 오버플로로 인한 패킷 손실.

3. **둘 다 존재하는 이유는요?**
   → 흐름 제어는 **수신자 보호**, 혼잡 제어는 **네트워크 보호** 목적이 다름.

</details>

## 7. Nagle 알고리즘이란 무엇이며, 게임 서버에서는 왜 비활성화하나요?

<details close>
<summary><strong>💡 자세한 정답</strong></summary>

<div style="background-color:#f6f8fa; padding:15px; border-radius:10px;">

## 💡 **Nagle 알고리즘 (Nagle’s Algorithm)**

> **작은 패킷(Small Packet)** 들을 모아서 한 번에 전송하는 TCP의 **전송 효율화 알고리즘**입니다.
> 단, 실시간성이 중요한 애플리케이션(예: 게임, 채팅)에서는 오히려 **지연(latency)** 을 유발하기 때문에 **비활성화**합니다.

---

### ⚙️ **1️⃣ 동작 원리**

> “작은 데이터 여러 개를 바로 보내지 말고, ACK 올 때까지 잠시 모았다가 한 번에 보내자.”

* TCP는 기본적으로 전송 효율을 위해 **소량의 데이터를 버퍼에 모은 후 전송**
* Nagle 알고리즘은 아래 조건에서만 전송을 허용합니다:

  ```
  1. 이전 패킷에 대한 ACK을 받았거나
  2. 버퍼에 충분한 데이터가 쌓였을 때
  ```
* 즉, 아직 ACK이 안 온 상태라면 새로운 소량 데이터는 **버퍼에 대기**하게 됨.

---

### ⚙️ **2️⃣ 예시 상황**

```text
[Client]
  send("A");
  send("B");
  send("C");

→ Nagle 작동 시:
   A 보냄 → ACK 기다림 동안 B, C는 버퍼에 쌓임 (지연 발생)
```

→ 소량 패킷이 밀려 **지연(delay)** 이 누적될 수 있음.

---

### ⚙️ **3️⃣ 게임 서버에서의 문제점**

|   구분   | 설명                                |
| :----: | :-------------------------------- |
| **문제** | 실시간 입력(키 입력, 이동 패킷 등)이 지연됨        |
| **원인** | Nagle이 ACK 대기 중 패킷을 묶어 전송 지연      |
| **결과** | 캐릭터 움직임, 타격 반응이 느려짐 (input delay) |

게임 서버는 **소량 패킷이라도 즉시 전송하는 저지연(low latency)** 이 중요하므로,
**Nagle 알고리즘을 비활성화**해야 합니다.

---

### ⚙️ **4️⃣ 비활성화 방법**

> TCP 옵션: `TCP_NODELAY`

```cpp
int flag = 1;
setsockopt(socket, IPPROTO_TCP, TCP_NODELAY, (char*)&flag, sizeof(flag));
```

→ 설정 시 Nagle 알고리즘 비활성화
→ **모든 패킷을 즉시 전송 (No Delay)**

---

### ⚙️ **5️⃣ 관련 개념**

|        개념       | 설명                                 |
| :-------------: | :--------------------------------- |
|  **Nagle 알고리즘** | 작은 패킷을 묶어 전송 (지연 발생 가능)            |
| **Delayed ACK** | ACK 응답을 약간 늦춰 전송 수 줄이는 기능          |
|   🧩 둘 다 활성화 시  | 작은 패킷 + 늦은 ACK → **최악의 지연 콜라보** 발생 |

---

## 🎯 **면접용 요약**

> Nagle 알고리즘은 작은 패킷을 모아 전송 효율을 높이는 TCP 기능이지만,
> 게임 서버처럼 **실시간 반응이 중요한 환경**에서는 **입력 지연**을 초래합니다.
>
> 따라서 일반적으로 `setsockopt()`를 이용해
> **`TCP_NODELAY` 옵션을 설정하여 비활성화**합니다.

---

### 🔥 꼬리질문 예상

1. **Nagle 알고리즘과 Delayed ACK이 함께 켜져 있으면?**
   → 양쪽 모두 대기 → 수백 ms 단위 지연 발생.

2. **그럼 언제 Nagle을 사용하는 게 좋은가요?**
   → 대용량 데이터 전송보다 **작은 제어 메시지 다량 송신** 시 효율적 (예: Telnet, 파일 전송 초기).

3. **UDP에는 Nagle 알고리즘이 있나요?**
   → 없습니다. UDP는 비연결형, 흐름 제어·ACK 자체가 없기 때문.

</details>

## 8. Delayed ACK은 무엇이고, Nagle과의 조합 문제는 무엇인가요?

<details close>
<summary><strong>💡 자세한 정답</strong></summary>

<div style="background-color:#f6f8fa; padding:15px; border-radius:10px;">

## 💡 **Delayed ACK (지연된 확인 응답)**

> TCP는 매 수신 패킷마다 바로 ACK(확인 응답)을 보내지 않고,
> **일정 시간(보통 100~200ms)** 동안 **ACK을 잠시 지연시켜** 여러 패킷에 대한 응답을 **한 번에 처리**하는 최적화 기법입니다.

---

### ⚙️ **1️⃣ 동작 목적**

|       목적       | 설명                         |
| :------------: | :------------------------- |
| **네트워크 효율 향상** | 불필요한 ACK 패킷 수를 줄여 전송량 감소   |
|  **CPU 부담 감소** | 송·수신 간 ACK 처리 빈도를 줄여 성능 향상 |

---

### ⚙️ **2️⃣ 동작 방식**

1. 패킷을 수신한 즉시 ACK을 보내지 않음
2. 잠시 대기(최대 200ms)
3. 그 사이 추가 데이터가 오면 → 함께 ACK
4. 대기 시간 초과 시 → 단독 ACK 전송

```text
[송신자] ----> [수신자]
         (데이터 수신)
         ← ACK (0.2초 후)
```

→ 즉, **응답 효율을 위해 ACK을 묶어 보냄**.

---

### ⚙️ **3️⃣ Nagle 알고리즘과의 조합 문제**

|        요소       | 동작                          |
| :-------------: | :-------------------------- |
|  **Nagle 알고리즘** | ACK이 오기 전까지 새로운 소량 패킷 전송 지연 |
| **Delayed ACK** | ACK을 일부러 늦게 전송              |

→ **두 기능이 동시에 활성화되면 다음과 같은 악순환 발생:**

```
1️⃣ 송신자: "ACK 안 왔네, 더 못 보냄"  ← Nagle 대기
2️⃣ 수신자: "조금 더 기다렸다가 ACK 보낼래"  ← Delayed ACK 대기
→ 결과: 200ms 이상 RTT 증가
```

📉 즉, **상호 대기(deadlock 유사 상황)** 로 인한 **지연 폭발(latency spike)** 발생.

---

### ⚙️ **4️⃣ 게임 서버나 실시간 서비스의 해결 방법**

* Nagle → 비활성화 (`TCP_NODELAY = true`)
* Delayed ACK → 비활성화 (`TCP_QUICKACK` or OS 설정)

```cpp
int flag = 1;
setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (char*)&flag, sizeof(flag)); // Nagle 비활성화
setsockopt(sock, IPPROTO_TCP, TCP_QUICKACK, (char*)&flag, sizeof(flag)); // Delayed ACK 비활성화 (Linux)
```

---

### ⚙️ **5️⃣ 실제 영향 예시**

| 환경                      |  평균 지연 (RTT) |
| :---------------------- | :----------: |
| Nagle + Delayed ACK 활성화 |   100~200ms  |
| 둘 다 비활성화                | <5ms (즉시 반응) |

---

## 🎯 **면접용 요약**

> **Delayed ACK**은 ACK 패킷을 잠시 지연시켜 전송 효율을 높이는 기능입니다.
> 하지만 **Nagle 알고리즘**과 동시에 켜져 있으면,
> 송신자는 ACK을 기다리고, 수신자는 ACK을 지연시키면서 **서로 대기 상태**가 되어 **RTT(왕복 지연)** 이 증가합니다.
>
> 따라서 **게임 서버나 실시간 통신 환경**에서는
> `TCP_NODELAY`(Nagle 비활성화) + `TCP_QUICKACK`(Delayed ACK 비활성화)를 함께 설정합니다.

---

### 🔥 꼬리질문 예상

1. **Delayed ACK이 완전히 나쁜 기능인가요?**
   → 일반 웹 트래픽처럼 대용량, 비실시간 통신에서는 오히려 효율적.

2. **Nagle과 Delayed ACK 중 하나만 켜면 괜찮을까요?**
   → 보통 Nagle만 꺼도 실시간성 문제는 완화됨.

3. **RTT 증가가 왜 중요한가요?**
   → 입력 응답, 피드백 지연 → 게임/음성 서비스 품질 저하.

</details>

## 9. TCP Keep-Alive 옵션은 어떤 상황에서 사용하나요?

<details close>
<summary><strong>💡 자세한 정답</strong></summary>

<div style="background-color:#f6f8fa; padding:15px; border-radius:10px;">

## 💡 **TCP Keep-Alive 옵션**

> TCP 연결이 장시간 동안 **데이터 송수신 없이 유휴 상태(idle)** 일 때,
> 상대 측이 여전히 **정상적으로 연결되어 있는지 확인하기 위한 메커니즘**입니다.
>
> 즉, **비정상적으로 끊긴 세션(예: 네트워크 단절, 비정상 종료 등)** 을 감지하기 위해 사용됩니다.

---

### ⚙️ **1️⃣ 동작 원리**

|   단계  | 설명                                               |
| :---: | :----------------------------------------------- |
| **①** | 일정 시간 동안 트래픽이 없으면 Keep-Alive 패킷 전송 (데이터 없음, 제어용) |
| **②** | 상대가 응답(ACK)을 보내면 → 연결 정상 유지                      |
| **③** | 일정 횟수 이상 응답 없으면 → 연결 끊긴 것으로 간주 후 세션 종료           |

---

### ⚙️ **2️⃣ 주요 설정 값 (기본 OS 기준)**

|           옵션           | 설명                            | 기본값 (Linux 기준) |
| :--------------------: | :---------------------------- | :------------: |
|  `tcp_keepalive_time`  | 최초 Keep-Alive 패킷을 보내기 전 대기 시간 |   7200초 (2시간)  |
|  `tcp_keepalive_intvl` | 재전송 간격                        |       75초      |
| `tcp_keepalive_probes` | 재시도 횟수                        |       9회       |

```bash
# 예시: Linux sysctl 설정
sysctl -w net.ipv4.tcp_keepalive_time=60
sysctl -w net.ipv4.tcp_keepalive_intvl=10
sysctl -w net.ipv4.tcp_keepalive_probes=3
```

---

### ⚙️ **3️⃣ C++ 설정 예시**

```cpp
int optval = 1;
setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, &optval, sizeof(optval)); // Keep-Alive 활성화
```

→ OS 레벨 Keep-Alive 패킷이 주기적으로 전송됨.

---

### ⚙️ **4️⃣ 사용 목적 및 적용 사례**

| 상황                | 설명                                       |
| :---------------- | :--------------------------------------- |
| **장시간 연결 유지형 서버** | 채팅 서버, 게임 서버, HTTP Persistent Connection |
| **비정상 종료 감지**     | 클라이언트 네트워크 단절, 비정상 프로세스 종료 시 감지          |
| **NAT/방화벽 환경**    | NAT 타임아웃으로 인한 세션 끊김 방지                   |

---

### ⚙️ **5️⃣ 주의점**

* Keep-Alive는 **TCP 레벨에서만 연결 확인**
  → 실제 애플리케이션 레벨(게임 세션 등)은 여전히 별도 heartbeat 필요
* OS 기본 설정(2시간)은 너무 길기 때문에
  **서버 애플리케이션 단에서 별도 heartbeat 구현**이 일반적

---

## 🎯 **면접용 요약**

> **TCP Keep-Alive는 장시간 유휴 상태의 연결이 살아 있는지 확인하는 기능**입니다.
> 일정 시간마다 소량의 제어 패킷을 보내고, 응답이 없으면 세션을 종료합니다.
>
> 게임 서버나 채팅 서버처럼 **항상 연결을 유지해야 하는 서비스**에서
> **비정상 종료된 클라이언트를 빠르게 감지**하기 위해 사용됩니다.

---

### 🔥 꼬리질문 예상

1. **애플리케이션 단의 heartbeat과 차이는 무엇인가요?**
   → Keep-Alive는 OS 수준(커널 관리), heartbeat은 애플리케이션이 직접 주기 송신.

2. **NAT 환경에서 Keep-Alive를 사용하는 이유는요?**
   → NAT 장비가 비활성 연결을 일정 시간 후 삭제하므로, Keep-Alive로 세션 유지.

3. **Keep-Alive의 단점은 없나요?**
   → 패킷 오버헤드 발생, 비정상 세션 판단까지 시간 오래 걸릴 수 있음.

</details>

## 10. MTU와 MSS의 차이는 무엇인가요?

<details close>
<summary><strong>💡 자세한 정답</strong></summary>

<div style="background-color:#f6f8fa; padding:15px; border-radius:10px;">

## 💡 **MTU vs MSS**

> **MTU(Maximum Transmission Unit)** 와
> **MSS(Maximum Segment Size)** 는 네트워크 전송 단위 크기를 나타내지만,
> 적용 범위가 다릅니다.
>
> 간단히 말해,
> **MTU는 프레임 전체 크기**,
> **MSS는 TCP Payload(데이터 부분) 크기**입니다.

---

### ⚙️ **1️⃣ 기본 정의**

|        구분       | **MTU**                   | **MSS**                                  |
| :-------------: | :------------------------ | :--------------------------------------- |
|      **의미**     | 한 번에 전송 가능한 최대 **프레임 크기** | TCP가 한 세그먼트에 실을 수 있는 **데이터(Payload) 크기** |
|    **포함 범위**    | IP 헤더 + TCP/UDP 헤더 + 데이터  | **데이터만**                                 |
|    **계층 위치**    | 네트워크 계층 (IP 기준)           | 전송 계층 (TCP 기준)                           |
| **기본 크기 (이더넷)** | 1500바이트                   | 1460바이트 (1500 - 20(IP) - 20(TCP))        |
|    **설정 위치**    | 네트워크 인터페이스 (NIC)          | TCP 연결 협상 시 (SYN 패킷 내 옵션)                |

---

### ⚙️ **2️⃣ 관계 예시**

```text
MTU = 1500 bytes
 ├─ IP Header = 20 bytes
 ├─ TCP Header = 20 bytes
 └─ MSS = 1460 bytes (실제 데이터)
```

즉,

```
MSS = MTU - (IP Header + TCP Header)
```

---

### ⚙️ **3️⃣ Fragmentation (단편화) 방지**

> 만약 MTU보다 큰 패킷을 전송하면 IP 계층에서 **단편화(Fragmentation)** 발생.
> → 전송 지연 증가 + CPU 부하 + 패킷 손실 시 재전송 부담.

이를 방지하기 위해:

* TCP 연결 시 **MSS 협상(MSS Option)** 으로 상대 측과 최대 안전 크기 교환
* **경로 MTU(Path MTU) 탐색(PMTU Discovery)** 를 통해 동적으로 MSS 조정

---

### ⚙️ **4️⃣ 실제 TCP 연결 시 협상 예시**

```text
[SYN]  →  MSS=1460
[SYN+ACK] ← MSS=1400
→ 최종 MSS = min(1460, 1400) = 1400
```

→ 연결 시점에 양단이 서로 허용 가능한 **최소 MSS 값으로 통신**.

---

### ⚙️ **5️⃣ 게임/서버 환경에서의 고려**

| 항목            | 영향                                |
| :------------ | :-------------------------------- |
| **MSS 초과 패킷** | Fragmentation 발생 → 지연 증가          |
| **작은 MSS 설정** | 안정적이지만 헤더 오버헤드 증가                 |
| **적정 MSS 유지** | 지연 최소화 + 효율적 전송 (주로 1400~1460 권장) |

---

## 🎯 **면접용 요약**

> **MTU**는 네트워크 계층에서 전송 가능한 **전체 프레임 최대 크기**,
> **MSS**는 TCP가 한 번에 보낼 수 있는 **데이터(Payload) 최대 크기**입니다.
>
> MSS는 `MTU - (IP + TCP 헤더)` 로 계산되며,
> **Fragmentation(단편화)** 을 방지하기 위해 MSS를 조정합니다.

---

### 🔥 꼬리질문 예상

1. **MSS를 작게 설정하면 어떤 장점이 있나요?**
   → 단편화 위험 감소, 안정성 향상.

2. **MTU보다 큰 패킷을 보내면 무슨 일이 일어나나요?**
   → IP 계층에서 단편화 발생, 지연 및 손실 위험 증가.

3. **Path MTU Discovery는 무엇인가요?**
   → 중간 라우터의 MTU를 고려해 최적 MSS를 자동으로 조정하는 기법.

</details>

## 11. 블로킹(Blocking) / 논블로킹(Non-blocking) / 비동기(Asynchronous) I/O의 차이점을 설명하세요

<details close>
<summary><strong>💡 자세한 정답</strong></summary>

<div style="background-color:#f6f8fa; padding:15px; border-radius:10px;">

## 💡 **Blocking / Non-blocking / Asynchronous I/O 비교**

> I/O(입출력) 호출 시 **호출 흐름이 어떻게 진행되는지**,
> 그리고 **스레드가 블로킹되는지 여부**에 따라 구분됩니다.

---

### ⚙️ **1️⃣ 핵심 비교표**

|          구분         | **Blocking I/O**    | **Non-blocking I/O**        | **Asynchronous I/O**              |
| :-----------------: | :------------------ | :-------------------------- | :-------------------------------- |
|      **호출 방식**      | I/O 완료될 때까지 대기      | I/O 준비 안 되어도 즉시 반환          | I/O 요청만 보내고 완료 시점에 알림             |
|    **스레드 블로킹 여부**   | 블로킹됨                | 블로킹되지 않음                    | 블로킹되지 않음                          |
|     **결과 확인 시점**    | 함수 반환 시             | 반복 호출(polling)              | OS가 완료 시 알려줌(callback, event)     |
| **예시 함수 (Windows)** | `recv()`, `send()`  | `ioctlsocket(..., FIONBIO)` | `WSARecv`, `AcceptEx` (IOCP 기반)   |
|  **예시 함수 (Linux)**  | `read()`, `write()` | `fcntl(fd, O_NONBLOCK)`     | `aio_read()`, `epoll`, `io_uring` |
|     **스레드 효율성**     | 낮음 (대기 시간 많음)       | 중간 (반복 확인 필요)               | 높음 (완전 이벤트 기반)                    |

---

### ⚙️ **2️⃣ 동작 흐름 예시**

#### 🔹 **(1) Blocking I/O**

```cpp
recv(socket, buf, size, 0); // 데이터 도착할 때까지 스레드 정지
```

* 호출 즉시 블로킹됨
* 커널에서 데이터 수신 완료 후 반환
* 단순하지만 **지연 발생** 가능

```
[App] ---- 대기 ----> [Kernel I/O 완료 후 반환]
```

---

#### 🔹 **(2) Non-blocking I/O**

```cpp
ioctlsocket(sock, FIONBIO, &on);
int ret = recv(sock, buf, size, 0);
if (ret == -1 && WSAGetLastError() == WSAEWOULDBLOCK)
    ; // 아직 데이터 없음 → 나중에 다시 시도
```

* 즉시 반환 → 데이터 없으면 `EWOULDBLOCK` 에러
* **반복 확인(polling)** 필요
* CPU 낭비 가능 → `select()`, `poll()` 등과 함께 사용

```
[App] --요청--> [Kernel]
        ← 아직 데이터 없음 (즉시 반환)
```

---

#### 🔹 **(3) Asynchronous I/O (비동기 I/O)**

```cpp
WSARecv(sock, &buf, 1, &bytes, &flags, &overlapped, NULL);
// 바로 반환됨, 완료되면 IOCP 이벤트로 알림
```

* I/O 요청만 등록하고 **즉시 반환**
* **I/O 완료 시 OS가 자동 통보** (콜백, 이벤트, IOCP 큐 등)
* 스레드는 다른 작업 수행 가능 → **고성능 서버에 적합**

```
[App] --요청--> [Kernel]
   (다른 일 수행 중)
         ↓
[Kernel] I/O 완료 → [App] 알림
```

---

### ⚙️ **3️⃣ IOCP (Windows Asynchronous I/O) 예시**

> 실제 게임 서버나 대규모 네트워크 서버는 대부분 IOCP 기반 비동기 모델 사용.

* 커널이 I/O 완료 시점에 **Completion Port**에 알림 전달
* **스레드 풀(Thread Pool)** 이 이를 수신 후 처리
* **수천 개의 소켓을 효율적으로 관리 가능**

---

### ⚙️ **4️⃣ 간단 비교 요약**

| 기준               | Blocking  | Non-blocking | Asynchronous        |
| ---------------- | --------- | ------------ | ------------------- |
| **함수 반환 시점**     | I/O 완료 후  | 즉시 반환        | 즉시 반환               |
| **I/O 완료 알림 방식** | 함수 반환 시점  | 직접 재시도       | OS 이벤트로 통보          |
| **스레드 효율**       | 낮음        | 중간           | 매우 높음               |
| **적합한 상황**       | 간단한 단일 연결 | 소수의 소켓       | 대규모 서버(IOCP, epoll) |

---

## 🎯 **면접용 요약**

> **Blocking I/O**는 호출 시 스레드가 대기하며,
> **Non-blocking I/O**는 즉시 반환하지만 반복 확인이 필요하고,
> **Asynchronous I/O**는 I/O 완료를 OS가 **이벤트나 큐로 알려주는 방식**입니다.
>
> 비동기 방식은 스레드를 블로킹하지 않아 **IOCP, epoll 기반 고성능 서버**에서 주로 사용됩니다.

---

### 🔥 꼬리질문 예상

1. **IOCP가 Non-blocking I/O와 다른 점은 무엇인가요?**
   → Non-blocking은 직접 반복 확인, IOCP는 커널이 완료 이벤트를 큐로 전달.

2. **epoll과 select()의 차이는요?**
   → select는 FD 수 제한·매번 전체 검사, epoll은 이벤트 기반으로 효율적.

3. **게임 서버에서 Blocking I/O가 위험한 이유는요?**
   → 한 스레드가 대기 중이면 다른 클라이언트 요청을 처리할 수 없음.

</details>

## 12. `select`, `poll`, `epoll`의 차이점은 무엇인가요?

<details close>
<summary><strong>💡 자세한 정답</strong></summary>

<div style="background-color:#f6f8fa; padding:15px; border-radius:10px;">

## 💡 **select / poll / epoll 비교**

> 다수의 소켓을 동시에 감시하기 위한 **I/O 멀티플렉싱(Multiplexing)** 방식의 차이입니다.
>
> 세 방식 모두 “여러 파일 디스크립터(FD)”의 상태 변화를 감지하지만,
> **스케일링 성능**, **FD 관리 방식**, **커널 처리 구조**에서 큰 차이가 있습니다.

---

### ⚙️ **1️⃣ 핵심 비교표**

|       구분      | **select**              | **poll**            | **epoll**               |
| :-----------: | :---------------------- | :------------------ | :---------------------- |
|   **등장 순서**   | 가장 오래된 방식 (BSD 계열)      | 개선된 표준화 버전 (POSIX)  | Linux 고성능 전용            |
|  **FD 관리 방식** | 비트마스크(`fd_set`)         | 배열(`pollfd[]`)      | 커널 내부 이벤트 테이블           |
|  **FD 개수 제한** | 기본 1024개 (`FD_SETSIZE`) | 제한 없음 (단, 배열 크기 의존) | 제한 없음                   |
|  **FD 검사 방식** | 모든 FD 전체 스캔 (O(n))      | 모든 FD 전체 스캔 (O(n))  | 이벤트 기반 (O(1))           |
|  **변경 감지 방식** | 매 호출마다 전체 집합 복사         | 매 호출마다 전체 배열 전달     | 이벤트 등록 시 1회만 커널에 등록     |
| **데이터 복사 비용** | 많음 (유저-커널 반복 복사)        | 많음                  | 적음 (커널에서 직접 관리)         |
|  **스케일링 성능**  | 낮음 (FD 많을수록 급격히 느려짐)    | 중간                  | 높음 (수천 개 소켓 효율적)        |
| **이벤트 알림 방식** | Level Trigger만 지원       | Level Trigger       | Level + Edge Trigger 지원 |
|   **적합한 용도**  | 소수의 연결 (테스트용)           | 중간 규모 서버            | 대규모 서버, 게임 서버, 채팅 서버    |

---

### ⚙️ **2️⃣ 동작 구조 요약**

#### 🔹 **select**

```cpp
fd_set readfds;
FD_SET(sock, &readfds);
select(max_fd + 1, &readfds, NULL, NULL, NULL);
```

* 매번 FD 집합을 다시 전달해야 함
* 커널은 모든 FD 상태를 **전체 순회 검사**

---

#### 🔹 **poll**

```cpp
struct pollfd fds[N];
poll(fds, N, timeout);
```

* FD 집합이 배열 형태
* 여전히 모든 FD 검사 → O(n)
* 단, FD 개수 제한 없음 (select의 구조적 개선판)

---

#### 🔹 **epoll**

```cpp
int epfd = epoll_create(EPOLL_SIZE);
epoll_ctl(epfd, EPOLL_CTL_ADD, sock, &event);
epoll_wait(epfd, events, MAX_EVENTS, -1);
```

* 이벤트를 **커널 내부에 등록** → 이후부터는 변화만 감지
* **O(1)** 수준의 효율 (변화 있는 FD만 반환)
* Level / Edge Trigger 모두 지원
* **대규모 동시접속 서버 (IOCP 유사 구조)** 에 적합

---

### ⚙️ **3️⃣ Edge Trigger vs Level Trigger**

|           모드           | 설명                  | 특징                          |
| :--------------------: | :------------------ | :-------------------------- |
| **Level Trigger (기본)** | “준비된 상태면 계속 이벤트 발생” | 단순하지만 중복 이벤트 발생 가능          |
|    **Edge Trigger**    | “상태가 변할 때만 이벤트 발생”  | 효율적이지만 사용자 측 버퍼 완전 소모 처리 필요 |

---

### ⚙️ **4️⃣ 성능 및 구조적 차이**

| 비교 항목 | `select/poll` | `epoll` |
|:--|:--|
| FD 등록 비용 | 매번 호출 시 전달 | 한 번 등록 후 커널이 관리 |
| 감지 성능 | FD 수에 비례 (O(n)) | 변경된 FD만 보고 (O(1)) |
| 메모리 복사 | 유저-커널 간 매번 복사 | 커널 내부 유지 (1회 등록) |
| 대규모 연결 효율 | 매우 낮음 | 매우 높음 (수천~수만 소켓 가능) |

---

## 🎯 **면접용 요약**

> `select`와 `poll`은 **모든 FD를 매번 검사**해야 해서 O(n) 성능이고,
> `epoll`은 **커널이 이벤트를 직접 관리**하기 때문에 변경된 FD만 확인해 **O(1)** 성능을 냅니다.
>
> 따라서 **대규모 동시 접속 서버(게임/채팅 등)** 에서는 `epoll`이 필수적입니다.

---

### 🔥 꼬리질문 예상

1. **epoll의 Edge Trigger 모드에서 주의할 점은?**
   → 버퍼가 완전히 비워질 때까지 `read()` 반복해야 이벤트 재발생 방지.

2. **Windows의 IOCP와 epoll의 공통점은요?**
   → 둘 다 **이벤트 기반 비동기 모델**, 스레드 효율적 처리 구조.

3. **select가 FD 1024개 제한을 가지는 이유는?**
   → 내부에서 `fd_set`을 비트마스크 배열로 고정 크기로 관리하기 때문.

</details>


## 13. IOCP란 무엇이고, 동작 원리를 설명해보세요

<details close>
<summary><strong>💡 자세한 정답</strong></summary>

<div style="background-color:#f6f8fa; padding:15px; border-radius:10px;">

## 💡 **IOCP (I/O Completion Port)**

> **I/O Completion Port**는 **Windows 전용 비동기 I/O 모델**로,
> 수천 개의 소켓이나 파일 핸들에 대해 **효율적으로 입출력 완료 이벤트를 처리**하기 위한
> **커널 수준 Completion 기반 비동기 구조**입니다.
>
> 즉, **“I/O 요청은 비동기로 등록 → 완료 시 커널이 큐로 통지”** 하는 모델입니다.

---

### ⚙️ **1️⃣ IOCP의 핵심 구조**

|                구성 요소               | 역할                                     |
| :--------------------------------: | :------------------------------------- |
|         **Completion Port**        | I/O 완료 이벤트를 큐잉하는 커널 객체                 |
|       **Worker Thread Pool**       | 큐에서 완료 이벤트를 가져와 처리                     |
|         **Overlapped I/O**         | 비동기 I/O를 수행하기 위한 데이터 구조 (`OVERLAPPED`) |
| **커널 통지(Completion Notification)** | I/O 작업 완료 시점에 커널이 포트에 이벤트 등록           |

---

### ⚙️ **2️⃣ 동작 과정 (핵심 흐름)**

```text
① 소켓 생성 및 IOCP에 연결 (CreateIoCompletionPort)
② 비동기 I/O 요청 등록 (WSARecv, WSASend, AcceptEx 등)
③ I/O 완료 시, 커널이 Completion Port에 결과 등록
④ Worker Thread가 큐에서 이벤트를 꺼내 처리 (GetQueuedCompletionStatus)
```

---

### ⚙️ **3️⃣ 순서도 예시**

```
[App Thread]       [Kernel]           [IOCP Queue]
     |                 |                   |
1️⃣ CreateIoCompletionPort()               |
     |--------> 연결됨 ------------------->|
     |
2️⃣ WSARecv() (비동기 요청 등록)
     |------------- 요청 처리 대기 --------|
     |                 ↓
3️⃣ (I/O 완료)
     |<------------- 결과 저장 -----------|
     |                 ↓
4️⃣ GetQueuedCompletionStatus() ← 이벤트 큐 통보
     |→ 데이터 처리 완료
```

---

### ⚙️ **4️⃣ IOCP의 장점**

|           항목          | 설명                                |
| :-------------------: | :-------------------------------- |
| **비동기 Completion 기반** | 요청과 완료가 분리되어 효율적                  |
|     **스레드 효율 극대화**    | 커널이 스레드 Wake-up 조절 → 최소한의 스레드만 활성 |
|   **수천 개 소켓 처리 가능**   | 각 소켓마다 스레드 생성 불필요                 |
|   **CPU 코어 기반 스케줄링**  | IOCP 큐는 논리 코어 수에 맞춰 워커 스레드 운영     |

---

### ⚙️ **5️⃣ Thread Pool 운영 방식**

* IOCP는 커널이 **스레드의 작업량을 감시**하며,
  필요한 시점에만 새로운 스레드를 깨움
* 이로 인해 **컨텍스트 스위칭 최소화**, CPU 효율 극대화
* 일반적으로 코어 개수 × 2 정도의 워커 스레드가 적정

---

### ⚙️ **6️⃣ 예시 코드 개요**

```cpp
HANDLE iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, 0, 0, 0);

// 소켓 등록
CreateIoCompletionPort((HANDLE)socket, iocp, (ULONG_PTR)session, 0);

// 비동기 수신 요청
WSARecv(socket, &wsaBuf, 1, NULL, &flags, &session->overlapped, NULL);

// 완료 이벤트 대기
DWORD bytes, key;
LPOVERLAPPED overlapped;
GetQueuedCompletionStatus(iocp, &bytes, &key, &overlapped, INFINITE);
```

---

### ⚙️ **7️⃣ 다른 모델과의 비교**

|               모델               | 특징                        | 블로킹 여부 | 효율성    |
| :----------------------------: | :------------------------ | :----- | :----- |
|        **Blocking I/O**        | 단일 호출 블로킹                 | 블로킹    | 낮음     |
| **Non-blocking + select/poll** | FD 기반 이벤트 검사              | 비블로킹   | 중간     |
|        **epoll (Linux)**       | 이벤트 기반                    | 비블로킹   | 높음     |
|       **IOCP (Windows)**       | **Completion 기반 (완료 통지)** | 비블로킹   | **최고** |

---

## 🎯 **면접용 요약**

> IOCP는 **Windows의 Completion 기반 비동기 I/O 모델**로,
> 비동기 요청(Overlapped I/O)을 등록하면 커널이 **I/O 완료 시점에 이벤트 큐로 통보**합니다.
>
> **스레드 풀 + 커널 큐 기반 구조**로, 수천 개의 소켓을 효율적으로 처리할 수 있으며,
> **CPU 부하와 컨텍스트 스위칭을 최소화**하는 고성능 서버 아키텍처에 적합합니다.

---

### 🔥 꼬리질문 예상

1. **IOCP와 epoll의 차이점은 무엇인가요?**
   → epoll은 “준비 상태(Ready)” 기반, IOCP는 “완료(Completion)” 기반.

2. **Overlapped I/O란 무엇인가요?**
   → 비동기 요청을 커널에 등록해놓고 완료 시점에 콜백/이벤트로 결과 받는 구조.

3. **스레드 개수를 많이 늘리면 성능이 좋아질까요?**
   → 아니요. IOCP는 커널이 자동으로 스레드 스케줄링하므로 과도한 생성은 오히려 오버헤드만 증가.

</details>

## 14. `WSARecv` / `WSASend` 함수가 `send` / `recv`와 다른 점은 무엇인가요?

<details close>
<summary><strong>💡 자세한 정답</strong></summary>

<div style="background-color:#f6f8fa; padding:15px; border-radius:10px;">

## 💡 **WSARecv / WSASend vs send / recv**

> `WSARecv`와 `WSASend`는 **Windows 전용 비동기 I/O 함수**로,
> **Overlapped I/O 구조**를 사용해 **IOCP(Completion Port)** 와 연동되는 점에서
> 일반 `send` / `recv`와 근본적으로 다릅니다.

---

### ⚙️ **1️⃣ 핵심 비교표**

|          구분          | `send` / `recv`      | `WSASend` / `WSARecv`       |
| :------------------: | :------------------- | :-------------------------- |
|      **I/O 방식**      | 동기(Blocking) 또는 논블로킹 | 비동기(Asynchronous)           |
| **Overlapped 구조 사용** | ❌ 없음                 | ✅ 있음 (`OVERLAPPED` 구조체 필요)  |
|   **IOCP 연동 가능 여부**  | ❌ 불가능                | ✅ 가능 (Completion 기반 이벤트 통지) |
|     **함수 반환 시점**     | 데이터 송수신 완료 후 반환      | 요청 등록만 하고 즉시 반환             |
|     **작업 완료 확인**     | 함수 반환 시              | 커널이 IOCP 큐로 완료 통보           |
|       **사용 목적**      | 간단한 동기 통신            | 대규모 비동기 서버 구조 (IOCP)        |

---

### ⚙️ **2️⃣ 동작 흐름 차이**

#### 🔹 **`recv` (동기 / 블로킹 예시)**

```cpp
int ret = recv(sock, buf, size, 0); 
// 데이터 도착할 때까지 스레드 블로킹
```

→ **I/O 완료 시점에 함수 반환**
→ 단일 스레드에서 대기하면 다른 연결을 처리 불가.

---

#### 🔹 **`WSARecv` (비동기 / IOCP 연계)**

```cpp
WSABUF wsaBuf;
wsaBuf.buf = buffer;
wsaBuf.len = sizeof(buffer);

OVERLAPPED overlapped = {};
DWORD flags = 0;

WSARecv(sock, &wsaBuf, 1, NULL, &flags, &overlapped, NULL);
// 즉시 반환됨 — 실제 I/O는 커널에서 비동기 수행
```

→ 요청만 등록하고 즉시 반환
→ 커널이 I/O 완료 시 **Completion Port 큐에 결과 등록**
→ 서버 스레드는 `GetQueuedCompletionStatus()` 로 완료 이벤트 수신

---

### ⚙️ **3️⃣ IOCP 연동 구조**

```
[App Thread]
   ↓  (WSARecv 등록)
[Kernel] ---- 비동기 I/O 처리 ----
   ↓  (I/O 완료 시)
[IOCP Queue]
   ↓
[Worker Thread] ← GetQueuedCompletionStatus() 호출 중
```

→ **비동기 요청 → 커널 처리 → 완료 통지**
→ 스레드는 대기하지 않고 **다른 클라이언트 처리 병행 가능**

---

### ⚙️ **4️⃣ OVERLAPPED 구조체**

```cpp
typedef struct _OVERLAPPED {
    ULONG_PTR Internal;
    ULONG_PTR InternalHigh;
    DWORD Offset;
    DWORD OffsetHigh;
    HANDLE hEvent;
} OVERLAPPED;
```

* 각 I/O 작업의 상태 추적용 구조체
* **IOCP 큐에서 어떤 작업이 완료되었는지 식별 가능**

---

### ⚙️ **5️⃣ 실제 IOCP 서버 예시 코드**

```cpp
HANDLE iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, 0, 0, 0);
CreateIoCompletionPort((HANDLE)sock, iocp, (ULONG_PTR)session, 0);

WSARecv(sock, &wsaBuf, 1, NULL, &flags, &session->overlapped, NULL);

DWORD bytes, key;
LPOVERLAPPED overlapped;
GetQueuedCompletionStatus(iocp, &bytes, &key, &overlapped, INFINITE);
```

→ `WSARecv` 등록 후 즉시 반환
→ 완료 시 `GetQueuedCompletionStatus()` 에서 **비동기 이벤트 수신**

---

### ⚙️ **6️⃣ 요약 구조 비교**

| 구분        | `recv()`         | `WSARecv()`               |
| :-------- | :--------------- | :------------------------ |
| 호출 시점     | I/O 수행 및 완료 후 반환 | I/O 요청만 등록 후 즉시 반환        |
| 제어권       | 호출 스레드 유지        | 커널에게 위임                   |
| I/O 완료 알림 | 반환 시점            | IOCP 큐 이벤트로               |
| 구조체       | 없음               | `WSABUF`, `OVERLAPPED` 사용 |
| 모델        | 동기/논블로킹          | 완전 비동기                    |

---

## 🎯 **면접용 요약**

> `WSARecv` / `WSASend`는 `send` / `recv`와 달리
> **Overlapped 구조 기반 비동기 I/O 함수**이며,
> **IOCP(Completion Port)** 와 연계되어 I/O 완료를 커널이 **이벤트 큐로 통지**합니다.
>
> 이로 인해 스레드는 블로킹되지 않고, **수천 개의 소켓을 효율적으로 처리**할 수 있습니다.

---

### 🔥 꼬리질문 예상

1. **Overlapped I/O란 무엇인가요?**
   → I/O 요청을 커널에 등록하고, 완료 시점을 이벤트나 큐로 통보받는 방식.

2. **WSARecv에서 NULL 대신 실제 콜백 등록 시 차이는?**
   → 콜백 기반(`Completion Routine`)으로도 동작 가능하지만,
   IOCP에서는 대부분 NULL로 두고 큐 기반으로 처리.

3. **send/recv 기반 서버보다 IOCP 기반 서버의 장점은?**
   → 스레드 대기 없음, 컨텍스트 스위칭 최소, 대규모 동시접속 처리 가능.

</details>


## 15. Overlapped 구조체(WSAOVERLAPPED)는 어떤 역할을 하나요?

<details close>
<summary><strong>💡 자세한 정답</strong></summary>

<div style="background-color:#f6f8fa; padding:15px; border-radius:10px;">

## 💡 **Overlapped 구조체(WSAOVERLAPPED)의 역할**

> `WSAOVERLAPPED`(또는 `OVERLAPPED`) 구조체는
> **비동기 I/O 요청의 상태를 추적하고**,
> **I/O 완료 시 어떤 작업이 끝났는지 식별하기 위한 핵심 데이터 구조**입니다.
>
> 즉, **비동기 I/O 요청과 완료 이벤트를 매칭하기 위한 식별자 역할**을 수행합니다.

---

### ⚙️ **1️⃣ 구조체 정의**

```cpp
typedef struct _OVERLAPPED {
    ULONG_PTR Internal;      // I/O 상태 (커널이 관리)
    ULONG_PTR InternalHigh;  // 전송된 바이트 수 등 결과 정보
    DWORD Offset;            // 파일/소켓 오프셋 (파일 I/O 시 사용)
    DWORD OffsetHigh;
    HANDLE hEvent;           // 수동 이벤트 객체 핸들
} OVERLAPPED, *LPOVERLAPPED;
```

> `WSAOVERLAPPED`는 `OVERLAPPED`와 동일한 구조를 가지며,
> **Winsock 전용 비동기 함수(`WSARecv`, `WSASend` 등)** 에서 사용됩니다.

---

### ⚙️ **2️⃣ 주요 역할 요약**

|           항목           | 설명                                                |
| :--------------------: | :------------------------------------------------ |
|     **① 요청 상태 추적**     | 커널이 비동기 I/O의 진행 상태를 이 구조체에 저장                     |
|     **② 완료 시점 매칭**     | I/O 완료 후 어떤 요청이 끝났는지 구분                           |
| **③ IOCP 이벤트 전달용 포인터** | `GetQueuedCompletionStatus()` 호출 시 함께 반환되어 식별에 사용 |
|    **④ 수동 이벤트 동기화**    | (콜백 모드 사용 시) `hEvent`로 수동 대기 가능                   |

---

### ⚙️ **3️⃣ IOCP와의 연계 예시**

```cpp
// 세션 구조체 내 Overlapped 포함
struct Session {
    SOCKET sock;
    OVERLAPPED recvOv;
    OVERLAPPED sendOv;
    char buffer[4096];
};

// 비동기 수신 등록
WSABUF wsaBuf;
wsaBuf.buf = session->buffer;
wsaBuf.len = sizeof(session->buffer);
DWORD flags = 0;

WSARecv(session->sock, &wsaBuf, 1, NULL, &flags, &session->recvOv, NULL);
```

* 커널이 비동기 수신 요청을 받아 처리
* 완료 시 IOCP 큐에 다음 정보가 등록됨:

  * 완료된 바이트 수
  * 연결 키(`CompletionKey`)
  * `LPOVERLAPPED` (즉, `session->recvOv` 주소)

```cpp
DWORD bytes;
ULONG_PTR key;
LPOVERLAPPED overlapped;

GetQueuedCompletionStatus(iocp, &bytes, &key, &overlapped, INFINITE);
// overlapped == &session->recvOv → 어떤 세션의 어떤 I/O인지 식별 가능
```

📌 이렇게 `OVERLAPPED` 포인터를 통해
**“어떤 I/O 요청이 완료되었는가”를 정확히 매칭**할 수 있다.

---

### ⚙️ **4️⃣ 구조체가 필요한 이유**

|           이유          | 설명                               |
| :-------------------: | :------------------------------- |
|   **비동기 요청은 즉시 반환됨**  | 한 소켓에 여러 I/O가 동시에 등록될 수 있음       |
|  **I/O 완료 순서가 예측 불가** | 따라서 요청 단위로 상태를 추적해야 함            |
| **OVERLAPPED로 식별 가능** | 커널이 완료 시점에 이 구조체 포인터를 반환하여 매칭 수행 |

---

### ⚙️ **5️⃣ 여러 I/O 요청 관리 예시**

```cpp
// 1) 수신 요청
WSARecv(sock, &recvBuf, 1, NULL, &flags, &session->recvOv, NULL);

// 2) 송신 요청
WSASend(sock, &sendBuf, 1, NULL, 0, &session->sendOv, NULL);

// 3) 완료 통보 시
GetQueuedCompletionStatus(..., &overlapped, ...);

if (overlapped == &session->recvOv)
    OnRecvComplete(session);
else if (overlapped == &session->sendOv)
    OnSendComplete(session);
```

→ **Overlapped 포인터 비교만으로 작업 종류 구분 가능**.

---

## 🎯 **면접용 요약**

> `WSAOVERLAPPED` 구조체는 **비동기 I/O 요청의 상태를 추적하고**,
> **I/O 완료 시 어떤 요청이 끝났는지 식별하기 위한 커널 관리 구조체**입니다.
>
> IOCP 모델에서는 `GetQueuedCompletionStatus()`를 통해
> 이 구조체 포인터가 반환되므로,
> **완료된 세션과 작업 종류를 정확히 매칭**할 수 있습니다.

---

### 🔥 꼬리질문 예상

1. **하나의 소켓에 여러 Overlapped I/O를 동시에 걸 수 있나요?**
   → 가능합니다. 각 I/O 요청마다 별도의 `OVERLAPPED` 필요.

2. **hEvent 필드는 언제 사용되나요?**
   → IOCP가 아닌 이벤트 기반(WaitForSingleObject) 모델에서 동기 대기 시 사용.

3. **GetQueuedCompletionStatus에서 Overlapped가 NULL로 오는 경우는요?**
   → 타임아웃 발생, 또는 IOCP 큐가 강제로 종료(PostQueuedCompletionStatus)된 경우입니다.

</details>


## 16. IOCP에서 Worker Thread의 동작 흐름을 설명하세요

<details close>
<summary><strong>💡 자세한 정답</strong></summary>

<div style="background-color:#f6f8fa; padding:15px; border-radius:10px;">

## 💡 **IOCP의 Worker Thread 동작 흐름**

> IOCP(입출력 완료 포트)에서 **Worker Thread**는
> 커널이 큐에 등록한 **I/O 완료 이벤트를 가져와 처리하는 주체**입니다.
>
> 즉, **`GetQueuedCompletionStatus()`로 이벤트를 대기하고,
> 완료된 세션별 I/O 결과를 처리하는 루틴을 수행하는 스레드**입니다.

---

### ⚙️ **1️⃣ 전체 구조 요약**

```text
[Client] → [Kernel I/O 처리] → [IOCP Queue] → [Worker Thread Pool]
```

1. I/O 요청 (`WSARecv`, `WSASend`, `AcceptEx`) 등록
2. 커널이 완료 시 IOCP 큐에 이벤트 등록
3. Worker Thread가 큐에서 이벤트 꺼냄 (`GetQueuedCompletionStatus`)
4. 세션별 완료 처리 함수 호출 (`OnRecv`, `OnSend`, 등)

---

### ⚙️ **2️⃣ 동작 흐름 상세 단계**

|        단계       | 설명                                                              |
| :-------------: | :-------------------------------------------------------------- |
|   **① 대기 상태**   | Worker Thread는 `GetQueuedCompletionStatus()`를 호출하여 I/O 완료 큐를 대기 |
| **② 완료 이벤트 수신** | 커널이 I/O 완료 시, 해당 이벤트를 IOCP 큐에 푸시                                |
|   **③ 스레드 깨움**  | 대기 중인 Worker Thread 중 하나가 깨어나 이벤트 수신                            |
|   **④ 세션 식별**   | 반환된 `CompletionKey` 또는 `OVERLAPPED`를 통해 어떤 세션의 I/O인지 식별         |
|  **⑤ 처리 루틴 호출** | I/O 종류(수신, 송신, Accept)에 따라 적절한 콜백 또는 핸들러 함수 호출                  |
|  **⑥ 다음 대기 진입** | 처리 완료 후 다시 `GetQueuedCompletionStatus()`로 복귀, 다음 이벤트 대기         |

---

### ⚙️ **3️⃣ 실제 동작 예시 코드**

```cpp
void WorkerThread(HANDLE iocp)
{
    DWORD bytesTransferred;
    ULONG_PTR key;
    LPOVERLAPPED overlapped;

    while (true)
    {
        BOOL ret = GetQueuedCompletionStatus(
            iocp, &bytesTransferred, &key, &overlapped, INFINITE);

        if (ret == FALSE || bytesTransferred == 0)
        {
            // 세션 종료 또는 오류
            OnDisconnect((Session*)key);
            continue;
        }

        Session* session = reinterpret_cast<Session*>(key);

        // 어떤 I/O가 완료되었는지 구분
        if (overlapped == &session->recvOv)
            OnRecvComplete(session, bytesTransferred);
        else if (overlapped == &session->sendOv)
            OnSendComplete(session, bytesTransferred);
        else if (overlapped == &session->acceptOv)
            OnAcceptComplete(session);
    }
}
```

📌 **핵심 포인트:**

* `key` → 세션 포인터(또는 식별자)
* `overlapped` → 어떤 I/O 요청이 완료되었는지
* `bytesTransferred` → 실제 전송된 데이터 크기

---

### ⚙️ **4️⃣ 동작 다이어그램**

```
[Client] ---- send() ----> [Kernel I/O 처리]
                                 ↓
                      (I/O 완료 시 결과 등록)
                                 ↓
                       [IOCP Completion Queue]
                                 ↓
                 [Worker Thread] ← GetQueuedCompletionStatus()
                                 ↓
                   OnRecv / OnSend 등 처리
```

---

### ⚙️ **5️⃣ 스레드 풀 운영 방식**

|      항목     | 설명                            |
| :---------: | :---------------------------- |
|  **스레드 수**  | CPU 코어 수 × 2 정도 권장            |
| **커널 스케줄링** | 커널이 자동으로 스레드 Wake-up 조절       |
|    **장점**   | 컨텍스트 스위칭 최소, 효율적 CPU 활용       |
|    **결과**   | 수천 개 연결을 소수의 스레드로 안정적으로 처리 가능 |

---

### ⚙️ **6️⃣ 오류 처리 및 연결 종료 조건**

* `bytesTransferred == 0` → 클라이언트 정상 종료
* `ret == FALSE` → 네트워크 오류 또는 강제 종료
* 이런 경우 `OnDisconnect()` 호출 후 세션 정리

---

## 🎯 **면접용 요약**

> IOCP의 Worker Thread는
> **`GetQueuedCompletionStatus()`로 커널 큐의 I/O 완료 이벤트를 대기하고**,
> 이벤트가 발생하면 **세션별 Overlapped 정보를 통해 작업 종류를 식별**한 뒤
> **해당 처리 루틴(OnRecv, OnSend 등)** 을 호출합니다.
>
> 모든 처리가 끝나면 다시 대기 상태로 돌아가며,
> 이 구조 덕분에 **소수의 스레드로 수천 개의 연결을 효율적으로 처리**할 수 있습니다.

---

### 🔥 꼬리질문 예상

1. **IOCP에서 스레드 수를 너무 많이 만들면 어떻게 되나요?**
   → 컨텍스트 스위칭 오버헤드 증가, 성능 저하.

2. **Overlapped가 NULL로 오는 경우는요?**
   → 큐 종료 신호(PostQueuedCompletionStatus) 등 특수 상황.

3. **비동기 I/O와 IOCP의 근본 차이는?**
   → 비동기 I/O는 커널이 완료 시점을 관리, IOCP는 그 완료 이벤트를 큐로 통합 관리.

</details>


## 17. IOCP 모델의 장점은 무엇인가요?

<details close>
<summary><strong>💡 자세한 정답</strong></summary>

<div style="background-color:#f6f8fa; padding:15px; border-radius:10px;">

## 💡 **IOCP (I/O Completion Port) 모델의 장점**

> IOCP는 **Windows의 고성능 비동기 I/O 모델**로,
> 커널 수준에서 **I/O 완료 이벤트를 큐 단위로 관리**하고,
> **스레드 풀 기반으로 효율적 분산 처리**가 가능한 구조입니다.
>
> 즉, **“수천 개의 소켓을 소수의 스레드로 안정적·효율적으로 처리”** 하는 것이 IOCP의 핵심 강점입니다.

---

### ⚙️ **1️⃣ 주요 장점 요약**

|               항목              | 설명                                                           |
| :---------------------------: | :----------------------------------------------------------- |
| **① 커널 수준 대기 (효율적 Blocking)** | `GetQueuedCompletionStatus()`가 커널 객체를 직접 대기 → 불필요한 CPU 소모 없음 |
|      **② FD(소켓) 수 제약 없음**     | select처럼 1024 제한이 없고, 수천~수만 소켓 동시 처리 가능                      |
|       **③ 스레드 효율성 극대화**       | 커널이 스레드 Wake-up 시점을 제어 → 불필요한 스위칭 제거                         |
|   **④ Completion 기반 이벤트 처리**  | I/O 완료 후에만 이벤트 발생 → 불필요한 폴링(polling) 제거                      |
|   **⑤ 확장성(Scalability) 우수**   | CPU 코어 수에 맞춰 스레드 자동 조정 가능                                    |
|       **⑥ 컨텍스트 스위칭 최소화**      | 스레드 풀 재사용 + 커널 직접 통지 구조로 오버헤드 최소화                            |
|       **⑦ 높은 안정성과 신뢰성**       | 커널이 모든 I/O 완료를 직접 관리하므로, 유실·중복 이벤트 거의 없음                     |

---

### ⚙️ **2️⃣ 구조적 장점 비교**

|    구분    | select/poll            | epoll          | **IOCP**               |
| :------: | :--------------------- | :------------- | :--------------------- |
|  이벤트 모델  | FD 상태 기반 (ready)       | 이벤트 기반 (ready) | **완료 기반 (completion)** |
|   커널 연동  | 유저모드 반복 호출             | 커널 이벤트 감시      | **커널 큐 직접 관리**         |
|   FD 제한  | 1024 제한 (`FD_SETSIZE`) | 제한 없음          | **제한 없음**              |
|  스레드 효율  | 낮음 (모두 검사)             | 중간 (변경만 확인)    | **높음 (완료 이벤트만 처리)**    |
| 컨텍스트 스위칭 | 많음                     | 중간             | **최소화 (스레드 풀)**        |
|  적합한 규모  | 소규모                    | 중규모            | **대규모 고성능 서버**         |

---

### ⚙️ **3️⃣ 스레드 효율 극대화 원리**

* IOCP는 내부적으로 **스레드 풀(Thread Pool)** 을 운영함.
* 커널이 **현재 실행 중인 스레드 수를 감시**하고,
  I/O가 완료되면 **필요한 스레드만 깨움**.
* 결과적으로 **“CPU 코어 수 × 2” 수준의 스레드로 수천 소켓 처리** 가능.

```text
[Worker Thread 1] ← 처리 중
[Worker Thread 2] ← 대기
       ↓
커널이 완료 이벤트 발생 시 → 최소한의 스레드만 깨움
```

---

### ⚙️ **4️⃣ CPU 효율 및 확장성**

| 항목               | IOCP 효과                        |
| :--------------- | :----------------------------- |
| **CPU Idle 최소화** | 커널 대기 기반이라 busy loop 없음        |
| **스케줄링 효율**      | 스레드 풀 재사용으로 context switch 최소화 |
| **멀티코어 활용**      | 각 코어별 워커 스레드 균등 배분 가능          |
| **부하 분산**        | I/O 완료 이벤트가 자동 분산됨             |

---

### ⚙️ **5️⃣ 실전 적용 예시**

* **대규모 게임 서버 (예: MMORPG)**
  → 수천 명 동시 접속, 빠른 송수신 필요
* **웹 서버, 채팅 서버, DB 프록시 서버**
  → 다수의 클라이언트를 동시에 유지해야 하는 구조에 최적화

---

## 🎯 **면접용 요약**

> IOCP는 **Completion 기반 비동기 모델**로,
> **커널 수준에서 I/O 완료를 통지하고, 스레드 풀을 통해 효율적으로 분산 처리**합니다.
>
> 이로 인해
>
> * **FD 수 제약이 없고**,
> * **CPU 사용 효율이 높으며**,
> * **컨텍스트 스위칭 최소화**,
> * **대규모 동시 접속 환경에서도 안정적 성능 유지**가 가능합니다.

---

### 🔥 꼬리질문 예상

1. **epoll과 IOCP의 가장 큰 차이는 무엇인가요?**
   → epoll은 “준비(ready)” 상태 기반, IOCP는 “완료(completion)” 기반.

2. **IOCP에서 스레드 수를 많이 늘리면 더 빠를까요?**
   → 아니요, 커널이 자동 조정하므로 과도한 스레드는 오히려 오버헤드만 유발.

3. **IOCP에서 CPU 코어 수보다 많은 연결을 처리할 수 있는 이유는요?**
   → 비동기 I/O 기반이라 스레드가 I/O를 기다리지 않고 즉시 다른 이벤트 처리 가능.

</details>


## 18. IOCP에서 Overlapped I/O가 완료되면 커널은 어떻게 알려주나요?

<details close>
<summary><strong>💡 자세한 정답</strong></summary>

<div style="background-color:#f6f8fa; padding:15px; border-radius:10px;">

## 💡 **IOCP의 I/O 완료 통지 방식**

> IOCP는 **Completion 기반 비동기 모델**로,
> 비동기 I/O(`WSARecv`, `WSASend`, `AcceptEx` 등)가 완료되면
> **커널이 “Completion Packet(완료 패킷)”을 Completion Queue에 등록(post)** 하여
> Worker Thread에게 알리는 방식으로 동작합니다.

---

### ⚙️ **1️⃣ 전체 동작 흐름 요약**

```text
① Overlapped I/O 요청 등록 (WSARecv, WSASend)
② 커널에서 비동기 I/O 수행
③ I/O 완료 시, 커널이 Completion Packet 생성
④ 해당 Completion Port의 큐에 Packet 등록
⑤ Worker Thread가 GetQueuedCompletionStatus()로 수신
```

---

### ⚙️ **2️⃣ 단계별 상세 동작**

|              단계              | 설명                                                                         |
| :--------------------------: | :------------------------------------------------------------------------- |
|        **① I/O 요청 등록**       | `WSARecv` / `WSASend` 호출 시 `OVERLAPPED` 구조체와 함께 비동기 요청이 커널에 전달             |
|        **② 커널 비동기 처리**       | 실제 송·수신은 커널 I/O 시스템에서 비동기로 수행                                              |
|          **③ 완료 시점**         | 작업 완료 후 커널이 “어떤 요청이 끝났는가”를 식별 (`OVERLAPPED` 주소 기반)                         |
|  **④ Completion Packet 생성**  | 커널이 I/O 결과(전송 바이트 수, 세션 키, Overlapped 포인터 등)를 포함한 **Completion Packet** 생성 |
| **⑤ Completion Queue에 Post** | 해당 I/O Completion Port의 **큐(Completion Queue)** 에 Packet을 push             |
|    **⑥ Worker Thread 수신**    | 대기 중인 스레드가 `GetQueuedCompletionStatus()`를 통해 Packet을 pop 받아 처리             |

---

### ⚙️ **3️⃣ 커널 내부 개념도**

```
[User Thread]
  |--WSARecv() 등록--------------------> [Kernel I/O Subsystem]
  |                                      ↓
  |                                (비동기 송수신 수행)
  |                                      ↓
  |                         [Completion Packet 생성]
  |                                      ↓
  |------------ Completion Queue(Post) -----------> [IOCP Handle]
                                                 ↓
                                   [Worker Thread] ← GetQueuedCompletionStatus()
```

---

### ⚙️ **4️⃣ Completion Packet 구조 (커널 내부)**

|          필드          | 설명                                              |
| :------------------: | :---------------------------------------------- |
|   **CompletionKey**  | `CreateIoCompletionPort()` 호출 시 등록한 세션 포인터(식별자) |
| **BytesTransferred** | 송수신된 데이터 크기                                     |
|   **LPOVERLAPPED**   | 어떤 Overlapped I/O 요청이 완료되었는지 식별용 포인터            |

→ 이 3가지 정보가 **`GetQueuedCompletionStatus()` 호출 결과로 반환**됨.

---

### ⚙️ **5️⃣ Worker Thread 수신 예시**

```cpp
DWORD bytes;
ULONG_PTR key;
LPOVERLAPPED overlapped;

BOOL ret = GetQueuedCompletionStatus(iocp, &bytes, &key, &overlapped, INFINITE);

if (ret && bytes > 0)
{
    Session* session = reinterpret_cast<Session*>(key);

    if (overlapped == &session->recvOv)
        OnRecvComplete(session, bytes);
    else if (overlapped == &session->sendOv)
        OnSendComplete(session, bytes);
}
```

→ Worker Thread는 이 정보를 통해
**“어떤 세션의 어떤 I/O가 완료되었는가”** 를 정확히 파악.

---

### ⚙️ **6️⃣ 커널 통지 방식의 장점**

|      항목      | 설명                                  |
| :----------: | :---------------------------------- |
|  **스레드 효율**  | 커널이 완료 이벤트 발생 시점에만 스레드 깨움           |
| **오버헤드 최소화** | 폴링 불필요, 불필요한 wake-up 제거             |
|    **확장성**   | 수천 개 소켓 동시 처리 가능 (Completion 기반 구조) |
|    **안정성**   | 이벤트 유실 없음 — 커널이 직접 큐 관리             |

---

## 🎯 **면접용 요약**

> IOCP에서 Overlapped I/O가 완료되면
> 커널은 **Completion Packet을 생성하여 Completion Queue에 등록(Post)** 하고,
> 대기 중인 Worker Thread가 **`GetQueuedCompletionStatus()`** 로 이를 꺼내 처리합니다.
>
> 즉, IOCP는 **커널이 직접 완료 이벤트를 큐 단위로 통지하는 구조**로,
> 스레드가 폴링 없이 효율적으로 완료 알림을 받을 수 있습니다.

---

### 🔥 꼬리질문 예상

1. **Completion Packet에는 어떤 정보가 들어있나요?**
   → `CompletionKey`, `LPOVERLAPPED`, `BytesTransferred`.

2. **GetQueuedCompletionStatus는 언제 반환되나요?**
   → Completion Queue에 Packet이 들어오거나, 에러/종료 신호(PostQueuedCompletionStatus) 발생 시.

3. **커널이 여러 스레드에게 동시에 알릴 수 있나요?**
   → 아니요. 하나의 완료 이벤트는 **단 한 개의 스레드에게만 전달**되어 중복 처리 방지.

</details>

## 19. 하나의 세션(Session)이 여러 번의 `WSARecv`를 동시에 요청해도 되나요?

<details close>
<summary><strong>💡 자세한 정답</strong></summary>

<div style="background-color:#f6f8fa; padding:15px; border-radius:10px;">

## 💡 **하나의 세션에서 다중 WSARecv 동시 요청 가능 여부**

> 결론적으로 **가능합니다.**
>
> 단, 각 `WSARecv` 호출마다 **별도의 Overlapped 구조체와 버퍼**를 사용해야 하며,
> **수신 순서 보장 및 버퍼 중복 관리**에 주의해야 합니다.

---

### ⚙️ **1️⃣ 기본 개념**

> IOCP의 `WSARecv()`는 **비동기(Overlapped) I/O 요청 등록 함수**입니다.
> 즉, 호출 즉시 반환되며 커널이 백그라운드에서 실제 수신을 처리합니다.

따라서 하나의 세션 소켓에 대해:

```cpp
WSARecv(sock, &buf1, 1, NULL, &flags, &ov1, NULL);
WSARecv(sock, &buf2, 1, NULL, &flags, &ov2, NULL);
```

처럼 여러 I/O 요청을 동시에 등록하는 것이 **합법적**입니다.

커널은 내부적으로 각 요청을 **큐에 등록**하여,
데이터가 들어오는 대로 순차적으로 처리 후 IOCP 큐에 결과를 **Completion Packet** 형태로 통보합니다.

---

### ⚙️ **2️⃣ 가능한 이유**

|           항목          | 설명                                    |
| :-------------------: | :------------------------------------ |
| **Overlapped I/O 구조** | 각 요청마다 독립적인 `OVERLAPPED` 구조체 존재       |
|    **비동기 커널 큐 처리**    | 커널이 소켓 단위로 요청 큐를 관리                   |
|      **순서 제약 없음**     | 각 요청은 독립 실행, 완료 순서는 데이터 도착 순서에 따라 달라짐 |

→ 즉, 한 소켓이 동시에 여러 I/O 요청을 커널에 등록 가능.

---

### ⚙️ **3️⃣ 주의할 점**

|             항목            | 주의 내용                                                    |
| :-----------------------: | :------------------------------------------------------- |
|     **① 버퍼 중복 사용 금지**     | 동일한 버퍼 주소를 여러 `WSARecv` 요청에 재사용하면, 커널이 쓰기 충돌 발생          |
| **② Overlapped 구조 공유 금지** | `OVERLAPPED`는 커널이 I/O 완료 후 덮어쓰므로, 요청마다 별도 객체 필요          |
|       **③ 순서 보장 불가**      | 여러 요청이 동시에 완료될 수 있으므로, 수신 데이터 순서를 **애플리케이션 레벨에서 재조립** 필요 |
|     **④ 과도한 중첩 요청 주의**    | 필요 이상으로 많은 I/O를 미리 걸면 커널 메모리 부담 증가 가능                    |

---

### ⚙️ **4️⃣ 안전한 사용 예시**

```cpp
// 각 요청마다 독립된 Overlapped와 버퍼 사용
struct RecvContext {
    WSAOVERLAPPED overlapped;
    WSABUF buffer;
    char data[4096];
};

RecvContext recvCtx1 = {};
RecvContext recvCtx2 = {};

DWORD flags = 0;
WSARecv(sock, &recvCtx1.buffer, 1, NULL, &flags, &recvCtx1.overlapped, NULL);
WSARecv(sock, &recvCtx2.buffer, 1, NULL, &flags, &recvCtx2.overlapped, NULL);
```

> 커널은 두 요청을 모두 대기열에 등록하고,
> 데이터가 도착할 때마다 각각 **별도의 Completion Packet**으로 IOCP 큐에 통보합니다.

---

### ⚙️ **5️⃣ 수신 순서 관리 예시**

```cpp
// Worker Thread에서 Completion 수신
if (overlapped == &session->recvCtx1.overlapped)
    OnRecvComplete(session, recvCtx1.buffer);
else if (overlapped == &session->recvCtx2.overlapped)
    OnRecvComplete(session, recvCtx2.buffer);
```

→ 애플리케이션 레벨에서 **수신 시점에 따라 데이터 정렬 / 조립** 필요.

---

### ⚙️ **6️⃣ 일반적인 서버 구현 패턴**

대부분의 고성능 IOCP 서버는 다음과 같은 방식으로 동작합니다.

|        패턴       | 설명                                          |
| :-------------: | :------------------------------------------ |
| **단일 수신 요청 유지** | 한 번의 `WSARecv` 완료 후 다음 `WSARecv` 즉시 재등록     |
|      **이유**     | 수신 순서 유지 용이, 메모리 관리 단순                      |
|      **예시**     | `OnRecvComplete()` 내에서 다음 `WSARecv()` 호출 등록 |

```cpp
void OnRecvComplete(Session* s, int bytes) {
    ProcessPacket(s->buffer, bytes);
    PostRecv(s); // 다음 수신 재등록
}
```

---

## 🎯 **면접용 요약**

> 하나의 세션에서 **여러 번의 `WSARecv`를 동시에 요청하는 것은 가능**하지만,
> 각 요청마다 **독립된 Overlapped 구조체와 버퍼를 사용해야 하며**,
> **데이터 순서 보장 및 중복 버퍼 관리**에 주의해야 합니다.
>
> 대부분의 서버는 이를 단순화하기 위해
> “**하나의 수신 완료 후 다음 수신 재등록 방식**”을 사용합니다.

---

### 🔥 꼬리질문 예상

1. **Overlapped를 재사용하면 왜 위험한가요?**
   → 커널이 내부적으로 I/O 상태를 덮어쓰기 때문에 충돌 발생.

2. **다중 `WSARecv`를 쓰면 성능이 더 좋은가요?**
   → 경우에 따라 다름. 보통 단일 재등록 방식이 더 단순하고 안정적.

3. **수신 순서를 보장하려면 어떻게 해야 하나요?**
   → 애플리케이션 레벨에서 시퀀스 넘버 부여 또는 패킷 조립 로직 추가.

</details>


## 20. 🔥[심화] IOCP 기반 서버에서 스레드 풀 크기는 어떻게 결정하나요?

<details close>
<summary><strong>💡 자세한 정답</strong></summary>

<div style="background-color:#f6f8fa; padding:15px; border-radius:10px;">

## 💡 **결론 우선**

* **IO 전용(IOCP 워커)**: **논리 코어 수 = 기본값**(권장), 최대 **코어×2**까지 상황에 따라 확대
* **CPU 작업(패킷 처리/인코딩 등)**: **별도 작업 큐(스레드 풀 분리)**, **코어×(0.5~1.0)** 범위에서 부하에 맞춰 조정
* **원칙**: I/O 완료 처리(짧고 가벼움)는 “적게·빠르게”, 무거운 CPU 작업은 “분리·병렬화”

---

### ⚙️ 1️⃣ IOCP의 **Concurrency** 개념 (핵심)

* `CreateIoCompletionPort`의 **NumberOfConcurrentThreads** = **동시에 runnable 가능한 워커 상한**
* **0 지정 시 OS 기본값(= 논리 코어 수)**
* 이 상한까지만 커널이 워커를 **깨워서** 실행 → **불필요한 컨텍스트 스위칭 억제**

**권장 기본값**

* **I/O 중심 서버(워크가 매우 짧음)**: **= 논리 코어 수**
* **I/O는 짧고 콜백에서 가벼운 파싱만**: **= 코어** 또는 **코어+1**
* **콜백이 드물게 무거워짐**: **코어~코어×2** 범위에서 계단식 확장(측정 기반)

---

### ⚙️ 2️⃣ I/O와 CPU 작업 **분리 전략**

* **워커(Completion 처리)**: `OnRecv/OnSend/OnAccept`에서 **최소 작업**만 수행(파싱·큐잉)
* **CPU 풀(별도 JobQueue)**: 패킷 로직, 암호화, DB 직전 전처리 등 **무거운 일**을 오프로딩
* 효과: IOCP 워커는 **짧게 반환** → **큐 체류 시간↓**, **스레드 경합↓**

```text
IOCP Worker: Dequeue completion → 최소 파싱 → JobQueue.Enqueue(task) → 즉시 다음 GQCS 대기
CPU Pool  : Dequeue task → 로직 처리 → 필요 시 SendQueue 등록
```

---

### ⚙️ 3️⃣ **초기 설정 가이드 (실무형)**

1. **IOCP Concurrency = 논리 코어 수**로 시작
2. **CPU 풀 스레드 수 = 코어×0.5 ~ 1.0**
3. **송신 전용 스레드**가 필요하면 1~2개(배치/집합 전송)로 제한

---

### ⚙️ 4️⃣ **증감 판단 지표(측정으로 튜닝)**

다음 지표를 1~5분 간격으로 관찰, 한 번에 1단계씩만 조정:

* **GQCS 대기 시간**(p95) ↑, **Completion 큐 길이**(대기 이벤트 수) ↑ → **IOCP 워커 +1**
* **CPU 사용률 100% 근접**, **컨텍스트 스위칭** 과다, **런큐 길이** ↑ → **워커 과다** (−1)
* **JobQueue 대기 시간**↑, **Task 완료 지연**↑ → **CPU 풀 +1**
* **캐시 미스/메모리 대역폭** 포화 시: 스레드 추가 대신 **작업 크기 줄이기/데이터 지역성 개선**

**간단 규칙**

```
if (IOCP_queue_len ↑ and CPU_usage < 85%)  IOCP_workers++
if (run_queue_len ↑ or ctx_switch ↑ or CPU_usage > 95%)  IOCP_workers--
if (job_queue_wait ↑)  cpu_pool_workers++
```

---

### ⚙️ 5️⃣ **작업 특성에 따른 선택**

| 작업 특성                     | 권장                                              |
| ------------------------- | ----------------------------------------------- |
| 대부분 **I/O-bound** (짧은 콜백) | IOCP = **코어 수**, CPU 풀 **작게**                   |
| **혼합형** (간헐적 CPU 스파이크)    | IOCP = **코어~코어+1**, CPU 풀 **코어×0.5~1.0**        |
| **CPU-bound** 비중 큼        | IOCP = **코어** 유지, CPU 풀 **코어×1.0~1.5** + 작업 쪼개기 |

---

### ⚙️ 6️⃣ **NUMA / Affinity 최적화(옵션)**

* **NUMA 노드별** IOCP 워커/CPU 풀 분할, **메모리 할당 지역화**
* **핫 코어 고정(Affinity)** 는 장기 부하에서 **캐시 로컬리티** 개선 효과

---

### ⚙️ 7️⃣ **반패턴(피해야 할 설정)**

* **“코어×N” 무지성 확대**: 컨텍스트 스위칭↑, 캐시 슬래싱↑
* **IOCP 콜백에서 무거운 로직**: Completion 큐 체증 → 지연 폭발
* **단일 거대 락**: 스레드 늘릴수록 경합만 증가

---

### ⚙️ 8️⃣ **면접용 한 줄 정리**

> **IOCP Concurrency는 기본적으로 논리 코어 수**로 두고,
> 콜백은 **가볍게** 처리한 뒤 **CPU 풀로 오프로딩**합니다.
> **지표(GQCS 대기, 큐 길이, CPU/CS)** 를 보며 **코어~코어×2** 범위에서 미세 조정합니다.

---

## 🎯 **면접용 요약**

* **기본값**: IOCP 워커 = **논리 코어 수**
* **확대 상한**: **코어×2**(측정 기반)
* **CPU 작업은 분리 풀**에서 처리(코어×0.5~1.0)
* **증감 기준**: Completion 큐 길이, GQCS 대기, CPU 사용률/컨텍스트 스위칭, JobQueue 대기

---

### 🔥 꼬리질문 예상

1. **왜 코어×2까지 열어두나요?**
   → I/O 완료 폭주·페이지 폴트 등으로 잠시 막힐 때 **숨통 확보**, 과도한 스위칭 전까지는 효과.
2. **워커를 줄여야 하는 신호는?**
   → CPU 95%↑, 컨텍스트 스위칭↑, 런큐 길이↑, 지연 개선 없이 오히려 악화.
3. **콜백이 무거우면?**
   → **즉시 오프로딩**(JobQueue). IOCP 워커는 빠르게 반환해 **큐 체증 방지**.

</div>
</details>

## 21. 클라이언트-서버 구조와 P2P 구조의 차이점은 무엇인가요?

<details close>
<summary><strong>💡 자세한 정답</strong></summary>

<div style="background-color:#f6f8fa; padding:15px; border-radius:10px;">

## 💡 **클라이언트-서버 구조 vs P2P 구조**

> 네트워크 아키텍처는 크게 **클라이언트-서버(Client–Server)** 와 **P2P(Peer-to-Peer)** 로 나뉩니다.
> 두 구조는 **데이터 흐름의 방향**, **중앙 집중성**, **확장성** 측면에서 차이를 가집니다.

---

### ⚙️ **1️⃣ 핵심 비교표**

|          구분          | **클라이언트-서버 구조**           | **P2P 구조**              |
| :------------------: | :------------------------ | :---------------------- |
|    **중심 노드 존재 여부**   | ✅ 중앙 서버 존재                | ❌ 중앙 서버 없음 (모든 노드가 대등)  |
|     **데이터 흐름 방향**    | Client → Server → Client  | Peer ↔ Peer (직접 교환)     |
|       **관리 주체**      | 중앙 서버가 전체 관리              | 각 노드가 자율 관리             |
| **확장성(Scalability)** | 서버 부하에 따라 한계 존재           | 노드가 늘수록 처리량 증가          |
|        **신뢰성**       | 서버 장애 시 전체 영향             | 일부 노드 장애는 영향 제한적        |
|      **보안 / 제어**     | 중앙 집중 관리로 용이              | 보안·인증 어려움               |
|     **속도 / 효율성**     | 서버 경유로 안정적이지만 경로 길어질 수 있음 | 근접 노드 간 직접 통신으로 빠를 수 있음 |
|       **대표 예시**      | 온라인 게임 서버, 웹 서비스, DB 서버   | 토렌트, 블록체인, 메신저 P2P 전송   |

---

### ⚙️ **2️⃣ 구조 개념도**

#### 🔹 **클라이언트-서버 구조**

```
   [Client A] \
                → [Server] ← [Client B]
   [Client C] /
```

* 서버가 **데이터 처리 및 중계의 중심**
* 모든 클라이언트는 서버를 통해서만 통신

#### 🔹 **P2P 구조**

```
   [Peer A] ↔ [Peer B] ↔ [Peer C]
            ↖──────────────↗
```

* 모든 피어가 동등한 역할
* 직접 연결하여 데이터 공유 및 교환

---

### ⚙️ **3️⃣ 실제 활용 예시**

| 분야           | 클라이언트-서버          | P2P                          |
| :----------- | :---------------- | :--------------------------- |
| **게임**       | MMORPG 서버(넥슨, NC) | 소규모 Co-op (예: 유저 간 직접 세션 연결) |
| **파일 공유**    | 웹 다운로드            | 토렌트, eMule                   |
| **금융 / 데이터** | 중앙 서버 기반 결제       | 블록체인, 분산원장                   |
| **메신저**      | 서버 중계 메시지         | 일부 WebRTC 기반 영상통화            |

---

### ⚙️ **4️⃣ 게임 서버 관점에서의 비교**

| 항목         | 클라이언트-서버 방식        | P2P 방식                         |
| :--------- | :----------------- | :----------------------------- |
| **장점**     | 보안, 동기화, 일관성 유지 쉬움 | 서버 부하 없음, 응답 지연↓               |
| **단점**     | 서버 부하 집중, 유지 비용↑   | 해킹·패킷 위조 위험↑, NAT Traversal 문제 |
| **적합한 용도** | MMORPG, 대규모 멀티플레이  | 1:1 대전, 소규모 실시간 협력             |

---

## 🎯 **면접용 요약**

> **클라이언트-서버 구조**는 중앙 서버가 통신을 중개하고 관리하는 방식으로,
> **안정성과 제어성이 뛰어나지만 서버 부하와 비용이 크고**,
>
> **P2P 구조**는 모든 노드가 직접 연결되어 데이터를 교환하므로
> **확장성과 효율은 높지만 보안·동기화 관리가 어렵습니다.**

---

### 🔥 꼬리질문 예상

1. **P2P 구조를 게임 서버에서 잘 안 쓰는 이유는요?**
   → 패킷 위조·속도 핵 가능성, NAT 문제, 동기화 불일치.

2. **하이브리드 구조란 무엇인가요?**
   → 기본은 서버 중심, 일부 실시간 교환(음성, 영상 등)은 P2P로 처리하는 혼합형.

3. **MMORPG가 반드시 서버-클라이언트 구조를 사용하는 이유는요?**
   → 보안, 일관성, 동기화(Authority) 유지가 핵심이기 때문.

</div>
</details>


## 22. 세션(Session)과 커넥션(Connection)의 차이는 무엇인가요?

<details close>
<summary><strong>💡 자세한 정답</strong></summary>

<div style="background-color:#f6f8fa; padding:15px; border-radius:10px;">

## 💡 **Session vs Connection**

> 네트워크 프로그래밍에서 **Connection(연결)** 은 **물리적 통신 경로**,
> **Session(세션)** 은 그 위에서 유지되는 **논리적 상태(context)** 를 의미합니다.
>
> 즉, Connection은 **TCP 소켓의 연결 상태**,
> Session은 **클라이언트의 논리적 단위(유저 정보, 게임 상태 등)** 입니다.

---

### ⚙️ **1️⃣ 핵심 비교표**

|      구분      | **Connection**                 | **Session**                |
| :----------: | :----------------------------- | :------------------------- |
|    **의미**    | 실제 네트워크 상의 연결(TCP 소켓)          | 연결 위에서 유지되는 논리적 상태/컨텍스트    |
|    **계층**    | 전송 계층 (TCP)                    | 응용 계층 (App Logic)          |
|   **관리 주체**  | OS / 네트워크 라이브러리                | 애플리케이션 서버                  |
|    **수명**    | TCP 연결이 유지되는 동안                | 로그인~로그아웃 등 논리 단위로 더 길거나 짧음 |
| **복수 매핑 여부** | 하나의 클라이언트 ↔ 하나의 커넥션            | 하나의 세션이 여러 커넥션을 가질 수도 있음   |
|    **예시**    | 소켓 연결, `connect()`, `accept()` | 로그인 상태, 사용자 ID, 게임방 정보     |

---

### ⚙️ **2️⃣ 개념적으로 구분하자면**

#### 🔹 Connection (연결)

> “물리적인 통신 통로”

* TCP 3-way handshake로 수립
* 클라이언트 ↔ 서버 간 데이터 송수신 통로
* 연결 종료 시 (4-way handshake) 자동 해제

```cpp
// 예: 소켓 연결
SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
connect(sock, ...); // TCP 연결(Connection) 수립
```

---

#### 🔹 Session (세션)

> “논리적인 사용자 상태”

* Connection 위에서 관리되는 **사용자 단위의 논리 상태 정보**
* 로그인 정보, 사용자 ID, 현재 위치, 방 참여 상태 등
* Connection이 끊겨도 세션은 일정 시간 유지 가능(예: 재접속)

```cpp
struct Session {
    SOCKET sock;
    std::string userId;
    Room* currentRoom;
    Buffer recvBuf;
    Buffer sendBuf;
};
```

---

### ⚙️ **3️⃣ 서버 입장에서의 관계**

```
[Client]
   ↓
  TCP Connection  ← 커널/IOCP 관리 (물리적 연결)
   ↓
[Session Object]  ← 애플리케이션 관리 (논리 상태)
```

* 커넥션(Connection): **소켓 I/O 단위 (OS 관리)**
* 세션(Session): **게임/서비스 단위 (서버 코드 관리)**
* 커넥션이 끊기면 세션 종료 처리 → or 재접속으로 세션 복구 가능

---

### ⚙️ **4️⃣ 예시 시나리오**

| 상황              | Connection    | Session                   |
| :-------------- | :------------ | :------------------------ |
| **TCP 연결 수립 시** | 소켓 생성 및 연결    | 세션 객체 생성                  |
| **데이터 수신 시**    | I/O 완료 이벤트 발생 | 세션 단위로 패킷 파싱/처리           |
| **네트워크 끊김 시**   | 소켓 종료         | 세션 상태 ‘Disconnected’ 로 전환 |
| **재접속 시**       | 새 커넥션 생성      | 이전 세션 복구 또는 재인증           |

---

### ⚙️ **5️⃣ 실제 게임 서버 구조 예시**

```cpp
class Session {
public:
    void OnConnected();
    void OnRecv(const PacketHeader* header);
    void OnSend();
    void OnDisconnected();
private:
    SOCKET _socket;          // Connection
    Player* _playerContext;  // Session 논리 상태
};
```

→ Connection은 OS 레벨 자원(소켓)
→ Session은 게임 로직 레벨 컨텍스트

---

## 🎯 **면접용 요약**

> **Connection**은 TCP 소켓을 통한 **물리적 네트워크 연결**,
> **Session**은 그 위에서 관리되는 **논리적 사용자 상태**입니다.
>
> Connection이 끊기면 물리적 통신은 종료되지만,
> Session은 애플리케이션에서 별도로 관리되므로 **재접속·복구·유지**가 가능합니다.

---

### 🔥 꼬리질문 예상

1. **Connection이 끊겨도 Session이 유지될 수 있나요?**
   → 가능합니다. 세션 상태를 별도 저장(DB, 메모리)하면 재접속 시 복구 가능.

2. **한 세션이 여러 커넥션을 가지는 예시는?**
   → 모바일 게임에서 같은 계정으로 여러 디바이스 로그인 시.

3. **IOCP에서 세션과 소켓(Connection)은 어떻게 연결되나요?**
   → 소켓을 `CreateIoCompletionPort()`로 IOCP에 등록하고,
   세션 객체 포인터를 CompletionKey로 연결하여 관리합니다.

</div>
</details>


## 23. 프로토콜 설계 시 직렬화(Serialization)가 필요한 이유는 무엇인가요?

<details close>
<summary><strong>💡 자세한 정답</strong></summary>

<div style="background-color:#f6f8fa; padding:15px; border-radius:10px;">

## 💡 **직렬화(Serialization)의 필요성**

> **직렬화(Serialization)** 는
> 프로그램 내부의 **데이터 구조(메모리 상 객체)** 를
> **네트워크로 전송 가능한 형태(바이트 스트림)** 로 변환하는 과정입니다.
>
> 즉, 서로 다른 시스템 간에 데이터를 **일관성 있게 주고받기 위해 필수적인 과정**입니다.

---

### ⚙️ **1️⃣ 왜 필요한가?**

|           구분           | 설명                                     |
| :--------------------: | :------------------------------------- |
| **1. 전송 가능 형태로 변환 필요** | 객체는 포인터, 패딩 등 메모리 의존 요소 포함 → 그대로 전송 불가 |
|   **2. 플랫폼 간 호환성 보장**  | CPU 아키텍처(엔디안, 정렬 방식)마다 메모리 표현 다름       |
|     **3. 프로토콜 명확화**    | 송신 측과 수신 측이 동일한 구조로 해석 가능              |
|  **4. 디버깅 및 유지보수 용이**  | 메시지 구조 명시 → 버전 관리 및 확장 용이              |

---

### ⚙️ **2️⃣ 직렬화 vs 비직렬화**

|     구분    | 직렬화(Serialization)         | 역직렬화(Deserialization)  |
| :-------: | :------------------------- | :--------------------- |
|   **역할**  | 객체 → 바이트 스트림 변환            | 바이트 스트림 → 객체 복원        |
| **수행 위치** | 송신 측                       | 수신 측                   |
|   **예시**  | `Send(packet.Serialize())` | `packet.Parse(buffer)` |

---

### ⚙️ **3️⃣ 예시 코드 (C++ 기반 게임 패킷)**

#### 🔹 송신 측 (Serialize)

```cpp
struct PlayerInfo {
    int id;
    float posX, posY;
};

void Serialize(char* buffer, const PlayerInfo& info) {
    memcpy(buffer, &info.id, sizeof(int));
    memcpy(buffer + 4, &info.posX, sizeof(float));
    memcpy(buffer + 8, &info.posY, sizeof(float));
}
```

#### 🔹 수신 측 (Deserialize)

```cpp
void Deserialize(const char* buffer, PlayerInfo& info) {
    memcpy(&info.id, buffer, sizeof(int));
    memcpy(&info.posX, buffer + 4, sizeof(float));
    memcpy(&info.posY, buffer + 8, sizeof(float));
}
```

💡 즉, **객체의 메모리 표현이 아닌, 정의된 프로토콜 순서에 따라** 데이터를 변환/복원.

---

### ⚙️ **4️⃣ 직렬화가 없다면 생기는 문제**

|       문제      | 설명                               |
| :-----------: | :------------------------------- |
|  **플랫폼 불일치**  | x86(리틀 엔디안) ↔ ARM(빅 엔디안) 간 값 뒤집힘 |
| **구조체 패딩 문제** | 구조체 내 패딩 바이트가 포함되어 전송 데이터 깨짐     |
|   **버전 충돌**   | 송신자와 수신자 구조체 변경 시 오해석 발생         |
|   **보안 위험**   | 포인터, 내부 메모리 직접 전송 시 비정상 접근 가능    |

---

### ⚙️ **5️⃣ 대표적인 직렬화 방식**

|               방식              | 설명                   | 예시             |
| :---------------------------: | :------------------- | :------------- |
|    **Binary Serialization**   | 고정 길이, 빠른 전송         | 게임 프로토콜, RPC   |
|   **Text-based (JSON/XML)**   | 가독성 높음, 용량 큼         | REST API, 로그   |
|      **Protocol Buffers**     | Google 설계, Schema 기반 | gRPC, 멀티플랫폼 통신 |
| **FlatBuffers / MessagePack** | 고성능, Zero-copy 구조    | 실시간 게임, IoT 통신 |

---

### ⚙️ **6️⃣ 게임 서버에서의 중요성**

* 클라이언트 ↔ 서버 간 데이터 송수신은 모두 **패킷 단위**로 이루어짐
* 패킷은 **명시적 구조(헤더 + Payload)** 로 정의되어야 함
* 직렬화를 통해 서버와 클라이언트가 **동일한 규약으로 해석** 가능

```text
[Packet 구조]
[Header: ID, Size] + [Payload: Serialized Data]
```

---

## 🎯 **면접용 요약**

> 직렬화는 **메모리 객체를 전송 가능한 바이트 스트림으로 변환하는 과정**이며,
> 서로 다른 시스템 간에 **데이터의 일관성과 호환성**을 보장하기 위해 필요합니다.
>
> 이를 통해 **엔디안, 패딩, 구조체 차이로 인한 데이터 깨짐을 방지**하고,
> 명확한 **프로토콜 기반 통신 구조**를 구현할 수 있습니다.

---

### 🔥 꼬리질문 예상

1. **엔디안(Endian) 문제는 어떻게 해결하나요?**
   → `htonl()`, `htons()` 같은 네트워크 바이트 오더 변환 사용.

2. **직렬화 시 성능 병목이 생기면 어떻게 최적화하나요?**
   → 미리 정의된 Schema 기반(Binary Format, FlatBuffers 등) 사용.

3. **직렬화 계층은 어느 계층에 해당하나요?**
   → OSI 7계층 중 **응용 계층(Application Layer)** 에서 동작.

</div>
</details>


## 24. 바이너리(Binary) 프로토콜과 JSON 프로토콜의 차이 및 선택 기준은 무엇인가요?

<details close>
<summary><strong>💡 자세한 정답</strong></summary>

<div style="background-color:#f6f8fa; padding:15px; border-radius:10px;">

## 💡 **Binary vs JSON Protocol**

> **프로토콜(Protocol)** 은 데이터를 송수신하기 위한 형식(Format)과 규칙입니다.
> 이 중에서도 **Binary** 와 **JSON(Text)** 방식은 가장 널리 쓰이는 두 가지 직렬화 형태이며,
> **성능·가독성·호환성**의 균형을 고려해 선택합니다.

---

### ⚙️ **1️⃣ 핵심 비교표**

|       구분      | **Binary Protocol**       | **JSON Protocol**        |
| :-----------: | :------------------------ | :----------------------- |
|     **형식**    | 이진(Binary) 형식             | 텍스트(Text, UTF-8) 형식      |
|    **가독성**    | 낮음 (사람이 읽기 어려움)           | 높음 (직접 읽고 디버깅 가능)        |
|   **데이터 크기**  | 작음 (압축 효율 높음)             | 큼 (문자열 표현 오버헤드)          |
|   **파싱 속도**   | 빠름 (고정 길이 기반)             | 느림 (문자열 파싱 필요)           |
|    **호환성**    | 양측이 동일한 구조체 정의 필요         | 동적 구조 가능 (Schema 불필요)    |
| **디버깅 / 테스트** | 어려움                       | 쉬움 (텍스트 출력 가능)           |
|   **대표 예시**   | TCP 게임 서버, gRPC(Protobuf) | REST API, WebSocket JSON |
|   **적합한 용도**  | 고속·대량 데이터 통신              | 유연한 클라이언트 통신 (웹/모바일)     |

---

### ⚙️ **2️⃣ 예시 비교**

#### 🔹 **Binary 형식**

```text
[Header][Size][Payload]
0x01 | 0x0008 | 0x0000000A 0x0000000B
```

> * 효율적이지만 사람이 보기 어렵고
> * 구조체 정의에 따라 고정 길이로 직렬화/역직렬화 수행

#### 🔹 **JSON 형식**

```json
{
  "id": 1,
  "x": 10,
  "y": 11
}
```

> * 가독성 우수, 네트워크 디버깅 쉬움
> * 하지만 문자열 기반이라 **전송 크기↑**, **파싱 비용↑**

---

### ⚙️ **3️⃣ 성능 측면**

|         항목        | Binary  | JSON          |
| :---------------: | :------ | :------------ |
|     **전송 효율**     | ✅ 작고 빠름 | ❌ 문자열 변환 오버헤드 |
|     **CPU 부하**    | ✅ 낮음    | ❌ 파싱/직렬화 비용 큼 |
| **Bandwidth 사용량** | ✅ 적음    | ❌ 많음          |
|    **디버깅 편의성**    | ❌ 낮음    | ✅ 매우 높음       |

---

### ⚙️ **4️⃣ 사용 사례별 선택 기준**

|               상황               | 권장 프로토콜                     | 이유                     |
| :----------------------------: | :-------------------------- | :--------------------- |
|   **실시간 게임 서버 (MMORPG, FPS)**  | **Binary**                  | 초당 수천 패킷 송수신, 성능/크기 중요 |
|       **웹 서비스, REST API**      | **JSON**                    | 플랫폼 다양, 가독성과 호환성 중요    |
| **서버 간 통신 (Microservice RPC)** | **Binary (gRPC, Protobuf)** | 성능+Schema 기반 안전성       |
|        **로그/통계 데이터 전송**        | **JSON / NDJSON**           | 가독성·분석 용이성             |
|      **모바일/WebSocket 채팅**      | **JSON (or MsgPack)**       | 클라이언트 플랫폼 다양성 확보       |

---

### ⚙️ **5️⃣ 하이브리드 접근도 가능**

* **Binary + JSON 헤더 혼합**

  * 헤더는 JSON, 본문은 Binary 압축
  * 초기 개발/디버깅 시 JSON 사용 → 배포 시 Binary 전환
* **MessagePack / FlatBuffers**

  * Binary 효율 + JSON 유연성의 중간 지점

---

### ⚙️ **6️⃣ 게임 서버 개발 관점 요약**

|      구분      | Binary 프로토콜             | JSON 프로토콜           |
| :----------: | :---------------------- | :------------------ |
|   **패킷 구조**  | 고정 길이(Header + Payload) | Key-Value 기반        |
|   **처리 성능**  | 매우 빠름                   | 상대적으로 느림            |
| **패킷 검사/로깅** | 별도 툴 필요                 | 로그로 바로 확인 가능        |
|    **확장성**   | 구조체 변경 시 재배포 필요         | Key 추가로 간단 확장       |
|   **적합 예시**  | IOCP 서버, UDP 통신         | WebSocket, HTTP API |

---

## 🎯 **면접용 요약**

> **Binary 프로토콜**은 빠르고 효율적이며 게임·실시간 서버에 적합하고,
> **JSON 프로토콜**은 가독성과 호환성이 뛰어나 웹·모바일 환경에 적합합니다.
>
> 선택 기준은 **성능 우선이면 Binary**, **유연성과 호환성 우선이면 JSON**입니다.

---

### 🔥 꼬리질문 예상

1. **게임 서버에서 JSON 대신 Binary를 쓰는 이유는?**
   → 문자열 파싱 비용이 크고, 전송량이 많아 **지연(latency)** 발생 가능.

2. **Binary의 단점은 어떻게 보완하나요?**
   → 버전 관리용 헤더 추가, Schema 기반 프로토콜(Protobuf/FlatBuffers) 사용.

3. **Binary와 JSON을 혼합해서 쓸 수 있나요?**
   → 가능합니다. 초기엔 JSON으로 개발 후, 성능 병목 구간만 Binary 전환.

</div>
</details>


## 25. 패킷 단위로 데이터를 주고받을 때, TCP에서 ‘패킷 경계’가 왜 사라지나요?

<details close>
<summary><strong>💡 자세한 정답</strong></summary>

<div style="background-color:#f6f8fa; padding:15px; border-radius:10px;">

## 💡 **TCP에서 패킷 경계가 사라지는 이유**

> TCP는 **스트림(바이트 흐름, Byte Stream) 기반 프로토콜**이기 때문에
> **송신한 패킷 단위(Message 경계)** 를 그대로 보존하지 않습니다.
>
> 즉, 수신 측에서는 “패킷이 어디서 시작되고 끝나는지” 알 수 없으며,
> 애플리케이션이 직접 **헤더 기반 파싱(Length, ID 등)** 을 통해 경계를 복원해야 합니다.

---

### ⚙️ **1️⃣ 핵심 원리 요약**

|      구분      | UDP                      | TCP                      |
| :----------: | :----------------------- | :----------------------- |
|   **전송 단위**  | Datagram (패킷 단위)         | Stream (연속된 바이트 흐름)      |
| **패킷 경계 유지** | ✅ 유지됨                    | ❌ 사라짐                    |
|   **수신 방식**  | 1 `recv()` = 1 패킷        | 여러 패킷이 섞이거나 분리될 수 있음     |
|    **예시**    | `recv()`로 받은 데이터 = 송신 패킷 | `recv()`로 받은 데이터 ≠ 송신 단위 |

---

### ⚙️ **2️⃣ 왜 사라지는가? (TCP의 스트림 특성)**

TCP는 전송 중 다음과 같은 처리를 수행합니다.

|              단계             | 설명                                |
| :-------------------------: | :-------------------------------- |
| **1. 송신 시 패킷 합침 (Nagle 등)** | 작은 메시지를 묶어서 보냄 → 여러 송신이 하나로 합쳐짐   |
|      **2. 수신 시 분할 가능**      | MTU/혼잡 제어 등에 의해 한 메시지가 여러 조각으로 도착 |
|      **3. 커널 버퍼 단위 관리**     | OS 레벨에서 단순히 “데이터 흐름”으로만 관리        |
|         **4. 결과적으로**        | 수신자는 “언제 어디까지가 한 메시지인지” 모름        |

💡 TCP는 “순서 보장 + 신뢰성 보장”에는 집중하지만,
“메시지 단위(Message Boundary)” 개념은 **제공하지 않음**.

---

### ⚙️ **3️⃣ 예시로 보는 현상**

#### 🔹 송신 코드

```cpp
send(sock, "Hello", 5, 0);
send(sock, "World", 5, 0);
```

#### 🔹 수신 코드

```cpp
char buf[10];
recv(sock, buf, sizeof(buf), 0);
```

|                예상                | 실제 가능성                                 |
| :------------------------------: | :------------------------------------- |
| `"Hello"` → `"World"` 순서로 두 번 수신 | `"HelloWorld"` 한 번에 수신될 수도 있음          |
|                                  | `"He"` / `"lloWorld"` 처럼 잘려서 수신될 수도 있음 |

👉 **TCP는 ‘데이터 스트림’만 보장하고, 메시지 경계는 개발자가 직접 복원해야 함.**

---

### ⚙️ **4️⃣ 해결 방법 (명시적 패킷 구조 정의)**

> 수신 측에서 **명시적 헤더 구조를 통해 경계를 복원**해야 합니다.

#### 예시: 고정된 패킷 헤더 설계

```cpp
struct PacketHeader {
    uint16_t size;  // 전체 패킷 길이
    uint16_t id;    // 메시지 타입
};
```

#### 수신 파싱 로직

```cpp
void ProcessStream(Session* s)
{
    while (s->recvBuffer.GetSize() >= sizeof(PacketHeader))
    {
        PacketHeader header;
        s->recvBuffer.Peek(&header, sizeof(header));

        if (s->recvBuffer.GetSize() < header.size)
            break; // 아직 패킷 전체 도착 X

        s->recvBuffer.Pop(&header, header.size);
        HandlePacket(s, header.id, header);
    }
}
```

✅ **핵심 포인트**

* `recv()`가 패킷 단위 보장을 하지 않으므로
* 애플리케이션 레벨에서 **버퍼링 + Length 기반 파싱** 필요.

---

### ⚙️ **5️⃣ 반대로 UDP의 경우**

* UDP는 **Datagram 기반**이라 `recvfrom()` 한 번이 **하나의 메시지**.
* 패킷 경계가 그대로 유지되며, 수신자가 직접 버퍼링할 필요 없음.
* 하지만 신뢰성(순서, 재전송)은 TCP가 더 우수.

---

## 🎯 **면접용 요약**

> TCP는 **스트림 기반 프로토콜**이라 송신한 패킷의 경계가 보존되지 않습니다.
>
> 따라서 수신 측에서는 **패킷 길이(Length) 정보가 포함된 헤더 구조**를 설계하고,
> 이를 기반으로 **직접 버퍼에서 패킷 단위로 파싱**해야 합니다.

---

### 🔥 꼬리질문 예상

1. **패킷 경계를 복원하는 일반적인 방법은?**
   → 헤더에 `Length` 필드 포함 후, 수신 버퍼에서 길이만큼 잘라 처리.

2. **Nagle 알고리즘이 경계에 영향을 주나요?**
   → 예, 작은 패킷을 병합하여 전송함으로써 경계 혼합 가능성 증가.

3. **UDP에서는 이런 문제가 없는 이유는요?**
   → UDP는 Datagram 기반이므로 OS가 패킷 단위를 그대로 전달함.

</div>
</details>

## 26. 패킷 단위 처리를 위해 서버에서는 어떤 전략을 사용하나요?

<details close>
<summary><strong>💡 자세한 정답</strong></summary>

<div style="background-color:#f6f8fa; padding:15px; border-radius:10px;">

## 💡 **패킷 단위 처리를 위한 서버 측 전략**

> TCP는 **스트림(바이트 흐름) 기반 프로토콜**이므로,
> 수신 시점에 **패킷 경계(boundary)** 가 사라집니다.
>
> 따라서 서버는 **명확한 패킷 구분 규칙**을 직접 정의해야 하며,
> 보통 **헤더 기반 Length-Prefix 구조**, **고정 길이 방식**,
> 또는 **Delimiter(구분자)** 방식을 사용합니다.

---

### ⚙️ **1️⃣ 주요 방식 비교**

|           방식           | 설명                      | 장점         | 단점            |
| :--------------------: | :---------------------- | :--------- | :------------ |
| **① Length-Prefix 방식** | 헤더에 전체 패킷 길이(`size`) 명시 | 유연성 높고 효율적 | 헤더 파싱 필요      |
|  **② Fixed-Length 방식** | 모든 패킷을 동일 크기로 정의        | 단순, 빠름     | 공간 낭비, 확장 어려움 |
|   **③ Delimiter 방식**   | `\n`, `\0` 등 특수 문자로 구분  | 텍스트 기반에 적합 | 바이너리엔 부적합     |

---

### ⚙️ **2️⃣ Length-Prefix 구조 (표준 방식)**

> 가장 일반적이며, 실무 서버(특히 IOCP/epoll 기반)에서 표준으로 사용됩니다.

#### 📦 구조 예시

```
[Header: 4 bytes][Body: N bytes]
 └── size (전체 길이)
```

#### 📜 구조 정의

```cpp
#pragma pack(push, 1)
struct PacketHeader {
    uint16_t size; // 전체 길이(Header + Body)
    uint16_t id;   // 메시지 ID
};
#pragma pack(pop)
```

#### 📥 파싱 로직

```cpp
void ProcessRecvBuffer(Session* s) {
    while (true) {
        if (s->recvBuf.Size() < sizeof(PacketHeader))
            break; // 헤더 미도착

        PacketHeader header;
        s->recvBuf.Peek(&header, sizeof(header));

        if (s->recvBuf.Size() < header.size)
            break; // 패킷 전체 미도착

        std::vector<char> packet(header.size);
        s->recvBuf.Read(packet.data(), header.size);

        HandlePacket(s, header.id, packet);
    }
}
```

✅ **장점**

* 다양한 크기의 패킷 처리 가능
* 불완전한 수신 데이터는 버퍼에 누적 후 재조립 가능
* **게임 서버, 채팅 서버 등 고성능 TCP 서버에서 기본 구조**

---

### ⚙️ **3️⃣ Fixed-Length 구조 (고정 크기)**

> 모든 패킷을 동일 크기로 설계하여 경계 파싱 불필요.

```cpp
struct FixedPacket {
    int id;
    char data[64];
};
recv(sock, &packet, sizeof(FixedPacket), 0);
```

✅ 단순하고 빠름
❌ 패킷마다 데이터 크기가 다를 경우 낭비 심함 → 실무에선 거의 사용하지 않음.

---

### ⚙️ **4️⃣ Delimiter(구분자) 기반 방식**

> 텍스트 프로토콜(HTTP, Redis, MQTT 등)에서 사용.
> 특정 문자(`\n`, `\r\n`, `\0`)가 오면 한 패킷으로 간주.

```cpp
recvBuf.append(data);
size_t pos = recvBuf.find('\n');
if (pos != std::string::npos) {
    std::string msg = recvBuf.substr(0, pos);
    recvBuf.erase(0, pos + 1);
    HandleMessage(msg);
}
```

✅ 단순 구현, 사람이 읽기 쉬움
❌ 바이너리 전송엔 부적합, 성능 낮음.

---

### ⚙️ **5️⃣ 실무에서의 표준 조합**

|         구조        | 적용 사례                      |
| :---------------: | :------------------------- |
| **Length-Prefix** | 대부분의 게임 서버, 채팅 서버, DB 프로토콜 |
|  **Fixed-Length** | 임베디드 장비, 단순 센서 통신          |
|  **Delimiter 기반** | 텍스트 명령 프로토콜, HTTP, Redis 등 |

---

## 🎯 **면접용 요약**

> TCP는 스트림 기반이라 패킷 경계가 사라지므로,
> 서버는 보통 **헤더 기반 Length-Prefix 구조**로 패킷을 구분합니다.
>
> 상황에 따라 **고정 길이 방식**(단순 구조) 또는
> **구분자 기반 방식**(텍스트 프로토콜용)을 사용할 수도 있습니다.

---

### 🔥 꼬리질문 예상

1. **Length-Prefix 방식에서 부분 수신 시 어떻게 처리하나요?**
   → 수신 버퍼에 누적 후, `Header + Body` 전체 도착 시 처리.

2. **Delimiter 방식은 언제 쓰나요?**
   → 사람이 읽는 명령형 프로토콜(예: HTTP, 채팅 명령)에서 사용.

3. **게임 서버에서 Length-Prefix를 쓰는 이유는요?**
   → 데이터 크기가 가변적이고, 빠른 파싱·정확한 경계 구분이 필요하기 때문.

</div>
</details>

## 27. TCP에서 “헤더 + 바디” 구조로 데이터를 보내는 이유는 무엇인가요?

<details close>
<summary><strong>💡 자세한 정답</strong></summary>

<div style="background-color:#f6f8fa; padding:15px; border-radius:10px;">

## 💡 **TCP에서 헤더 + 바디 구조를 사용하는 이유**

> TCP는 **스트림 기반 프로토콜**이라 데이터의 “패킷 경계”가 사라집니다.
> 따라서 서버와 클라이언트가 데이터를 주고받을 때는
> **헤더(Header)** 에 메타정보를 포함하여 **명확한 메시지 경계와 구조를 유지**해야 합니다.
>
> 이를 위해 대부분의 프로토콜은 **“헤더 + 바디(Header + Body)” 구조**를 사용합니다.

---

### ⚙️ **1️⃣ 핵심 목적**

|         목적         | 설명                                                                 |
| :----------------: | :----------------------------------------------------------------- |
|  **① 메시지 경계 명확화**  | TCP 스트림은 연속된 바이트 흐름이라 “한 메시지의 시작과 끝”을 알 수 없음 → 헤더의 `length` 필드로 구분 |
|   **② 파싱 용이성 확보**  | 수신 측이 헤더만 보고 “얼마만큼의 데이터를 더 읽어야 하는지” 판단 가능                          |
| **③ 변동 길이 데이터 처리** | 가변 크기의 문자열, JSON, 구조체 등을 유연하게 전송 가능                                |
|   **④ 프로토콜 확장성**   | ID, Version, Flag, Checksum 등 다양한 정보를 추가 가능                        |

---

### ⚙️ **2️⃣ 기본 구조 예시**

```
┌──────────┬────────────────────────────┐
│  Header  │            Body            │
│ (고정길이)│       (가변 길이, 실제 데이터) │
└──────────┴────────────────────────────┘
```

#### 📜 구조 예시 (C++)

```cpp
#pragma pack(push, 1)
struct PacketHeader {
    uint16_t size;  // 전체 패킷 크기 (Header + Body)
    uint16_t id;    // 패킷 식별자 (예: LOGIN, MOVE 등)
};
#pragma pack(pop)
```

#### 📦 예시 전송 구조

| 필드   | 크기 | 설명                    |
| :--- | :- | :-------------------- |
| size | 2B | 전체 패킷 길이              |
| id   | 2B | 메시지 타입                |
| body | N  | 실제 데이터 (예: 닉네임, 좌표 등) |

---

### ⚙️ **3️⃣ 수신 측 파싱 절차**

```cpp
void ProcessRecv(Session* s)
{
    while (true)
    {
        // (1) 헤더 도착 확인
        if (s->recvBuf.Size() < sizeof(PacketHeader))
            break;

        PacketHeader header;
        s->recvBuf.Peek(&header, sizeof(header));

        // (2) 전체 패킷 도착 확인
        if (s->recvBuf.Size() < header.size)
            break;

        // (3) 완전한 패킷 추출
        std::vector<char> packet(header.size);
        s->recvBuf.Read(packet.data(), header.size);

        HandlePacket(s, header.id, packet);
    }
}
```

✅ 헤더의 `size`를 기준으로 **“메시지 단위 복원”** 가능.

---

### ⚙️ **4️⃣ 헤더의 추가 활용 예시**

|      필드      | 용도                                |
| :----------: | :-------------------------------- |
|   **size**   | 전체 길이 (패킷 경계 파악용)                 |
|    **id**    | 메시지 종류 식별 (ex. LOGIN, MOVE, CHAT) |
|  **version** | 프로토콜 버전 관리                        |
|   **flag**   | 암호화, 압축 여부 표시                     |
| **checksum** | 데이터 무결성 검증                        |

> 💡 “헤더 = 데이터의 설명서”, “바디 = 실제 데이터” 역할.

---

### ⚙️ **5️⃣ “헤더 + 바디” 구조의 장점 요약**

|       항목      | 장점                       |
| :-----------: | :----------------------- |
| **패킷 경계 명확화** | TCP 스트림의 단점 해결           |
| **파싱 효율성 향상** | 부분 수신 시에도 헤더 정보만으로 판단 가능 |
|  **유연한 확장성**  | 새로운 패킷 타입 추가 용이          |
|  **메모리 안전성**  | 크기 정보를 명시함으로써 오버리드 방지    |
|  **디버깅 편의성**  | 구조 명확 → 로깅 및 트래픽 분석 용이   |

---

### ⚙️ **6️⃣ 실무 적용 예시 (게임 서버)**

|      구성 요소     | 역할                       |
| :------------: | :----------------------- |
|   **Header**   | 패킷 크기(size) + 명령 ID(cmd) |
|    **Body**    | 실제 데이터 (좌표, 캐릭터 상태 등)    |
| **SendBuffer** | 헤더와 바디를 합쳐 전송            |
| **RecvBuffer** | 헤더 기반으로 스트림에서 메시지 복원     |

---

## 🎯 **면접용 요약**

> TCP는 스트림 기반이라 패킷 경계가 존재하지 않기 때문에,
> 서버는 **“헤더 + 바디 구조”**를 사용해 **메시지 경계를 명확히 구분**하고
> **파싱을 단순화**하며, **가변 길이 데이터와 확장성**을 확보합니다.
>
> 헤더는 데이터의 “메타 정보”, 바디는 “실제 내용” 역할을 수행합니다.

---

### 🔥 꼬리질문 예상

1. **헤더의 크기를 고정하는 이유는요?**
   → 빠른 파싱을 위해. 고정된 크기면 Peek만으로 헤더 정보 확인 가능.

2. **헤더 없이 보내면 어떻게 되나요?**
   → 수신 측이 경계를 알 수 없어, 여러 패킷이 섞이거나 잘려서 해석됨.

3. **UDP에서도 헤더+바디 구조가 필요한가요?**
   → UDP는 메시지 단위 전송이라 필수는 아니지만, 데이터 구조 구분엔 여전히 유용.

</div>
</details>


## 28. 패킷 재전송은 TCP 내부적으로 어떻게 처리되나요?

<details close>
<summary><strong>💡 자세한 정답</strong></summary>

<div style="background-color:#f6f8fa; padding:15px; border-radius:10px;">

## 💡 **TCP의 패킷 재전송 메커니즘 (Retransmission Mechanism)**

> TCP는 **신뢰성 있는 전송(Guaranteed Delivery)** 을 보장하기 위해
> 손실된 패킷을 자동으로 감지하고 **재전송(Retransmission)** 합니다.
>
> 재전송은 **ACK 타임아웃(RTO 기반)** 또는 **중복 ACK 감지(Fast Retransmit)** 에 의해 발생합니다.

---

### ⚙️ **1️⃣ TCP 신뢰성 보장을 위한 기본 원리**

|               기능              | 설명                                     |
| :---------------------------: | :------------------------------------- |
| **순서 보장 (In-order Delivery)** | 시퀀스 번호(Sequence Number) 기반으로 데이터 순서 유지 |
|   **손실 검출 (Loss Detection)**  | ACK 응답 또는 타임아웃 기반으로 패킷 손실 감지           |
|    **재전송 (Retransmission)**   | 손실된 구간의 세그먼트를 다시 전송                    |
|          **혼잡 제어 연계**         | 재전송 발생 시 혼잡 윈도우(cwnd) 축소로 네트워크 안정화     |

---

### ⚙️ **2️⃣ 패킷 재전송이 발생하는 두 가지 핵심 트리거**

#### 🔹 (1) **ACK 타임아웃 (RTO: Retransmission Timeout)**

> 특정 패킷에 대한 ACK이 일정 시간 안에 도착하지 않으면,
> TCP는 **타임아웃이 발생했다고 판단**하고 해당 패킷을 **재전송**합니다.

```text
[송신 측] —──> [패킷 #10 전송]
(대기 중… ACK 미수신)
→ 일정 시간 경과 (RTO 만료)
→ 패킷 #10 재전송
```

✅ 특징

* **가장 기본적인 재전송 메커니즘**
* 네트워크 혼잡 또는 손실 시 동작
* 타임아웃 값은 **RTT(왕복 시간)** 을 기반으로 동적 계산

#### 🔹 RTO 계산식 (RFC 6298)

```text
RTO = SRTT + max(G, K * RTTVAR)
```

* `SRTT`: 최근 RTT(평균 왕복 지연)
* `RTTVAR`: RTT 변동폭(분산)
* `K`: 보정 계수 (일반적으로 4)
* `G`: 최소 타이머 간격

---

#### 🔹 (2) **중복 ACK 기반 빠른 재전송 (Fast Retransmit)**

> 수신 측이 **중복된 ACK을 3회 연속으로 보낼 경우**,
> 송신 측은 “해당 구간의 패킷이 손실되었다”고 판단하고 즉시 재전송합니다.

```text
패킷 순서: 1, 2, 3, 4
#3 손실 발생

수신 측:
ACK(3) → ACK(3) → ACK(3) → ACK(3)
↑
3회 중복 ACK 도착 → 송신 측은 #3을 즉시 재전송
```

✅ 특징

* **타임아웃 전에 빠른 복구 가능 (지연 최소화)**
* **3번 이상 중복 ACK** → 손실 추정
* 이후 **Fast Recovery 단계** 진입 (혼잡 윈도우 일시 축소)

---

### ⚙️ **3️⃣ TCP 재전송 동작 흐름 요약**

```
[패킷 전송]
   ↓
[ACK 수신 대기]
   ↓
(1) ACK 수신 → 다음 패킷 전송
(2) 중복 ACK 3회 → 빠른 재전송
(3) ACK 미수신, RTO 만료 → 타임아웃 재전송
```

---

### ⚙️ **4️⃣ 재전송 이후 동작**

|           상황           | 후속 동작                                       |
| :--------------------: | :------------------------------------------ |
| **Fast Retransmit 발생** | 혼잡 윈도우(cwnd) 절반으로 감소 → **Fast Recovery** 진입 |
|     **RTO 기반 재전송**     | 윈도우 초기화(Slow Start 재시작)                     |
|      **ACK 정상 회복**     | cwnd 점진적 증가 (Congestion Avoidance)          |

> 즉, 재전송은 **단순 재전달이 아니라**,
> **혼잡 제어 알고리즘(Tahoe, Reno, NewReno 등)** 과 함께 동작하여
> 네트워크 안정성을 보장합니다.

---

### ⚙️ **5️⃣ 예시 시퀀스 (Fast Retransmit)**

```
송신: [1][2][3][4][5]
수신: [1][2][X][4][5]   ← 패킷 #3 손실
ACK:  ACK(3), ACK(3), ACK(3)
→ 송신 측: “패킷 #3 손실” 판단 → #3 재전송
→ 이후 ACK(6) 수신 → 정상 복구
```

---

## 🎯 **면접용 요약**

> TCP는 신뢰성 전송을 위해 **패킷 손실 시 자동 재전송 메커니즘**을 갖습니다.
>
> **ACK 타임아웃(RTO)** 으로 ACK이 일정 시간 내 도착하지 않으면 재전송하고,
> **중복 ACK이 3회 이상 수신되면 빠른 재전송(Fast Retransmit)** 을 수행합니다.
>
> 이 과정은 **혼잡 제어 알고리즘**과 연계되어
> 네트워크 안정성과 성능을 동시에 보장합니다.

---

### 🔥 꼬리질문 예상

1. **Fast Retransmit과 Timeout 재전송의 차이는요?**
   → Fast Retransmit은 중복 ACK 기반으로 즉시 재전송, Timeout은 일정 시간(RTO) 경과 후 재전송.

2. **RTO 값은 고정인가요?**
   → 아닙니다. RTT(왕복 지연) 평균과 변동폭에 따라 동적으로 조정됩니다.

3. **UDP는 왜 재전송을 하지 않나요?**
   → UDP는 비연결형이며, 신뢰성 대신 **지연 최소화**를 우선하기 때문입니다.

</div>
</details>


## 29. TCP에서 순서가 뒤섞인 패킷은 어떻게 복원되나요?

<details close>
<summary><strong>💡 자세한 정답</strong></summary>

<div style="background-color:#f6f8fa; padding:15px; border-radius:10px;">

## 💡 **TCP의 패킷 순서 복원 메커니즘**

> TCP는 **신뢰적 전송(Reliable Transmission)** 을 보장하기 위해
> 수신 순서가 뒤섞이거나 손실된 패킷을 **시퀀스 번호(Sequence Number)** 기반으로
> 자동으로 **재조립(Reassembly)** 합니다.
>
> 즉, “도착 순서가 아닌, 송신 순서대로” 데이터를 상위 계층에 전달합니다.

---

### ⚙️ **1️⃣ TCP의 시퀀스 번호 개념**

|                항목               | 설명                          |
| :-----------------------------: | :-------------------------- |
|    **Sequence Number (SEQ)**    | 송신자가 각 바이트에 부여하는 고유 번호      |
| **Acknowledgment Number (ACK)** | 수신자가 “다음으로 기대하는 바이트 번호”     |
|              **역할**             | 손실·중복·순서 뒤섞임 발생 시 재정렬 근거 제공 |

💡 즉, “패킷 단위”가 아니라 “바이트 단위로 순서를 추적”합니다.

---

### ⚙️ **2️⃣ 순서 뒤섞임(Out-of-Order) 발생 예시**

```
송신 측: [1001~1500][1501~2000][2001~2500]
수신 측: [1501~2000] → [1001~1500] → [2001~2500]
```

1. 두 번째 패킷이 먼저 도착함 (순서 뒤섞임)
2. 수신 측은 아직 1001~1500을 받지 못했기 때문에
   **ACK(1001)** 을 계속 보냄 → “난 아직 1001부터 기다리고 있다”는 의미
3. 송신 측은 중복 ACK을 감지하여 필요 시 **빠른 재전송(Fast Retransmit)** 수행
4. 누락된 세그먼트가 도착하면
   → 커널이 내부 **Reassembly Buffer** 에서 정렬 후 상위 계층으로 전달

---

### ⚙️ **3️⃣ 수신 측 내부 동작 (Reassembly Buffer)**

|      단계      | 설명                                |
| :----------: | :-------------------------------- |
|   **① 수신**   | 패킷 도착 시 SEQ 번호 확인                 |
|  **② 버퍼 저장** | 도착 순서에 상관없이 Reassembly Buffer에 저장 |
|  **③ 누락 탐지** | 연속되지 않은 시퀀스 발견 시 중복 ACK 전송        |
|  **④ 누락 복원** | 누락된 세그먼트 도착 시 정렬 후 상위 계층 전달       |
| **⑤ ACK 갱신** | 마지막으로 연속된 SEQ + 1을 ACK로 전송        |

---

### ⚙️ **4️⃣ 순서 복원 예시 시퀀스**

```
송신: [SEQ=1000 LEN=500], [SEQ=1500 LEN=500], [SEQ=2000 LEN=500]

도착 순서:
#2 → #1 → #3

수신 버퍼 상태:
[1500~2000] 저장 → ACK(1000) (누락 감지)
[1000~1500] 도착 → 재조립 완료 → 상위 계층 전달
[2000~2500] 이어서 도착 → ACK(2500)
```

결과적으로:

* **수신 애플리케이션은 항상 올바른 순서로 데이터 수신**
* **TCP 내부 버퍼에서 순서 정렬과 누락 복원 자동 수행**

---

### ⚙️ **5️⃣ 재조립 완료 후 ACK 처리**

* TCP는 “마지막으로 연속된 시퀀스의 끝”을 ACK 번호로 보냅니다.
  → 즉, `ACK = 마지막으로 연속적으로 받은 바이트 + 1`

예시:

```
수신한 바이트: 0~999, 1000~1999, (2000~2999 누락)
→ ACK(2000)

2000~2999 도착 후
→ ACK(3000)
```

---

### ⚙️ **6️⃣ 순서 뒤섞임 처리 요약**

|  단계 | 동작                    |
| :-: | :-------------------- |
| 1️⃣ | 도착한 세그먼트의 SEQ 확인      |
| 2️⃣ | Reassembly Buffer에 저장 |
| 3️⃣ | 누락된 범위 감지 시 중복 ACK 전송 |
| 4️⃣ | 누락 세그먼트 도착 시 버퍼에서 재조립 |
| 5️⃣ | 연속 구간 완성 시 상위 계층으로 전달 |

---

## 🎯 **면접용 요약**

> TCP는 **시퀀스 번호 기반 재조립(Reassembly)** 을 통해
> 순서가 뒤섞인 패킷을 **정확한 순서로 복원**합니다.
>
> 수신 측은 패킷을 **Reassembly Buffer** 에 저장한 뒤
> **ACK 번호를 통해 누락 여부를 송신자에게 알리고**,
> 누락 패킷이 도착하면 **자동으로 정렬 후 상위 계층으로 전달**합니다.

---

### 🔥 꼬리질문 예상

1. **중복 ACK이란 무엇이며 왜 필요한가요?**
   → 순서가 어긋났을 때 송신자에게 “이전 패킷이 아직 도착 안 함”을 알리기 위한 통지.

2. **Reassembly Buffer는 어디에 존재하나요?**
   → TCP 스택(커널 공간) 내부 버퍼에 존재, 애플리케이션이 아닌 OS가 관리.

3. **UDP에서는 이런 복원이 가능한가요?**
   → 불가능. UDP는 순서 보장·재전송·재조립 기능이 없음.

</div>
</details>


## 30. 서버에서 클라이언트가 비정상 종료(전원 끔 등)된 경우 어떻게 감지하나요?

<details close>
<summary><strong>💡 자세한 정답</strong></summary>

<div style="background-color:#f6f8fa; padding:15px; border-radius:10px;">

## 💡 **TCP 연결 종료 감지 방식 (비정상 종료 탐지)**

> 클라이언트가 **정상적으로 종료하지 않고**
> 갑작스럽게 종료(전원 차단, 네트워크 끊김 등)되는 경우,
> 서버는 즉시 알 수 없습니다.
>
> TCP는 **상태 기반 스트림 프로토콜**이기 때문에,
> 연결이 끊겼는지 확인하려면 **데이터 송수신이나 Keep-Alive** 를 통해 감지해야 합니다.

---

### ⚙️ **1️⃣ 감지 방법 요약**

|               감지 방식               | 설명                           | 발생 시점        |
| :-------------------------------: | :--------------------------- | :----------- |
|        **① recv() 반환값 = 0**       | 상대가 **정상적으로 연결 종료 (FIN 수신)** | 정상 종료        |
|   **② 소켓 오류 (WSAECONNRESET 등)**   | 상대가 **비정상 종료 (RST 수신)**      | 즉시 감지 가능     |
|        **③ Keep-Alive 타이머**       | 장시간 무응답 시 커널이 주기적으로 확인       | 수 초~수 분 지연   |
| **④ 응용 계층 Heartbeat (Ping/Pong)** | 일정 주기로 직접 신호 송수신             | 가장 실시간 감지 가능 |

---

### ⚙️ **2️⃣ 정상 종료 (Graceful Close)**

> 클라이언트가 `close()` 호출 → TCP 4-way handshake 수행
> 서버는 다음과 같이 감지합니다.

```cpp
int ret = recv(sock, buf, sizeof(buf), 0);
if (ret == 0)
{
    // 정상 종료 (FIN 수신)
    DisconnectClient(sock);
}
```

✅ `recv()`가 **0**을 반환하면 FIN 패킷이 도착했다는 뜻.

---

### ⚙️ **3️⃣ 비정상 종료 (Abrupt Termination)**

> 클라이언트가 전원을 끄거나 네트워크가 끊긴 경우
> FIN 없이 **RST(Reset) 패킷** 또는 **무응답 상태**가 발생.

#### 🔹 감지 흐름

|        상황        | 서버 동작                   | 결과                 |
| :--------------: | :---------------------- | :----------------- |
| **전원 끔 / 강제 종료** | 연결이 갑자기 끊김 → OS가 RST 전송 | `WSAECONNRESET` 발생 |
|  **네트워크 케이블 제거** | TCP는 재시도 반복             | 일정 시간 후 타임아웃       |
|   **프로세스 크래시**   | 커널이 즉시 RST 전송           | 소켓 오류 반환           |

#### 🔹 코드 예시

```cpp
int ret = recv(sock, buf, sizeof(buf), 0);
if (ret == SOCKET_ERROR)
{
    int err = WSAGetLastError();
    if (err == WSAECONNRESET || err == WSAECONNABORTED)
        DisconnectClient(sock); // 비정상 종료 감지
}
```

---

### ⚙️ **4️⃣ Keep-Alive 옵션 활용**

> TCP Keep-Alive는 **커널 수준에서 연결 유효성**을 주기적으로 확인합니다.
> 오랜 시간 데이터를 주고받지 않는 세션의 상태를 검사할 때 유용합니다.

#### 🔹 설정 방법

```cpp
BOOL opt = TRUE;
setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, (char*)&opt, sizeof(opt));
```

#### 🔹 윈도우 기본 설정 (조정 가능)

|           항목          | 기본값 | 설명            |
| :-------------------: | :-- | :------------ |
|    `KeepAliveTime`    | 2시간 | 비활성 상태 지속 시간  |
|  `KeepAliveInterval`  | 1초  | 재시도 간격        |
| `KeepAliveRetryCount` | 5회  | 실패 후 연결 종료 판단 |

💡 즉, 약 **2시간 이상 무통신 시 커널이 탐지**
(실시간 반응이 필요할 경우, **응용 계층 Heartbeat** 로 보완)

---

### ⚙️ **5️⃣ Heartbeat (응용 계층 수준 확인)**

> 서버와 클라이언트가 일정 주기로 **Ping/Pong 메시지** 교환.

#### 🔹 예시

```cpp
// 서버 → 클라이언트
Send(PING);

// 일정 시간 내 PONG 미수신 시
DisconnectClient(session);
```

✅ 실시간 감지 가능 (수 초 단위)
✅ 게임 서버, 채팅 서버 등에서 **표준 방식**

---

### ⚙️ **6️⃣ 요약 비교**

|         구분        | 정상 종료           | 비정상 종료          |
| :---------------: | :-------------- | :-------------- |
|    **recv()=0**   | FIN 수신          | X               |
| **WSAECONNRESET** | X               | RST 수신 (즉시 감지)  |
|   **KeepAlive**   | 사용 시 일정 시간 후 감지 | 사용 시 일정 시간 후 감지 |
|   **Heartbeat**   | -               | 실시간 감지 가능       |

---

## 🎯 **면접용 요약**

> TCP는 비정상 종료 시 자동으로 즉시 감지되지 않기 때문에,
> 서버는 다음 3가지를 통해 연결 상태를 확인합니다.
>
> 1️⃣ **recv()=0** → 정상 종료(FIN 수신)
> 2️⃣ **소켓 오류(WSAECONNRESET)** → 비정상 종료(RST 수신)
> 3️⃣ **KeepAlive 또는 Heartbeat** → 장시간 유휴 세션 감시
>
> 실시간 서비스(게임, 채팅 등)에서는 보통 **응용 계층 Heartbeat** 로 보완합니다.

---

### 🔥 꼬리질문 예상

1. **KeepAlive와 Heartbeat의 차이는요?**
   → KeepAlive는 커널 수준(수 분~시간 단위), Heartbeat은 애플리케이션 수준(수 초 단위).

2. **RST 패킷은 언제 발생하나요?**
   → 연결된 소켓이 비정상 종료되거나, 존재하지 않는 포트로 접근 시.

3. **recv()가 블로킹일 때 클라이언트가 끊기면?**
   → 블로킹이 해제되며 `recv()`가 0 또는 오류 코드 반환.

</div>
</details>


## 31. 네트워크 지연(Latency)과 대역폭(Bandwidth)의 차이는 무엇인가요?

<details close>
<summary><strong>💡 자세한 정답</strong></summary>

<div style="background-color:#f6f8fa; padding:15px; border-radius:10px;">

## 💡 **Latency vs Bandwidth**

> 네트워크 성능을 구성하는 두 핵심 요소는
> **지연(Latency)** 과 **대역폭(Bandwidth)** 입니다.
>
> 둘은 자주 혼동되지만,
> **“속도(Speed)”와 “용량(Capacity)”의 관계**로 이해하면 명확합니다.

---

### ⚙️ **1️⃣ 개념 요약**

|      구분      | **Latency (지연)**                      | **Bandwidth (대역폭)**        |
| :----------: | :------------------------------------ | :------------------------- |
|    **의미**    | 데이터가 한 지점에서 다른 지점으로 **이동하는 데 걸리는 시간** | 단위 시간당 **전송 가능한 데이터의 최대량** |
|    **단위**    | 밀리초 (ms)                              | Mbps, MB/s, Gbps           |
|    **유형**    | 왕복 지연(RTT), 단방향 지연 등                  | 업로드/다운로드 대역폭               |
| **주요 영향 요인** | 거리, 라우터 수, 큐잉, 처리 지연                  | 회선 용량, 혼잡도                 |
|    **비유**    | 🚗 도로 위 자동차의 **속도**                   | 🛣️ 도로의 **너비(차선 수)**       |

---

### ⚙️ **2️⃣ 예시로 이해하기**

> **비유:**
> 대역폭은 **도로 폭**,
> 지연은 **자동차 한 대가 목적지에 도착하는 데 걸리는 시간**.

|      시나리오      | 설명                                      |
| :------------: | :-------------------------------------- |
| **Bandwidth↑** | 한 번에 더 많은 차량(데이터)을 보낼 수 있음              |
|  **Latency↓**  | 차량이 더 빠르게 목적지에 도착함                      |
|   **둘 다 중요**   | 대역폭이 넓어도 왕복 시간이 길면 체감 속도 느림 (ex. 해외 서버) |

---

### ⚙️ **3️⃣ 실제 수식 비교**

|       항목      | 계산식                           | 의미           |
| :-----------: | :---------------------------- | :----------- |
|  **Latency**  | 전송 지연 + 전파 지연 + 큐잉 지연 + 처리 지연 | 단일 패킷의 이동 시간 |
| **Bandwidth** | (전송된 데이터량) / (걸린 시간)          | 전송 가능한 최대 용량 |

#### 🔹 예시

```
Bandwidth = 100 Mbps
Latency = 100 ms
→ 1초에 100Mbit 전송 가능하지만, 왕복 시간은 0.1초 소요.
```

---

### ⚙️ **4️⃣ 상호 관계**

|        구분       | 영향                                |
| :-------------: | :-------------------------------- |
| **대역폭↑, 지연 동일** | 더 많은 데이터 동시 전송 가능                 |
| **지연↓, 대역폭 동일** | 응답 체감 속도 개선                       |
|  **지연↑ + 대역폭↑** | 대용량 전송은 가능하지만 반응 느림 (ex. 클라우드 백업) |
|  **지연↓ + 대역폭↓** | 즉각 반응하지만 전송량 적음 (ex. IoT 장비)      |

---

### ⚙️ **5️⃣ 실무 예시**

|          환경         | Latency    | Bandwidth     | 설명              |
| :-----------------: | :--------- | :------------ | :-------------- |
| **로컬 LAN (사무실 내부)** | 1~2 ms     | 수백 Mbps~Gbps  | 거의 즉시 응답        |
|      **국내 인터넷**     | 10~30 ms   | 100~1000 Mbps | 빠르고 안정적         |
| **해외 서버 (미국 ↔ 한국)** | 150~250 ms | 수백 Mbps       | 대역폭 충분하지만 반응 지연 |
|      **위성 통신**      | 600+ ms    | 수십 Mbps       | 극단적인 지연, 저대역폭   |

---

### ⚙️ **6️⃣ 게임 서버 개발 관점**

|       항목      | 영향                              |
| :-----------: | :------------------------------ |
|  **Latency**  | 입력 반응 속도, 캐릭터 움직임, 타격 판정에 직접 영향 |
| **Bandwidth** | 동시 플레이어 수, 상태 동기화 주기 등에 영향      |
|   **최적화 방향**  | 지연 최소화(핑), 필요 최소 패킷 설계로 대역폭 효율화 |

---

## 🎯 **면접용 요약**

> **Latency**는 “데이터가 이동하는 데 걸리는 시간”,
> **Bandwidth**는 “단위 시간당 전송 가능한 데이터 양”입니다.
>
> 즉, Latency는 **속도**, Bandwidth는 **용량** 개념이며,
> 게임·실시간 통신 환경에서는 **지연 최소화**가 체감 성능에 더 큰 영향을 줍니다.

---

### 🔥 꼬리질문 예상

1. **Latency를 줄이려면 어떻게 해야 하나요?**
   → 서버 위치 최적화, RTT 단축, 라우팅 hop 최소화, Nagle 비활성화 등.

2. **Bandwidth는 충분한데 체감 속도가 느린 이유는요?**
   → RTT(지연)와 패킷 손실률이 높을 경우 TCP 재전송으로 인해 실제 처리율 저하.

3. **Ping은 Latency 측정 도구인가요?**
   → 맞습니다. ICMP echo 요청/응답을 통해 왕복 지연(RTT)을 측정합니다.

</div>
</details>


## 32. 패킷 손실(Packet Loss)이 발생했을 때 TCP는 어떻게 반응하나요?

<details close>
<summary><strong>💡 자세한 정답</strong></summary>

<div style="background-color:#f6f8fa; padding:15px; border-radius:10px;">

## 💡 **TCP의 패킷 손실 대응 메커니즘**

> TCP는 **신뢰성 있는 전송(Reliable Transmission)** 을 보장하기 위해
> 패킷 손실이 발생하면 이를 자동으로 감지하고,
> **재전송(Retransmission)** 과 **혼잡 제어(Congestion Control)** 를 통해
> 네트워크를 안정화시킵니다.

---

### ⚙️ **1️⃣ 손실 감지 방식**

TCP는 패킷 손실을 다음 두 가지 방법으로 감지합니다.

|             감지 방식            | 설명                        | 특징                         |
| :--------------------------: | :------------------------ | :------------------------- |
| **① 중복 ACK (Duplicate ACK)** | 수신 측이 같은 ACK을 3회 이상 연속 보냄 | 빠른 손실 감지 (Fast Retransmit) |
|   **② 타임아웃 (RTO Timeout)**   | 특정 패킷의 ACK이 시간 내 도착하지 않음  | 느리지만 확실한 감지 방식             |

---

### ⚙️ **2️⃣ 손실 감지 후 동작 흐름**

#### 🔹 (1) **Fast Retransmit**

> 중복 ACK이 3번 이상 도착 시, 송신 측은 해당 패킷 손실을 즉시 인식하고 **재전송 수행**.

```text
패킷 순서: [1][2][3][4]
#3 손실 발생 → 수신 측은 [ACK=3]을 연속 3회 전송
→ 송신 측: “패킷 #3 손실” 판단 → 즉시 #3 재전송
```

✅ 장점

* 타임아웃 기다리지 않고 빠르게 복구
* 지연(latency) 최소화

---

#### 🔹 (2) **RTO 기반 재전송**

> 일정 시간 동안 ACK이 도착하지 않으면, **타이머 만료(RTO)** 로 판단하고 해당 세그먼트 재전송.

```text
RTO = SRTT + max(G, K × RTTVAR)
```

* `SRTT`: 평균 왕복 시간 (Smoothed RTT)
* `RTTVAR`: RTT 변동폭
* `K`: 보정 계수 (보통 4)
* `G`: 최소 타이머 간격

✅ 특징

* 더 확실하지만 느림
* 혼잡 구간에서 다수 발생 시 전송량 급감

---

### ⚙️ **3️⃣ 손실 이후 혼잡 제어 (Congestion Control)**

> TCP는 패킷 손실을 **네트워크 혼잡의 신호**로 해석하고,
> 전송 속도를 줄여 네트워크 부하를 완화합니다.

#### 🔹 대표 알고리즘 (Reno 기준)

|            단계            | 설명                               |
| :----------------------: | :------------------------------- |
|      **Slow Start**      | 초기에는 작은 윈도우로 시작, ACK마다 윈도우 2배 증가 |
| **Congestion Avoidance** | 혼잡 임계점 이후엔 선형 증가                 |
|    **Fast Retransmit**   | 중복 ACK 3회 수신 시 손실 감지 → 즉시 재전송    |
|     **Fast Recovery**    | 혼잡 윈도우(cwnd)를 절반으로 감소 후 회복 시도    |

#### 💡 예시 흐름

```
1. 패킷 손실 발생 → 중복 ACK 3회
2. Fast Retransmit 실행 (#N 재전송)
3. cwnd = cwnd / 2 (혼잡 윈도우 축소)
4. 네트워크 안정 시 다시 점진적 증가
```

---

### ⚙️ **4️⃣ 시각화 요약**

```
패킷 손실 발생
   ↓
[1] 중복 ACK 3회? → Fast Retransmit
   ↓
[2] 아니면 ACK 미수신 → Timeout Retransmit
   ↓
[3] 혼잡 윈도우 축소 (cwnd ↓)
   ↓
[4] 점진적 증가 (Slow Start / Congestion Avoidance)
```

---

### ⚙️ **5️⃣ 손실 감지 시 실제 반응 예시 (Wireshark)**

|             이벤트             | 표시            | 의미          |
| :-------------------------: | :------------ | :---------- |
|         `Dup ACK #3`        | 중복된 ACK 수신    | 빠른 재전송 트리거  |
|    `Fast Retransmission`    | TCP 재전송 발생    | 손실 복구       |
|     `RTO Retransmission`    | 타임아웃 만료 후 재전송 | 느린 복구       |
| `Congestion Window Reduced` | cwnd 절반 감소    | 혼잡 회피 단계 진입 |

---

## 🎯 **면접용 요약**

> TCP는 패킷 손실을 **ACK 기반으로 감지**하고,
> **재전송(Fast Retransmit or Timeout)** 을 수행하며
> 동시에 **혼잡 윈도우(cwnd)** 를 줄여 네트워크 부하를 완화합니다.
>
> 즉, 단순히 다시 보내는 것이 아니라
> **네트워크 전체 안정성을 유지하는 알고리즘**이 함께 작동합니다.

---

### 🔥 꼬리질문 예상

1. **Fast Retransmit과 Timeout의 차이는요?**
   → Fast Retransmit은 중복 ACK 기반(빠름), Timeout은 ACK 미도착 기반(느림).

2. **TCP가 손실률이 높은 환경에서 느려지는 이유는?**
   → 손실을 혼잡으로 간주하여 cwnd를 계속 줄이기 때문.

3. **UDP에서는 패킷 손실을 어떻게 처리하나요?**
   → 자체적으로 복구하지 않음. 애플리케이션에서 직접 구현해야 함 (예: RTP, QUIC 등).

</div>
</details>


## 33. 게임 서버에서 UDP를 사용하는 이유는 무엇인가요?

<details close>
<summary><strong>💡 자세한 정답</strong></summary>

<div style="background-color:#f6f8fa; padding:15px; border-radius:10px;">

## 💡 **게임 서버에서 UDP를 사용하는 이유**

> 실시간 게임(예: FPS, 액션, MOBA 등)에서는
> **낮은 지연(Low Latency)** 과 **빠른 상태 갱신(Real-time Update)** 이 중요합니다.
>
> 이러한 특성 때문에 **TCP보다 빠르고 단순한 전송 계층인 UDP**가 주로 사용됩니다.

---

### ⚙️ **1️⃣ TCP vs UDP 비교 요약**

|     항목     | **TCP**                  | **UDP**              |
| :--------: | :----------------------- | :------------------- |
|  **연결 방식** | 연결형(Connection-oriented) | 비연결형(Connectionless) |
| **신뢰성 보장** | 재전송, 순서 보장, 손실 복구        | 보장 없음                |
|  **헤더 크기** | 20바이트 이상                 | 8바이트                 |
|  **지연 특성** | 상대적으로 느림 (혼잡 제어, ACK 등)  | 빠름 (즉시 전송)           |
|  **패킷 경계** | 스트림 기반 (경계 사라짐)          | 메시지 단위 보존            |
|  **적합 환경** | 파일 전송, 채팅, HTTP          | 게임, 음성, 실시간 스트리밍     |

---

### ⚙️ **2️⃣ 게임 서버에서 UDP를 선호하는 이유**

#### ✅ **1. 낮은 지연 시간 (Low Latency)**

* TCP는 **ACK, 재전송, 순서 보장** 때문에 지연이 누적됨.
* UDP는 **송신 즉시 전송**, 손실 시에도 대기하지 않음 → 반응 속도 빠름.

> 🎮 실시간 반응이 중요한 게임(예: FPS, 액션)은
> 1ms라도 빠른 응답이 중요 → “빠른 게 이긴다.”

---

#### ✅ **2. 패킷 경계 유지**

* UDP는 한 번의 `sendto()` → 한 번의 `recvfrom()`으로 **패킷 단위 유지**.
* TCP처럼 패킷이 합쳐지거나 분리되지 않음 → **파싱 단순화**.

```cpp
sendto(sock, data, len, 0, ...);
recvfrom(sock, buffer, sizeof(buffer), 0, ...); // 1:1 대응
```

---

#### ✅ **3. 손실 허용이 가능한 데이터 특성**

* 게임의 대부분 데이터는 **“상태 갱신(State Update)”**
  → 이전 프레임의 데이터가 손실되어도 **다음 프레임으로 보완 가능**.

예:

```
[위치 갱신]  → 손실 → 다음 프레임에서 다시 전송 → 자연 복구
```

> 손실 1~2패킷보다 **지연이 더 치명적**.

---

#### ✅ **4. 애플리케이션 수준의 신뢰성 구현 가능**

* UDP는 기본적으로 비신뢰적이지만,
  필요한 경우 개발자가 직접 **부분적 신뢰성**을 구현 가능.

  * 예: `SEQ 번호` + `ACK 비트마스크` 관리
  * 중요 패킷만 재전송 (예: 로그인, 스킬 사용)

> 즉, TCP의 신뢰성은 “과한 오버헤드”
> 게임은 **“필요한 만큼의 신뢰성”만 구현**.

---

### ⚙️ **3️⃣ 예시: TCP 기반의 문제점**

|    상황    | TCP 동작               | 결과                                      |
| :------: | :------------------- | :-------------------------------------- |
| 1개 패킷 손실 | 재전송 대기 (수십~수백 ms)    | **모든 패킷 전송 지연 (Head-of-Line Blocking)** |
| 이동 정보 전송 | 1프레임 지연 시 캐릭터 순간이동   | 플레이 감각 저하                               |
| UDP 사용 시 | 손실된 정보는 무시, 새 위치로 갱신 | 자연스럽고 반응성 유지                            |

---

### ⚙️ **4️⃣ 실제 적용 구조 (예시)**

|      계층     | 구성                    | 역할                |
| :---------: | :-------------------- | :---------------- |
|  **전송 계층**  | UDP                   | 빠른 전송             |
|  **응용 계층**  | Custom Reliable Layer | 패킷 번호, 재전송, 손실 처리 |
| **프로토콜 예시** | ENet, RakNet, QUIC    | UDP 위에서 신뢰성 일부 구현 |

---

### ⚙️ **5️⃣ 실무 예시**

|               장르              | 주요 프로토콜                 | 이유                  |
| :---------------------------: | :---------------------- | :------------------ |
| **FPS (Valorant, Overwatch)** | UDP                     | 1프레임(16ms) 이하 반응 필요 |
|     **MOBA (LoL, Dota2)**     | UDP                     | 실시간 상태 갱신 중심        |
|    **MMORPG (Maple, WoW)**    | TCP (로그인/DB) + UDP (전투) | 혼합 구조 (Hybrid)      |

---

## 🎯 **면접용 요약**

> 게임 서버는 실시간성이 중요하므로
> **빠른 전송, 낮은 지연, 패킷 경계 보존**이 가능한 **UDP**를 사용합니다.
>
> 신뢰성이 필요한 구간은 **응용 계층에서 직접 보완(부분 재전송, SEQ 관리)** 하며,
> **손실보다 지연이 더 치명적인 환경**에 최적화되어 있습니다.

---

### 🔥 꼬리질문 예상

1. **그렇다면 TCP를 쓰는 게임도 있나요?**
   → 있습니다. 턴제·패킷 손실이 치명적인 MMORPG, 채팅/DB 통신 등은 TCP 사용.

2. **UDP에서 신뢰성이 필요한 데이터는 어떻게 처리하나요?**
   → SEQ/ACK 기반의 Custom Reliable Layer 구현 (예: ENet, RakNet).

3. **UDP는 순서 보장이 안 되는데, 순서가 중요한 데이터는요?**
   → SEQ 번호를 붙여 애플리케이션 단에서 직접 정렬합니다.

</div>
</details>


## 34. UDP 기반 신뢰성 보장을 위해 사용할 수 있는 기법은 무엇인가요?

<details close>
<summary><strong>💡 자세한 정답</strong></summary>

<div style="background-color:#f6f8fa; padding:15px; border-radius:10px;">

## 💡 **UDP에서 신뢰성 보장을 위한 기법**

> UDP는 **비연결형, 비신뢰성 프로토콜**로
> 순서 보장·재전송·중복 제어 기능이 없습니다.
>
> 따라서 실시간 게임이나 스트리밍 서버에서는
> 필요한 범위 내에서 **애플리케이션 계층에서 직접 신뢰성을 구현**합니다.

---

### ⚙️ **1️⃣ 신뢰성 보장을 위한 핵심 기법**

|                  기법                  | 설명                      | 역할               |
| :----------------------------------: | :---------------------- | :--------------- |
|    **① 시퀀스 번호 (Sequence Number)**    | 각 패킷에 고유 번호 부여          | 순서 정렬, 중복 수신 감지  |
|   **② ACK / NACK (Acknowledgment)**  | 수신 측이 받은 패킷 번호 회신       | 손실 탐지 및 재전송 트리거  |
| **③ 재전송 타이머 (Retransmission Timer)** | 일정 시간 내 ACK 미수신 시 재전송   | 손실 복구            |
|    **④ 슬라이딩 윈도우 (Sliding Window)**   | 송신 측의 전송 범위 제어          | 흐름 제어 및 효율적 전송   |
|    **⑤ 패킷 중복 제거 (Deduplication)**    | 동일 SEQ 번호 중복 수신 시 무시    | 데이터 무결성 유지       |
| **⑥ 부분 신뢰성 (Selective Reliability)** | 중요 패킷만 보장 (ex. 스킬, 로그인) | 지연 최소화 + 효율적 신뢰성 |
|        **⑦ RTT 측정 및 타임아웃 보정**        | 왕복 시간 기반 타이머 동적 조정      | 네트워크 상황 대응       |

---

### ⚙️ **2️⃣ 시퀀스 번호 (Sequence Number)**

> 모든 패킷에 `seq` 필드를 추가하여 순서 추적.

```cpp
struct UdpPacket {
    uint32_t seq;
    uint16_t size;
    char data[512];
};
```

* 수신 측은 `seq` 기준으로 **정렬**
* 중복 수신(`seq` 동일) 시 폐기
* 손실 구간(`seq` 건너뜀) 감지 → 재요청(ACK/NACK)

---

### ⚙️ **3️⃣ ACK / NACK 시스템**

#### 🔹 ACK (Positive Acknowledgment)

> “이 패킷을 잘 받았다”는 응답 전송.

```text
Client → Server: seq=100
Server → Client: ACK(100)
```

#### 🔹 NACK (Negative Acknowledgment)

> “이 번호 패킷이 누락됐다”는 응답 전송.

```text
수신된 패킷: 1, 2, 4, 5 → NACK(3)
송신 측: #3 재전송
```

✅ 장점: 불필요한 전체 재전송 방지
✅ 단점: 제어 패킷 증가

---

### ⚙️ **4️⃣ 재전송 타이머 (Retransmission Timer)**

> 송신 후 일정 시간 내 ACK이 오지 않으면 재전송.

```cpp
if (Now() - packet.sendTime > timeout)
    Resend(packet);
```

* `timeout`은 **RTT 기반으로 동적 조정**
* 너무 짧으면 불필요한 중복 전송
* 너무 길면 지연 증가

---

### ⚙️ **5️⃣ 슬라이딩 윈도우 (Sliding Window)**

> TCP처럼 “전송 가능한 패킷 범위”를 제한하여
> 흐름 제어 및 혼잡 방지 수행.

```
송신 버퍼: [SEQ 100~120]
ACK(105) 수신 → 윈도우 이동 → [SEQ 106~126]
```

✅ 동시에 여러 패킷 송신 가능
✅ ACK 수신 시 전송 범위 이동 (Window Slide)

---

### ⚙️ **6️⃣ 부분 신뢰성 (Selective Reliability)**

> 모든 패킷을 보장하지 않고,
> **중요한 데이터만 재전송**하도록 설계.

|           구분           | 신뢰 보장 |   재전송 여부   |
| :--------------------: | :---: | :--------: |
| **로그인 / 스킬 사용 / 거래 등** |   O   |     재전송    |
| **이동 정보 / 위치 갱신 / 상태** |   X   | 최신값으로 덮어쓰기 |

✅ 장점: 실시간성 유지
✅ 단점: 일부 데이터는 손실 허용

---

### ⚙️ **7️⃣ 구현 예시 (간략 코드)**

```cpp
void SendReliable(Packet pkt) {
    pkt.seq = nextSeq++;
    sendto(sock, &pkt, sizeof(pkt), 0, ...);
    pending[pkt.seq] = Now();
}

void OnReceive(Packet pkt) {
    if (pkt.seq == expectedSeq) {
        Process(pkt);
        expectedSeq++;
        sendto(sock, &ACK(pkt.seq), sizeof(ACK), 0, ...);
    } else if (pkt.seq < expectedSeq) {
        // 중복 수신
    } else {
        // 손실 감지 (NACK)
    }
}
```

---

### ⚙️ **8️⃣ 실제 사용 예시**

| 라이브러리 / 프로토콜 | 설명                               |
| :----------: | :------------------------------- |
|   **ENet**   | UDP 위에 신뢰성, 순서, 흐름 제어 구현         |
|  **RakNet**  | 게임용 네트워크 엔진 (부분 신뢰성 지원)          |
|   **QUIC**   | UDP 기반 전송 계층 (TLS + 재전송 + 멀티플렉싱) |
|    **KCP**   | 경량 Reliable UDP (RTT 기반 빠른 복구)   |

---

## 🎯 **면접용 요약**

> UDP는 신뢰성을 보장하지 않지만,
> **시퀀스 번호**, **ACK/NACK**, **재전송 타이머**, **슬라이딩 윈도우** 등을 이용해
> **애플리케이션 계층에서 부분적 신뢰성**을 구현할 수 있습니다.
>
> 실시간 게임에서는 **지연 최소화**를 위해
> “모든 패킷이 아닌, 중요한 패킷만 재전송”하는 방식이 일반적입니다.

---

### 🔥 꼬리질문 예상

1. **UDP에서 순서 보장은 어떻게 하나요?**
   → `seq` 번호를 붙이고 수신 버퍼에서 정렬 처리.

2. **재전송 타이머를 RTT 기반으로 조정하는 이유는요?**
   → 네트워크 상태(혼잡/거리)에 따라 최적의 타이밍이 달라지기 때문.

3. **UDP 신뢰성 보장은 TCP보다 항상 빠른가요?**
   → 대부분 빠르지만, 구현 복잡도는 개발자 책임이며 TCP만큼 완벽하지는 않음.

</div>
</details>


## 35. TCP의 슬라이딩 윈도우(Sliding Window)란 무엇인가요?

<details close>
<summary><strong>💡 자세한 정답</strong></summary>

<div style="background-color:#f6f8fa; padding:15px; border-radius:10px;">

## 💡 **TCP의 슬라이딩 윈도우(Sliding Window)**

> **슬라이딩 윈도우(Sliding Window)** 는 TCP에서
> 송신 측이 **ACK(응답)** 을 기다리지 않고 **여러 패킷을 연속 전송할 수 있게 하는 메커니즘**입니다.
>
> 즉, **데이터 흐름 제어(Flow Control)** 와 **전송 효율 향상**을 동시에 달성하기 위한 핵심 구조입니다.

---

### ⚙️ **1️⃣ 기본 개념**

|      구분      | 설명                                          |
| :----------: | :------------------------------------------ |
|    **목적**    | 연속된 데이터 전송을 통해 대역폭 활용 극대화                   |
|   **기반 원리**  | ACK 도착 시점에 따라 전송 가능한 범위를 “창(Window)”으로 관리   |
| **핵심 구성 요소** | 송신 윈도우(Send Window), 수신 윈도우(Receive Window) |

---

### ⚙️ **2️⃣ 윈도우의 구성**

#### 🔹 송신 측 관점

```
┌───────────────────────────────────────────────┐
│  이미 ACK 받은 구간 │ 전송됨(미확인) │ 아직 전송 안 된 구간 │
└───────────────────────────────────────────────┘
                  ↑
              Sliding Window
```

* **윈도우(Window)**: 한 번에 전송 가능한 데이터 범위
* **ACK 수신 시** → 윈도우가 “앞으로 밀려(slide)” 다음 데이터 전송 허용
* **ACK 미수신 시** → 윈도우 정지, 재전송 또는 대기

---

### ⚙️ **3️⃣ 예시 흐름**

#### 초기 상태

|   구간  | 상태                 |
| :---: | :----------------- |
| [1~5] | 전송 가능 (윈도우 크기 = 5) |
| [6~∞] | 전송 대기              |

```
송신: [1][2][3][4][5]
```

#### ACK(1), ACK(2), ACK(3) 수신

→ 윈도우가 앞으로 “슬라이드”

```
송신 가능 구간: [6][7][8]
```

💡 이렇게 윈도우가 “밀리면서(Slide)” 지속적으로 전송 → **슬라이딩 윈도우(Sliding Window)**

---

### ⚙️ **4️⃣ 윈도우 크기(Window Size)**

|     항목    | 설명                            |
| :-------: | :---------------------------- |
|   **정의**  | 수신 측이 한 번에 받아들일 수 있는 데이터 크기   |
|   **단위**  | 바이트(Byte)                     |
| **결정 주체** | 수신 측 (ACK에 window size 포함)    |
|   **역할**  | 송신 측의 전송 속도 제어 (Flow Control) |

> → “이만큼만 보내라”라는 수신 측의 신호
> → 송신 측은 이 범위를 넘기지 않도록 제어

---

### ⚙️ **5️⃣ 윈도우 동작과 ACK 관계**

|     상황    | 설명                       |
| :-------: | :----------------------- |
| **정상 수신** | ACK 수신 → 윈도우 전진 (Slide)  |
| **손실 발생** | ACK 미도착 → 윈도우 정지, 재전송    |
| **혼잡 감지** | 혼잡 윈도우(cwnd) 축소 → 전송량 제한 |
| **회복 단계** | ACK 재개 → 윈도우 점진적 확장      |

---

### ⚙️ **6️⃣ 송신/수신 윈도우의 차이**

|     구분    | 송신 윈도우         | 수신 윈도우                           |
| :-------: | :------------- | :------------------------------- |
| **관리 주체** | 송신 측           | 수신 측                             |
|   **의미**  | 아직 ACK 안 받은 범위 | 수신 버퍼에 남은 여유 공간                  |
| **조정 방식** | ACK 수신 시 확장    | 윈도우 크기 필드(window size)로 송신 측에 통보 |

---

### ⚙️ **7️⃣ 성능 관련: BDP 계산**

> 네트워크에서 **최적 윈도우 크기**는 **대역폭 × 지연시간(BDP)** 으로 계산합니다.

```
BDP = Bandwidth × RTT
예: 100Mbps × 0.1초 = 10Mbit (≈ 1.25MB)
```

💡 → RTT(왕복 지연)이 크면 윈도우 크기도 커야 전송 효율이 유지됨.

---

### ⚙️ **8️⃣ 시각적 흐름 요약**

```
[1][2][3][4][5] → 전송 중
ACK(1~3) 수신 → [6][7][8] 추가 전송 가능
ACK(4~5) 수신 → [9][10] 추가 전송
```

✅ 윈도우가 “앞으로 미끄러지듯 이동(Sliding)”
✅ ACK마다 전송 가능한 영역 확장

---

## 🎯 **면접용 요약**

> TCP의 슬라이딩 윈도우는 **ACK을 기다리지 않고 여러 패킷을 연속 전송**할 수 있게 하는 메커니즘입니다.
>
> 수신 측의 **윈도우 크기(Window Size)** 에 따라 송신 가능한 범위를 제한하며,
> ACK을 받으면 윈도우를 “앞으로 밀어(slide)” 효율적인 데이터 흐름을 유지합니다.
>
> 결과적으로 TCP는 **지연 시간 동안 전송 효율을 극대화**하고,
> **흐름 제어(Flow Control)** 를 통해 안정적인 통신을 보장합니다.

---

### 🔥 꼬리질문 예상

1. **혼잡 제어(Congestion Control)와 슬라이딩 윈도우의 관계는요?**
   → 송신 가능한 범위는 `min(cwnd, rwnd)`로 결정.
   cwnd는 네트워크 혼잡 기반, rwnd는 수신 버퍼 기반.

2. **윈도우 크기가 너무 작으면 어떤 문제가 생기나요?**
   → 대역폭을 다 활용하지 못함 (Throughput 저하).

3. **윈도우 크기를 동적으로 조절하는 이유는요?**
   → 수신 측 버퍼 상태 및 네트워크 혼잡도에 따라 최적화하기 위해.

</div>
</details>


## 36. RTT(Time Delay) 측정은 TCP 내부에서 어떻게 이뤄지나요?

<details close>
<summary><strong>💡 자세한 정답</strong></summary>

<div style="background-color:#f6f8fa; padding:15px; border-radius:10px;">

## 💡 **TCP의 RTT(Time Delay) 측정 메커니즘**

> TCP는 패킷 전송 후 ACK이 도착하기까지 걸리는 시간을 **RTT(Round Trip Time, 왕복 지연 시간)** 으로 측정합니다.
>
> 측정된 RTT를 기반으로 **재전송 타이머(RTO, Retransmission Timeout)** 를 계산하여
> 손실 감지 및 네트워크 안정성을 유지합니다.

---

### ⚙️ **1️⃣ RTT란 무엇인가**

|             항목            | 설명                                                        |
| :-----------------------: | :-------------------------------------------------------- |
| **RTT (Round Trip Time)** | 송신 측이 패킷 전송 후, 해당 패킷의 ACK을 받기까지 걸린 시간                     |
|           **단위**          | 밀리초 (ms)                                                  |
|           **의미**          | 네트워크 지연(Propagation + Processing + Queuing)을 포함한 전체 왕복 시간 |
|           **용도**          | RTO(재전송 타이머) 계산의 핵심 지표                                    |

---

### ⚙️ **2️⃣ RTT 측정 과정 (Sample RTT)**

1. 송신 시각 기록
2. 해당 패킷의 ACK 수신 시점 기록
3. 두 시점의 차이 = `SampleRTT`

```text
SampleRTT = ACK 수신 시간 - 전송 시간
```

💡 TCP는 모든 패킷이 아닌, **일부 세그먼트에 대해서만 RTT를 샘플링** (혼잡 방지 목적)

---

### ⚙️ **3️⃣ RTT의 가중 평균 (SRTT, DevRTT)**

> 네트워크 상황은 매 순간 변하기 때문에,
> TCP는 단일 RTT 값이 아닌 **지수 가중 평균(Exponential Weighted Moving Average)** 으로 추정치를 관리합니다.

#### 공식 (RFC 6298)

```text
SRTT = (1 - α) × SRTT + α × SampleRTT
RTTVAR = (1 - β) × RTTVAR + β × |SRTT - SampleRTT|
```

|          항목         | 의미                         | 기본값         |
| :-----------------: | :------------------------- | :---------- |
|       **SRTT**      | Smoothed RTT (평활화된 RTT 평균) | -           |
| **RTTVAR (DevRTT)** | RTT 변동폭(편차)                | -           |
|        **α**        | SRTT 가중치                   | 1/8 (0.125) |
|        **β**        | RTTVAR 가중치                 | 1/4 (0.25)  |

---

### ⚙️ **4️⃣ RTO (재전송 타이머) 계산**

> SRTT와 RTTVAR를 이용해 RTO를 동적으로 계산하여
> 너무 빠른 재전송(중복 전송)이나 너무 늦은 복구를 방지합니다.

#### 공식

```text
RTO = SRTT + max(G, K × RTTVAR)
```

|   항목  | 의미                            |
| :---: | :---------------------------- |
| **G** | Clock granularity (타이머 최소 단위) |
| **K** | 안전 계수 (보통 4)                  |

💡 즉, **평균 지연(SRTT)** + **변동 여유분(RTTVAR × 4)** 만큼 대기 후 재전송 수행.

---

### ⚙️ **5️⃣ 시각적 흐름 예시**

```
[패킷 #1 전송 시각] ───────────▶
                          [ACK 수신]
RTT 측정 = (ACK 시간) - (전송 시간)
SRTT, RTTVAR 갱신
→ RTO 조정
```

* RTT가 안정적 → RTO 감소 (빠른 재전송 가능)
* RTT 변동이 심함 → RTO 증가 (불필요한 재전송 방지)

---

### ⚙️ **6️⃣ 재전송 시의 예외 처리**

> 이미 재전송된 세그먼트에 대해서는 RTT 샘플링을 수행하지 않음.
> (왜냐하면 어떤 전송에 대한 ACK인지 구분 불가능하기 때문)

→ 이를 **Karn’s Algorithm** 으로 해결.

#### 📜 Karn’s Algorithm

1. 재전송된 세그먼트에 대한 RTT 측정 금지
2. 재전송이 발생한 경우 RTO를 **2배로 증가 (지연 보정)**
   → Exponential Backoff

---

### ⚙️ **7️⃣ 예시 (수치 시뮬레이션)**

|  전송 | SampleRTT(ms) | SRTT(ms) | RTTVAR(ms) | RTO(ms) |
| :-: | :------------ | :------- | :--------- | :------ |
|  초기 | 100           | 100      | 0          | 100     |
|  다음 | 120           | 102.5    | 5          | 122.5   |
|  다음 | 140           | 107.2    | 13         | 159     |
|  다음 | 110           | 107.7    | 12         | 155     |
|  다음 | 95            | 106.2    | 12         | 154     |

✅ RTT 변동이 크면 RTO 자동 상승 → 안정적 전송 보장.

---

## 🎯 **면접용 요약**

> TCP는 **RTT(왕복 지연)** 을 실시간으로 측정하여
> **SRTT(평균 RTT)** 와 **RTTVAR(변동폭)** 를 계산하고,
> 이를 기반으로 **RTO(재전송 타이머)** 를 동적으로 조정합니다.
>
> 또한 **Karn’s Algorithm** 을 통해 재전송 시 RTT 측정을 보류하고
> 네트워크 지연 환경에 안전하게 적응합니다.

---

### 🔥 꼬리질문 예상

1. **RTT가 급격히 증가하면 어떤 일이 발생하나요?**
   → RTO가 자동으로 커져 불필요한 재전송 방지.

2. **Karn’s Algorithm의 목적은요?**
   → 재전송된 세그먼트의 RTT를 잘못 측정하는 문제 방지.

3. **RTT와 RTO의 관계는?**
   → RTT는 실제 측정값, RTO는 재전송 기준값 (RTT 기반으로 계산됨).

</div>
</details>


## 37. 네트워크 성능 병목을 진단할 때 어떤 툴을 사용하나요?

<details close>
<summary><strong>💡 자세한 정답</strong></summary>

<div style="background-color:#f6f8fa; padding:15px; border-radius:10px;">

## 💡 **네트워크 병목(Bottleneck) 진단 도구**

> 네트워크 성능 저하가 발생했을 때는
> 패킷 지연, 손실, 혼잡, 포트 상태 등을 종합적으로 분석해야 합니다.
>
> 이때 사용하는 대표적인 툴로는 **`netstat`, `Wireshark`, `perfmon`, `nettop`** 등이 있습니다.

---

### ⚙️ **1️⃣ 주요 툴 요약**

|            툴 이름            | 용도                               | 플랫폼                     |
| :------------------------: | :------------------------------- | :---------------------- |
|        **Wireshark**       | 패킷 캡처 및 프로토콜 분석                  | Windows / macOS / Linux |
|         **netstat**        | 포트·연결 상태, 세션 수 확인                | 모든 OS                   |
|    **perfmon (성능 모니터)**    | 네트워크 인터페이스, CPU, 메모리 등 리소스 병목 추적 | Windows                 |
| **nettop / iftop / nload** | 실시간 트래픽 모니터링                     | macOS / Linux           |
|    **ping / traceroute**   | 지연(RTT), 경로(Route) 진단            | 모든 OS                   |
|          **iperf**         | 대역폭 및 전송 속도 측정                   | Windows / Linux         |
|         **tcpdump**        | CLI 기반 패킷 캡처 (Wireshark의 콘솔 버전)  | Linux / macOS           |
|          **nmap**          | 포트 스캔 및 보안 점검                    | Linux / Windows         |
|           **ss**           | netstat의 고급 대체 (Socket 상태 분석)    | Linux                   |

---

### ⚙️ **2️⃣ 대표 툴별 상세 설명**

#### ✅ **① Wireshark – 패킷 단위 분석**

* 네트워크 패킷을 **실시간으로 캡처** 후 **프로토콜 단위 분석** 가능
* TCP 재전송, RTT, 패킷 손실, 혼잡 상황을 시각적으로 확인
* 필터 예시:

  ```
  tcp.analysis.retransmission
  ip.addr == 192.168.0.10
  tcp.port == 8080
  ```

> 💡 TCP 재전송 횟수, RTT 분포, DUP ACK 등을 통해 **병목 위치(서버/네트워크/클라이언트)** 진단 가능

---

#### ✅ **② netstat – 연결 및 포트 상태 확인**

* 현재 열려 있는 **소켓, 포트, 연결 상태**를 조회
* 세션 수 과다, TIME_WAIT 누적 등 문제 진단에 유용

```bash
netstat -ano         # 포트별 PID/상태
netstat -s           # 프로토콜별 통계 (TCP retrans, segment 수 등)
```

> 💡 TIME_WAIT 과다 → 세션 재사용 문제
> 💡 ESTABLISHED 과다 → 연결 관리 병목

---

#### ✅ **③ perfmon (Windows Performance Monitor)**

* **네트워크 I/O, 패킷 손실, 대역폭 사용량** 등 시스템 성능을 통합적으로 추적
* 주요 카운터:

  * `Network Interface → Bytes Total/sec`
  * `TCPv4 → Segments Retransmitted/sec`
  * `Processor → % Processor Time`

> 💡 CPU·메모리·네트워크 병목을 함께 추적할 수 있는 종합 툴

---

#### ✅ **④ nettop / iftop – 실시간 트래픽 분석**

* 각 연결별 실시간 **송수신 속도 및 세션별 트래픽** 확인

```bash
sudo iftop -i eth0     # 인터페이스별 트래픽
sudo nettop             # macOS 실시간 네트워크
```

> 💡 특정 IP/포트가 과도한 트래픽을 점유하는지 실시간으로 확인 가능

---

#### ✅ **⑤ iperf – 네트워크 속도/대역폭 테스트**

* 송신 측과 수신 측 간 **순수 네트워크 성능(throughput)** 측정

```bash
iperf3 -s  # 서버 모드
iperf3 -c <server_ip> -p 5201  # 클라이언트 모드
```

> 💡 네트워크 자체 성능 문제인지, 애플리케이션 문제인지 구분 가능

---

#### ✅ **⑥ ping / traceroute**

* 네트워크 지연(RTT) 및 **경로 상 장애 구간** 탐지

```bash
ping 8.8.8.8
traceroute www.google.com
```

> 💡 특정 구간에서 RTT 급등 → 라우팅 병목 지점 추적 가능

---

### ⚙️ **3️⃣ 진단 예시 시나리오**

|    현상    | 원인 추정                  | 사용 툴                              |
| :------: | :--------------------- | :-------------------------------- |
|  RTT 급증  | 네트워크 지연, 라우팅 문제        | `ping`, `traceroute`, `Wireshark` |
|  전송속도 저하 | 대역폭 부족, 혼잡 윈도우 제한      | `iperf`, `perfmon`                |
|   세션 누적  | 연결 해제 누락, TIME_WAIT 과다 | `netstat`, `ss`                   |
|   패킷 손실  | 라우터 혼잡, MTU 문제         | `Wireshark`, `tcpdump`            |
| 특정 포트 지연 | 방화벽/포트 큐잉              | `nmap`, `netstat`                 |

---

## 🎯 **면접용 요약**

> 네트워크 병목을 진단할 때는 **계층별로 다른 도구**를 사용합니다.
>
> * **패킷 단위 분석** → `Wireshark`, `tcpdump`
> * **연결 상태 확인** → `netstat`, `ss`
> * **시스템 리소스 병목** → `perfmon`, `top`, `iotop`
> * **대역폭/지연 측정** → `iperf`, `ping`, `traceroute`
>
> 실무에서는 **Wireshark + netstat + perfmon** 조합이 가장 일반적입니다.

---

### 🔥 꼬리질문 예상

1. **TIME_WAIT 상태가 많을 때의 문제와 해결책은요?**
   → 포트 고갈 문제 발생 → `SO_REUSEADDR` / `SO_LINGER` 옵션 조정.

2. **Wireshark에서 TCP 재전송은 어떻게 필터링하나요?**
   → `tcp.analysis.retransmission` 필터 사용.

3. **iperf 측정 결과가 낮게 나온다면 어떤 가능성을 의심해야 하나요?**
   → MTU, 혼잡 윈도우, 네트워크 지연, CPU 리소스 부족 등.

</div>
</details>


## 38. 헤더 압축(Header Compression)의 목적은 무엇인가요?

<details close>
<summary><strong>💡 자세한 정답</strong></summary>

<div style="background-color:#f6f8fa; padding:15px; border-radius:10px;">

## 💡 **헤더 압축(Header Compression)의 목적**

> **헤더 압축(Header Compression)** 은
> IP·TCP·UDP 등의 **헤더 영역을 압축하여 전송 효율을 높이는 기술**입니다.
>
> 특히 **패킷 크기 대비 헤더 비중이 큰 환경(저대역폭, 무선망)** 에서
> **대역폭 절감과 지연 감소**를 목표로 사용됩니다.

---

### ⚙️ **1️⃣ 배경**

> 일반적인 IP + TCP 패킷의 헤더 크기:

|     계층     | 프로토콜    | 크기(Byte)  |
| :--------: | :------ | :-------- |
|  **IP 헤더** | IPv4 기준 | 20        |
| **TCP 헤더** | 기본      | 20        |
|   **총합**   |         | **40바이트** |

💡 만약 **64바이트 데이터**를 전송하면, 절반 이상이 헤더로 차지됨 → **비효율적 전송**

특히 다음 환경에서 심각한 문제:

* 무선 네트워크 (LTE, 5G 이전 세대)
* IoT, VPN, VoIP 등 소형 패킷 위주 서비스

---

### ⚙️ **2️⃣ 헤더 압축의 핵심 목적**

|         목적        | 설명                                |
| :---------------: | :-------------------------------- |
|    **① 대역폭 절감**   | 중복되는 헤더 필드(IP 주소, 포트 등)를 생략 또는 축소 |
|   **② 전송 효율 향상**  | 유효 데이터(payload) 비율 증가             |
|   **③ 전송 지연 감소**  | 패킷 크기 축소 → 전송 시간 단축               |
| **④ 무선 네트워크 최적화** | 제한된 채널 대역폭에서 효율 향상                |
| **⑤ 전력 절감 (모바일)** | 전송량 감소로 인한 송신 시간 단축               |

---

### ⚙️ **3️⃣ 작동 원리 요약**

> 헤더의 대부분은 “세션 내에서 반복되는 값”이므로,
> 최초 전송 시 전체 헤더를 보내고 이후에는 **변경된 필드만 전송**.

|                구분               | 설명                          |
| :-----------------------------: | :-------------------------- |
|    **1. 초기 패킷(Full Header)**    | 전체 헤더 포함                    |
| **2. 이후 패킷(Compressed Header)** | 변화된 필드만 전송 (예: 시퀀스 번호, 체크섬) |
|          **3. 수신 측 복원**         | 이전 헤더 상태를 기준으로 원본 헤더 재구성    |

---

### ⚙️ **4️⃣ 대표 프로토콜 예시**

|                     이름                     | 적용 계층      | 설명                         |
| :----------------------------------------: | :--------- | :------------------------- |
| **Van Jacobson TCP/IP Header Compression** | TCP/IP     | 초창기 헤더 압축 (PPP, SLIP에서 사용) |
|    **ROHC (Robust Header Compression)**    | IP/UDP/RTP | LTE, VoIP 등 무선망 표준 압축 방식   |
|      **IPHC (IP Header Compression)**      | IPv6       | IPv6 헤더의 40바이트를 소형화        |
|           **HC1, HC2 (6LoWPAN)**           | IPv6 / UDP | IoT 환경용 초경량 헤더 압축          |

---

### ⚙️ **5️⃣ 동작 예시 (Van Jacobson 방식)**

|     구분    | 전송 내용                   | 설명                  |
| :-------: | :---------------------- | :------------------ |
| **초기 전송** | IP(20) + TCP(20) + Data | 전체 헤더 송신            |
| **다음 전송** | 압축된 헤더(3~5바이트) + Data   | 시퀀스 증가량, Ack 변화만 전송 |
|   **결과**  | 약 85~90% 오버헤드 절감        |                     |

---

### ⚙️ **6️⃣ 적용 예시**

|           분야           | 설명                                   |
| :--------------------: | :----------------------------------- |
|     **VoIP (음성통신)**    | 작은 오디오 프레임 전송 시 헤더가 과도하게 큼 → 압축으로 절감 |
|      **VPN / 터널링**     | 다중 캡슐화 시 헤더 중복 제거                    |
| **IoT 네트워크 (6LoWPAN)** | 센서 데이터 전송 시 헤더 압축으로 배터리 절약           |
|  **모바일 네트워크 (LTE/5G)** | 무선 링크 효율 향상, 패킷 손실률 감소               |

---

## 🎯 **면접용 요약**

> **헤더 압축**의 목적은
> **반복되는 헤더 정보를 제거하거나 축소**하여
> **대역폭을 절감하고 전송 효율을 높이는 것**입니다.
>
> 특히 **저대역폭·무선 환경**에서 **지연 감소**와 **패킷 효율성 향상**을 위해 사용됩니다.

---

### 🔥 꼬리질문 예상

1. **헤더 압축이 실패하면 어떤 문제가 생기나요?**
   → 이전 상태 정보를 잃어 헤더 복원이 불가능 → 초기화(Full Header) 전송 필요.

2. **ROHC와 기존 TCP 헤더 압축의 차이는요?**
   → ROHC는 손실 많은 무선망 환경에서도 복원 가능한 강인한 압축 방식.

3. **헤더 압축이 적용되는 계층은 어디인가요?**
   → 전송 계층(TCP/UDP)과 네트워크 계층(IP) 사이 (Layer 3~4 사이).

</div>
</details>

## 39. 지연이 큰 네트워크에서 Throughput이 낮아지는 이유는 무엇인가요?

<details close>
<summary><strong>💡 자세한 정답</strong></summary>

<div style="background-color:#f6f8fa; padding:15px; border-radius:10px;">

## 💡 **지연(Latency)이 클수록 Throughput이 낮아지는 이유**

> 네트워크에서 **Throughput(실제 전송 처리량)** 은
> 단순히 대역폭(Bandwidth)만으로 결정되지 않습니다.
>
> **전송 지연(Latency)** 이 클 경우,
> TCP의 **슬라이딩 윈도우(ACK 기반 전송 제어)** 구조로 인해
> **데이터 파이프라인이 비효율적으로 채워지지 못하기 때문**입니다.

---

### ⚙️ **1️⃣ 기본 개념 정리**

|          용어          | 의미                                           |
| :------------------: | :------------------------------------------- |
|  **Bandwidth (대역폭)** | 단위 시간당 전송 가능한 **최대 데이터량** (이론적 한계)           |
|  **Latency (지연 시간)** | 데이터 1개의 왕복 전송에 걸리는 **RTT (Round Trip Time)** |
| **Throughput (처리량)** | 실제 전송된 데이터의 평균 속도 (실효 속도)                    |

---

### ⚙️ **2️⃣ 핵심 원리**

> TCP는 **ACK을 받아야 다음 데이터를 전송할 수 있는 구조**
> → RTT가 길면 ACK 대기 시간도 길어짐
> → 그동안 송신 윈도우가 비어 있음 → **전송 파이프라인이 비효율**

#### 📘 예시

```
송신자: [데이터 전송] → (RTT 대기 중...) → ACK 수신 후 다음 전송
```

RTT가 길수록 **"대기 시간 대비 전송량"** 이 줄어듭니다.

---

### ⚙️ **3️⃣ 수식 관계 (대역폭-지연 곱, BDP)**

> 네트워크의 이론적 최대 처리량은 다음으로 표현됩니다.

```text
Throughput ≈ Window Size / RTT
```

|        항목       | 설명                              |
| :-------------: | :------------------------------ |
| **Window Size** | 송신 측이 한 번에 보낼 수 있는 데이터 양 (Byte) |
|     **RTT**     | 왕복 지연 시간                        |

즉,

* RTT ↑ → Throughput ↓
* Window Size ↑ → Throughput ↑

---

### ⚙️ **4️⃣ 예시 계산**

|     항목     | 값                |
| :--------: | :--------------- |
|     대역폭    | 100 Mbps         |
|     RTT    | 100 ms (0.1초)    |
| TCP 윈도우 크기 | 64 KB (0.5 Mbit) |

```text
Throughput = 0.5 Mbit / 0.1s = 5 Mbps
```

👉 실제 대역폭(100 Mbps)의 **5%만 사용**됨.
👉 RTT가 길면 TCP 파이프라인이 비어 있어 **대역폭을 다 활용하지 못함.**

---

### ⚙️ **5️⃣ 해결 방법 (지연 환경에서 성능 개선)**

|                 방법                | 설명                                    |
| :-------------------------------: | :------------------------------------ |
|    **① 윈도우 크기(Window Size) 확대**   | 더 많은 데이터를 한 번에 전송 (Window Scaling 옵션) |
| **② 병렬 연결 사용 (HTTP/2, 게임 스트림 등)** | 여러 세션으로 대역폭 분산                        |
|  **③ TCP Congestion Control 튜닝**  | Reno → CUBIC, BBR 등 고지연 네트워크용 알고리즘    |
|     **④ 전송 계층 변경 (UDP 기반 전송)**    | ACK 대기 구조 제거, 자체 신뢰성 구현 (QUIC 등)      |

---

### ⚙️ **6️⃣ 시각적 예시**

#### 🔹 지연이 짧은 네트워크

```
[보냄][보냄][보냄][보냄]
        ↳ ACK 빠르게 수신 → 지속적 전송
→ 파이프가 가득 참 → 높은 Throughput
```

#### 🔹 지연이 긴 네트워크

```
[보냄]......(ACK 대기중)......
→ RTT 동안 전송 정체 → 파이프 비어 있음 → 낮은 Throughput
```

---

### ⚙️ **7️⃣ 실제 사례**

|            환경           |  RTT  | 윈도우 크기 |  Throughput 특성  |
| :---------------------: | :---: | :----: | :-------------: |
|        **로컬 LAN**       |  1ms  |  64KB  | 64 MB/s (매우 빠름) |
|         **국내망**         |  20ms |  64KB  |     3.2 MB/s    |
|     **해외망 (미국↔한국)**     | 200ms |  64KB  |     320 KB/s    |
| **같은 망 + 윈도우 확장 (1MB)** | 200ms |   1MB  |      5 MB/s     |

> 💡 RTT가 10배 커지면, 윈도우도 10배 키워야 동일 처리량 유지 가능.

---

## 🎯 **면접용 요약**

> TCP는 **ACK 기반의 슬라이딩 윈도우 구조**를 가지므로,
> 네트워크 지연이 커질수록 **ACK 대기 시간이 길어지고 파이프라인이 비게 되어**
> **실제 처리량(Throughput)** 이 감소합니다.
>
> 이를 보완하기 위해 **윈도우 크기 확장(Window Scaling)** 이나
> **고지연 환경용 혼잡 제어 알고리즘(BBR, CUBIC)** 을 사용합니다.

---

### 🔥 꼬리질문 예상

1. **Throughput을 높이려면 어떤 방법이 있나요?**
   → 윈도우 크기 확장, RTT 감소, 패킷 손실률 최소화.

2. **RTT가 높아도 UDP는 영향이 적은 이유는요?**
   → UDP는 ACK 기반 전송이 아니므로 대기 구간이 없음.

3. **TCP Window Scaling 옵션은 무엇을 의미하나요?**
   → 기존 16비트 윈도우(최대 65KB)를 확장해 최대 1GB까지 설정 가능한 기능.

</div>
</details>


## 40. TCP vs IOCP 서버의 확장성(Scalability)을 비교해보세요.

<details close>
<summary><strong>💡 자세한 정답</strong></summary>

<div style="background-color:#f6f8fa; padding:15px; border-radius:10px;">

## 💡 **TCP vs IOCP 서버의 확장성 비교**

> **TCP**는 프로토콜이고,
> **IOCP (I/O Completion Port)** 는 **Windows 환경의 비동기 I/O 모델**입니다.
>
> 따라서 “TCP 서버”라고 할 때는 일반적인 **Blocking / Non-blocking / select 기반 서버**,
> “IOCP 서버”는 **Completion 기반 비동기 서버**로 비교합니다.

---

### ⚙️ **1️⃣ 비교 요약**

|        항목       | **일반 TCP 서버 (Blocking / select 등)** | **IOCP 서버 (Windows)**              |
| :-------------: | :---------------------------------- | :--------------------------------- |
|  **I/O 처리 방식**  | Blocking 또는 Non-blocking            | Completion 기반 비동기 (Overlapped I/O) |
|    **스레드 모델**   | 1:1 or N:1 (FD per Thread)          | Worker Thread Pool (커널 관리)         |
|   **스케일링 한계**   | FD 수 증가 시 선형적 부하 증가                 | 대량 FD 처리에도 일정 부하 유지                |
|   **컨텍스트 스위칭**  | 다수 스레드로 인해 빈번                       | 최소화 (커널이 효율적 분배)                   |
| **Blocking 위험** | 있음 (accept, recv 등)                 | 없음 (비동기 완료 통지)                     |
|     **확장성**     | 수천~수만 수준                            | 수십~수백만 수준까지 가능                     |
|    **적합한 환경**   | 단일 코어, 적은 클라이언트                     | 멀티코어, 대규모 동접 처리용 서버                |

---

### ⚙️ **2️⃣ 일반 TCP 서버의 한계 (select / thread-per-client)**

#### 🔹 구조

```cpp
while (true) {
    for (socket in sockets)
        if (FD_ISSET(socket, readfds))
            recv(socket);
}
```

* `select`는 **FD 집합 전체를 매번 순회**
* FD 수가 많아질수록 → **O(N)** 부하 발생
* 스레드 기반 서버는 **FD당 스레드 1개** → **Context Switching 폭증**

> 🔸 클라이언트 수가 수천 이상일 때 성능 급락.

---

### ⚙️ **3️⃣ IOCP 서버의 구조**

#### 🔹 비동기 I/O + Completion 큐 기반

```cpp
CreateIoCompletionPort(...);   // Completion Port 생성
PostQueuedCompletionStatus(...); // 비동기 요청 등록
GetQueuedCompletionStatus(...); // 완료 이벤트 대기
```

* 커널이 I/O 완료 시 **Completion Packet**을 큐에 전달
* 워커 스레드(보통 CPU 코어 × 2)가 큐에서 작업을 가져가 처리
* FD 수가 증가해도 커널이 효율적으로 분배 → **O(1) 수준 부하**

> 🔸 수만~수십만 동시 세션도 안정적으로 처리 가능.

---

### ⚙️ **4️⃣ 확장성의 본질적 차이**

|       관점       | **일반 TCP (select, thread)** | **IOCP**        |
| :------------: | :-------------------------- | :-------------- |
|  **I/O 처리 단위** | 스레드 단위                      | Completion 단위   |
| **FD 증가 시 부하** | 선형 증가                       | 일정 수준 유지        |
|    **스레드 수**   | FD 개수에 비례                   | CPU 코어 수에 비례    |
|  **커널 개입 정도**  | 호출 시마다 FD 체크                | 커널이 완료 시점에 큐 등록 |
|   **확장성 한계**   | 수천 연결 이상 시 급격한 CPU 부하       | 수십만 동시 연결 처리 가능 |

---

### ⚙️ **5️⃣ 실제 수치 비교 예시**

|             항목            | 일반 TCP (select) |   IOCP  |
| :-----------------------: | :-------------: | :-----: |
|        **동시 접속자 수**       |      ~2,000     | 50,000+ |
|  **Context Switch / sec** |     수천~수만 회     | 수백 회 수준 |
| **CPU 사용률 (10K clients)** |     80~100%     |  10~20% |
|  **Latency (Broadcast)**  |     15~25ms     |  2~5ms  |
|       **Throughput**      |        낮음       |  매우 높음  |

> ⚙️ 실제로 Windows Server 환경에서 IOCP는 FD 증가에 따라 CPU 부하가 거의 선형적으로 증가하지 않음.

---

### ⚙️ **6️⃣ IOCP의 핵심 장점 요약**

|        항목        | 설명                                    |
| :--------------: | :------------------------------------ |
| **1. 비동기 완료 통지** | I/O 작업이 끝나면 커널이 직접 완료 큐에 등록           |
| **2. 최소 스레드 모델** | CPU 코어 수 기반 Worker Pool로 컨텍스트 스위칭 최소화 |
|  **3. FD 수 확장성** | 수십만 소켓 처리 가능 (Windows 내부 한계 내)        |
|  **4. 높은 자원 효율** | 유휴 스레드 없음, 커널 레벨 대기로 낭비 최소화           |

---

## 🎯 **면접용 요약**

> 일반 TCP 서버는 **FD 수 증가에 따라 부하가 선형 증가**하지만,
> **IOCP 서버는 커널 수준에서 비동기 완료를 관리하기 때문에**
> **FD가 늘어나도 오버헤드가 거의 증가하지 않습니다.**
>
> 따라서 IOCP는 **대규모 동시 접속 환경에서 매우 높은 확장성(Scalability)** 을 보입니다.

---

### 🔥 꼬리질문 예상

1. **IOCP가 epoll보다 나은 점은?**
   → 커널 수준 스케줄링 + Completion 기반으로 **스레드 효율이 더 높음** (단, Windows 한정).

2. **IOCP의 단점은 없나요?**
   → 구조 복잡도 높고, 디버깅 어려움. (초기 진입 장벽 있음)

3. **FD가 10만 개 이상일 때 select와 IOCP의 차이는?**
   → select는 매 루프마다 10만 소켓 검사 (O(N)),
   IOCP는 커널이 완료된 이벤트만 전달 (O(1)).

</div>
</details>
