# 🔗 C++ 함수 포인터(Function Pointer) 완전 정복

> 함수 포인터는 **함수의 주소를 변수처럼 저장하고, 나중에 호출하는 문법**이다.  
> 특히 **게임 서버 개발**과 같은 **고성능 시스템 프로그래밍** 분야에서, 함수 포인터는 **콜백 구조**, **이벤트 처리기**, **네트워크 패킷 핸들러** 등에 자주 사용된다.

---

## ✅ 왜 함수 포인터를 사용하는가?

| 목적 | 설명 |
|------|------|
| 동적 호출 | 실행 시점에 어떤 함수를 호출할지 결정 가능 (유연성 ↑) |
| 콜백 패턴 | 사용자 정의 동작을 나중에 실행 (예: 네트워크 수신 콜백) |
| 핸들러 매핑 | 명령어, 패킷 코드 등과 함수 연결 시 사용 |
| 오버헤드 감소 | 가상 함수보다 빠를 수 있음 (vtable 우회) |
| 의존성 분리 | 공통 인터페이스 정의 없이 다양한 함수 호출 가능 |

---

## 📌 함수 포인터 문법 기본

### ✨ 기본 형태
```cpp
반환형 (*변수명)(매개변수리스트);
```

### 📎 예시
```cpp
int Add(int a, int b) {
    return a + b;
}

int (*funcPtr)(int, int) = Add;

int result = funcPtr(3, 4);  // 7
```

---

## 🔁 함수 포인터 배열

함수를 배열처럼 저장하고 반복문으로 호출할 수 있음.

```cpp
int Add(int a, int b) { return a + b; }
int Sub(int a, int b) { return a - b; }

int (*operations[2])(int, int) = { Add, Sub };

for (int i = 0; i < 2; i++) {
    std::cout << operations[i](10, 5) << "\n";
}
```

---

## 📦 typedef/using을 통한 함수 포인터 정의

코드를 간결하게 하고, 가독성을 높인다.

```cpp
using Operation = int(*)(int, int);

int Add(int a, int b) { return a + b; }
int Sub(int a, int b) { return a - b; }

Operation op = Add;
std::cout << op(2, 3);  // 5
```

---

## 🧠 고급 문법: 함수 포인터를 매개변수로 전달

### 함수 자체를 인자로 받기
```cpp
void Execute(Operation op, int x, int y) {
    std::cout << "Result: " << op(x, y) << "\n";
}

Execute(Add, 3, 4);  // Result: 7
```

---

## 🧵 게임 서버에서의 활용 예

### 💡 예시 1: 패킷 핸들러 테이블
```cpp
enum class PacketType {
    LOGIN,
    MOVE,
    CHAT
};

void HandleLogin()  { std::cout << "Login Packet\n"; }
void HandleMove()   { std::cout << "Move Packet\n"; }
void HandleChat()   { std::cout << "Chat Packet\n"; }

using PacketHandler = void(*)();

PacketHandler packetTable[3] = {
    HandleLogin,
    HandleMove,
    HandleChat
};

// 패킷 수신 시 처리
void OnPacketReceived(PacketType type) {
    packetTable[static_cast<int>(type)]();
}
```

### 💡 예시 2: 커맨드 패턴 대체
```cpp
void Attack()  { std::cout << "Player attacks!\n"; }
void Defend()  { std::cout << "Player defends!\n"; }

std::unordered_map<std::string, void(*)()> commandMap = {
    { "attack", Attack },
    { "defend", Defend }
};

std::string input = "attack";
commandMap[input]();  // Player attacks!
```

---

## 🧩 함수 포인터 vs 람다 vs std::function

| 방식 | 설명 | 캡처 가능 여부 | 비용 | 유연성 |
|------|------|----------------|------|--------|
| 함수 포인터 | 단순, 빠름 | ❌ | 낮음 | 낮음 |
| 람다 표현식 | 코드 내에서 간결하게 작성 | ✅ | 중간 | 중간 |
| `std::function` | 객체처럼 함수 관리 가능 | ✅ | 높음 | 높음 |

> **게임 서버 개발**에서는 **성능 우선 → 함수 포인터 또는 람다 (캡처 없음)** 을 선호하는 경우가 많다.

---

## ⚠️ 주의사항

- 함수 시그니처가 정확히 일치해야 한다.
- 클래스의 멤버 함수는 일반 함수 포인터로 받을 수 없다.
  → 멤버 함수 포인터는 `void (ClassName::*)(args)` 형식 사용

### 🧪 멤버 함수 포인터 예시
```cpp
class NPC {
public:
    void Talk() {
        std::cout << "NPC says hello\n";
    }
};

void Execute(NPC* npc, void (NPC::*action)()) {
    (npc->*action)();  // 멤버 함수 호출
}

NPC npc;
Execute(&npc, &NPC::Talk);  // NPC says hello
```

---

## 🧠 마무리 정리

| 항목 | 설명 |
|------|------|
| 핵심 기능 | 함수 주소를 저장하고 나중에 호출 |
| 주 사용처 | 콜백, 이벤트 처리기, 핸들러 매핑, 디커플링 |
| 서버 분야 | 패킷 핸들러 테이블, 게임 커맨드 처리기 등 |
| 문법 요소 | 기본 포인터, typedef/using, 배열, 멤버 함수 등 |
| 대체 방식 | 람다, `std::function`, 인터페이스 기반 설계 |

---

> 🎮 게임 서버에서 수많은 패킷을 빠르게 구분하고 처리해야 할 때, 함수 포인터 기반의 테이블 구조는 매우 빠르고 효율적인 선택이다.  
> 클래스 기반 설계와 조합하여 사용하면 훨씬 더 유연한 아키텍처를 만들 수 있다.
