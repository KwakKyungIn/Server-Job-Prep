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

패킷 단위 처리를 위해 서버에서는 어떤 전략을 사용하나요?
→ 헤더 기반 length-prefix 구조, 고정 길이 or delimiter 방식.

TCP에서 “헤더 + 바디” 구조로 데이터를 보내는 이유는?
→ 메시지 경계 명확화, 파싱 용이성, 변동 길이 대응.

패킷 재전송은 TCP 내부적으로 어떻게 처리되나요?
→ ACK 타임아웃 or 중복 ACK 감지 기반 재전송.

TCP에서 순서가 뒤섞인 패킷은 어떻게 복원되나요?
→ 시퀀스 번호 기반 재조립 후 ACK 전송.

🔥 [심화] 서버에서 클라이언트가 비정상 종료(전원 끔 등)된 경우 어떻게 감지하나요?
→ KeepAlive, recv=0 반환, 소켓 오류(WSAECONNRESET) 기반 탐지.

🧾 4. 네트워크 디버깅 및 성능 관련

네트워크 지연(latency)과 대역폭(bandwidth)의 차이는?

패킷 손실이 발생했을 때 TCP는 어떻게 반응하나요?

게임 서버에서 UDP를 사용하는 이유는?

UDP 기반 신뢰성 보장을 위해 사용할 수 있는 기법은?
→ 시퀀스 번호, ACK, 재전송 타이머 등 커스텀 구현.

TCP의 슬라이딩 윈도우(Sliding Window)란?
→ 연속된 패킷 송신 관리, 효율적 ACK 처리 메커니즘.

🔥 [심화] RTT(Time Delay) 측정은 TCP 내부에서 어떻게 이뤄지나요?
→ RTT 샘플링 + 지수 가중 평균(SRTT, DevRTT) → RTO 계산.

네트워크 성능 병목을 진단할 때 어떤 툴을 사용하나요?
→ netstat, Wireshark, perfmon, nettop 등.

헤더 압축(Header Compression)의 목적은 무엇인가요?

지연이 큰 네트워크에서 Throughput이 낮아지는 이유는?

🔥 [심화] TCP vs IOCP 서버의 확장성(Scalability)을 비교해보세요.
→ 멀티 스레드 기반의 IOCP가 FD 증가 시 오버헤드 적음.