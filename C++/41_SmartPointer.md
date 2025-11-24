# ♻️ C++ 스마트 포인터 정리 (서버 프로그래밍 관점)

스마트 포인터는 **소유권(ownership)** 과 **수명(lifetime)** 을 코드에 명시해 주는 RAII 도구다.  
게임 서버/네트워크 서버처럼 **비동기 콜백**, **멀티스레드**, **에러 경로**가 많은 환경에서
`new/delete`를 직접 관리하면 **누수**, **이중 해제**, **use-after-free**, **순환 참조**가 쉽게 발생한다.  
스마트 포인터는 이런 문제를 크게 줄여준다.

---

## 🧭 개요 한눈에

| 타입 | 핵심 의미 | 소유권 | 스레드 안전성(참고) | 주요 용도 |
|---|---|---|---|---|
| `std::unique_ptr<T>` | 단독 소유(이동만 가능) | 1개 | 자체는 가벼움 | 리소스의 **명확한 소유자**가 하나일 때(버퍼, 파일, 소켓 등) |
| `std::shared_ptr<T>` | 공유 소유(참조계수) | N개 | **참조계수 증감은 원자적**, 객체 자체는 아님 | 동일 객체를 여러 컴포넌트가 **함께 소유**해야 할 때(세션, 엔티티) |
| `std::weak_ptr<T>` | 비소유(관찰자) | 0 | 가볍지만 `lock()` 비용 존재 | **순환 참조 방지**, 콜백/레지스트리에서 **존재만 확인** |

> 원칙: **기본은 `unique_ptr`**, 정말 공유가 필요할 때만 `shared_ptr`,  
> 공유로 인해 생기는 순환/수명 문제는 `weak_ptr`로 해결.

---

## 1) `std::unique_ptr` — 단독 소유, 이동만 가능

### 핵심 포인트
- 복사 불가, **이동(move)** 만 가능 → 소유권이 코드 흐름에 **명확히** 드러난다.
- 해제 시점은 소유 객체의 스코프 종료(파괴자)와 함께 **자동** 처리(RAII).
- 표준 팩토리: `std::make_unique<T>(args...)`  
  - 예외 안전(생성 중 예외 시 누수 없음), 짧고 빠름.

### 기본 사용
```cpp
#include <memory>

std::unique_ptr<char[]> MakeBuffer(size_t sz) {
    auto buf = std::make_unique<char[]>(sz); // 배열은 [] 버전
    // ... 초기화 ...
    return buf; // 이동 반환 (NRVO/Move)
}
````

### 컨테이너에 보관 (자원 캐시, 풀 등)

```cpp
#include <vector>
#include <memory>

struct Packet { /* ... */ };

std::vector<std::unique_ptr<Packet>> g_packets;

void PushPacket(std::unique_ptr<Packet> p) {
    g_packets.emplace_back(std::move(p)); // 소유권 이전
}
```

### 커스텀 deleter (소켓/HANDLE 등)

```cpp
#ifdef _WIN32
#include <winsock2.h>
#include <windows.h>
#endif
#include <memory>

struct SocketDeleter {
    void operator()(SOCKET s) const noexcept {
        if (s != INVALID_SOCKET) ::closesocket(s);
    }
};
using SocketUPtr = std::unique_ptr<std::remove_pointer_t<SOCKET>, SocketDeleter>;

struct HandleDeleter {
    void operator()(HANDLE h) const noexcept {
        if (h) ::CloseHandle(h);
    }
};
using HandleUPtr = std::unique_ptr<std::remove_pointer_t<HANDLE>, HandleDeleter>;
```

> 배열 해제: `unique_ptr<T[]>`는 자동으로 `delete[]` 호출.
> `shared_ptr`은 배열 특수화가 없으니 **커스텀 deleter** 필요.

---

## 2) `std::shared_ptr` — 공유 소유, 참조계수

### 핵심 포인트

* **참조계수(atomic)** 로 소유권을 공유. 마지막 소유자가 파괴될 때 해제.
* `std::make_shared<T>(args...)` 권장: **객체 + 컨트롤 블록**을 **한 번에** 할당 → 빠르고 캐시 친화적.
* 객체의 **동시 접근은 안전하지 않다** → 별도 락/동기화 필요.
* 남용하면 **성능 비용**(원자적 refcount·할당)과 **수명 얽힘**(순환 참조)을 초래.

### 서버 예: 세션을 공유 소유

```cpp
#include <memory>
#include <atomic>
#include <mutex>
#include <vector>

struct Session : std::enable_shared_from_this<Session> {
    void Start() {
        // 비동기 I/O 등록 시 콜백에 자기 수명 연장 필요할 때:
        auto self = shared_from_this(); // **주의**: 반드시 shared_ptr로 생성된 뒤에만 호출
        PostRecv(self);
    }
    void PostRecv(std::shared_ptr<Session> self);
    void OnRecv(const char* data, size_t len) {
        // ...
    }
};

using SessionPtr = std::shared_ptr<Session>;
```

### `enable_shared_from_this` 주의점

* `shared_from_this()`는 **해당 객체가 `shared_ptr`로 관리 중일 때만** 유효.
* **생성자 내부에서 호출 금지** (아직 컨트롤 블록 연결 전일 수 있음) → 팩토리/생성 이후 `Start()` 등에서 사용.

### 비동기 콜백에서 **weak\_ptr** 패턴 (메모리 안전)

```cpp
void AsyncRead(SessionPtr s) {
    std::weak_ptr<Session> w = s;
    PostIo([w](const char* data, size_t len) {
        if (auto self = w.lock()) {   // 살아 있으면 안전
            self->OnRecv(data, len);
        } // 아니면 조용히 무시 (세션이 이미 종료됨)
    });
}
```

### 공유 소유 생성 금지 패턴

```cpp
Foo* raw = new Foo;
std::shared_ptr<Foo> a(raw);
std::shared_ptr<Foo> b(raw); // ❌ 서로 다른 컨트롤 블록 → 이중 해제
```

> 항상 **단 한 번만** raw 포인터를 `shared_ptr`로 감싸고,
> 나머지는 그 `shared_ptr`을 **복사**해서 사용.

### aliasing 생성자 (부모 소유 유지하며 하위 뷰 노출)

```cpp
struct Player { std::vector<int> inventory; };
std::shared_ptr<Player> p = std::make_shared<Player>();
auto inv_view = std::shared_ptr<int>(p, p->inventory.data()); // p가 수명 보장
```

---

## 3) `std::weak_ptr` — 비소유 관찰자, 순환 참조 방지

### 핵심 포인트

* `shared_ptr`의 **참조계수를 증가시키지 않음** → 가볍고 순환 고리 끊기에 적합.
* 사용 시 `lock()`으로 일시 `shared_ptr` 획득(살아있으면 생성, 아니면 빈 포인터).

### 순환 참조 예 (잘못된 패턴)

```cpp
struct Pet;
struct Player {
    std::shared_ptr<Pet> pet;
};
struct Pet {
    std::shared_ptr<Player> owner;
};
// Player ↔ Pet 서로 shared_ptr 보유 → 절대 해제 안 됨 (메모리 누수)
```

### 해결: 한쪽을 `weak_ptr`

```cpp
struct Pet;
struct Player {
    std::shared_ptr<Pet> pet;     // 소유
};
struct Pet {
    std::weak_ptr<Player> owner;  // 관찰자
};
```

### 이벤트/레지스트리에서의 활용

* 콜백 테이블, 글로벌 매니저는 **`weak_ptr`로만 보관** → 대상 수명을 강제하지 않음.
* 실제 사용 시에만 `lock()` → 유효성 확인 후 처리.

---

## 4) 스레드 안전성 & 성능

* `shared_ptr`의 **참조계수 증감은 원자적** → 서로 다른 스레드 간 복사/파괴는 안전.
  그러나 **객체 내부 데이터**는 안전하지 않다 → 별도 락/원자 연산 필요.
* **핫패스**(패킷 루프, 매 틱)에서 `shared_ptr` 복사 남용은 피하자.

  * 가능하면 **`weak_ptr` + `lock()` 범위 최소화**, 또는 **raw 포인터 + 외부 수명 보장**.
* C++20: `std::atomic<std::shared_ptr<T>>` 지원. (C++17은 `atomic_load/store` 유틸 사용)
* `make_shared` 장점: **단일 할당**(객체+컨트롤 블록).
  단, **커스텀 deleter/메모리 리소스**가 필요하면 직접 생성이 유리할 수 있다.

---

## 5) 소유권 패턴 가이드 (서버 실전)

### ✅ 권장

* 리소스의 유일 소유: `unique_ptr` (버퍼, 파일, 소켓, 핸들)
* 여러 컴포넌트가 함께 소유: `shared_ptr` (세션/엔티티)
* 콜백/레지스트리/예약 작업: `weak_ptr` 저장 → 사용 시 `lock()`
* 컨테이너: `vector<unique_ptr<T>>`, `unordered_map<Key, unique_ptr<T>>`
  (명확한 소유, 이동으로 등록/해지 쉬움)
* 자기 참조 콜백: `enable_shared_from_this` + **생성자 외부**에서 사용

### ❌ 지양

* **무분별한 `shared_ptr` 남발** (비용 + 수명 얽힘)
* raw 포인터로 **소유를 암시** (누가 해제 책임인지 모호)
* `shared_ptr` 순환 참조 (반드시 `weak_ptr`로 끊기)
* 생성자에서 `shared_from_this()` 호출 (UB)

---

## 6) 서버 예제 모음

### (A) 세션 수명 안전한 비동기 콜백

```cpp
#include <memory>
#include <functional>

struct Session : std::enable_shared_from_this<Session> {
    void Start() {
        ReadLoop();
    }

    void ReadLoop() {
        auto self = shared_from_this(); // 필요 시 수명 연장
        PostIo([w = std::weak_ptr<Session>(self)](const char* buf, size_t len){
            if (auto s = w.lock()) {
                s->OnRecv(buf, len);
                s->ReadLoop(); // 다음 I/O
            } // 세션이 이미 종료되었다면 조용히 반환
        });
    }

    void OnRecv(const char*, size_t) { /* ... */ }
};

std::shared_ptr<Session> MakeSession() {
    return std::make_shared<Session>();
}
```

### (B) 소켓/HANDLE RAII 래퍼

```cpp
#ifdef _WIN32
#include <winsock2.h>
#include <windows.h>
#endif
#include <memory>

using SocketUPtr = std::unique_ptr<std::remove_pointer_t<SOCKET>, SocketDeleter>;
using HandleUPtr = std::unique_ptr<std::remove_pointer_t<HANDLE>, HandleDeleter>;

SocketUPtr CreateSocketRAII() {
    SOCKET s = ::socket(AF_INET, SOCK_STREAM, 0);
    return SocketUPtr(s); // 스코프 끝에서 closesocket 자동 호출
}
```

### (C) 순환 참조 방지 (Player ↔ Pet)

```cpp
#include <memory>
#include <string>

struct Player;
struct Pet {
    std::weak_ptr<Player> owner; // 🔑 약한 참조
    std::string name;
};

struct Player {
    std::string name;
    std::shared_ptr<Pet> pet;    // 강한 참조
};

int main() {
    auto p = std::make_shared<Player>();
    auto pet = std::make_shared<Pet>();
    p->pet = pet;
    pet->owner = p; // 순환 고리 끊김
}
```

---

## 7) 체크리스트 (면접/리뷰용)

* 기본은 **`unique_ptr`**: 공유가 필요할 때만 `shared_ptr`
* `shared_ptr` 사용 시 **순환 참조?** 한쪽은 `weak_ptr`로
* 콜백/스케줄러는 **`weak_ptr` 보관 + `lock()` 시점 최소화**
* `enable_shared_from_this`는 **생성자 외부**에서
* 배열은 `unique_ptr<T[]>`, `shared_ptr`은 **커스텀 deleter**
* 핫패스에서 `shared_ptr` 복사 남용 금지(프로파일링)
* 스레드 안전은 **참조계수만** 보장 → 객체 데이터는 별도 보호

---

## 🔚 요약

* 스마트 포인터는 서버에서 **수명과 소유권을 명시**하고,
  예외/에러/비동기 경로에서도 **안전한 해제**를 보장한다.
* 올바른 조합(`unique_ptr` 중심, 필요 시 `shared_ptr` + `weak_ptr`)과 패턴(콜백에서 `weak_ptr`)을 쓰면
  **누수/크래시/데드락/좀비 세션** 같은 버그를 크게 줄일 수 있다.


