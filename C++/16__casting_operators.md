# 🎯 C++ 타입 캐스팅 정리 (Casting in C++)

> C++에서는 `static_cast`, `dynamic_cast`, `const_cast`, `reinterpret_cast` 네 가지 명시적 형변환 연산자를 제공한다.  
> 각각의 목적, 안전성, 사용 조건이 다르며, **면접 단골 주제**로 자주 등장한다.

---

## 🧱 1. `static_cast`

### ✅ 개요
- **가장 기본적인 타입 변환 연산자**로, 타입 시스템에 비춰봤을 때 **논리적으로 허용 가능한 변환만 수행**한다.
- 컴파일 타임에 타입 검사를 수행하지만, **런타임 안전성은 보장하지 않는다**.
- **업캐스팅(자식 → 부모)**, 기본 타입 간 변환(int ↔ float) 등에 자주 사용된다.
- 포인터, 참조, 값 타입 모두 변환 가능.

### 🧪 예시 1: 기본형 간의 변환
```cpp
float f = 3.14f;
int i = static_cast<int>(f);  // i == 3
```

### 🧪 예시 2: 업캐스팅 (안전)
```cpp
class GameObject {};
class Player : public GameObject {};

Player* p = new Player();
GameObject* g = static_cast<GameObject*>(p); // OK: 자식 → 부모
```

### ⚠️ 예시 3: 다운캐스팅 (위험 가능성 있음)
```cpp
GameObject* g = new GameObject();
Player* p = static_cast<Player*>(g); // 컴파일 OK, 실행 시 미정 (UB 가능성 있음)
```

---

## 🎯 2. `dynamic_cast`

### ✅ 개요
- **상속 관계**에서 안전한 캐스팅을 수행할 수 있게 해주는 연산자.
- **RTTI (Run-Time Type Information)**를 사용하여 **실행 시간에 타입을 확인**한다.
- **반드시 하나 이상의 virtual 함수가 있어야 함.**
- **다운캐스팅 시 실패하면 `nullptr` 반환**, 참조 타입은 `std::bad_cast` 예외 발생.

### 🔐 사용 조건
- 부모 클래스에 `virtual` 함수가 1개 이상 있어야 RTTI 정보가 생성됨.
- **포인터 타입** 변환은 실패 시 `nullptr` 반환.
- **참조 타입** 변환은 실패 시 `std::bad_cast` 예외 던짐.

### 🧪 예시: 안전한 다운캐스팅
```cpp
class GameObject {
public:
    virtual void update() {}
};

class Player : public GameObject {
public:
    void control() {}
};

GameObject* g = new Player();
Player* p = dynamic_cast<Player*>(g);
if (p) {
    p->control();  // OK
}
```

### ⚠️ 예시: 잘못된 캐스팅 감지
```cpp
GameObject* g = new GameObject();  // 실제로는 Player가 아님
Player* p = dynamic_cast<Player*>(g);
if (!p) {
    std::cout << "Casting failed!" << std::endl;  // nullptr 반환
}
```

---

## 🔒 3. `const_cast`

### ✅ 개요
- `const`, `volatile` 등의 **속성 제거 또는 추가**에 사용된다.
- **기본 타입이나 포인터 타입에서만 의미가 있음.**
- 실질적으로 변경 가능한 객체여야 하며, `const`인 객체를 변경하면 **정의되지 않은 동작(UB)** 이 발생한다.

### 🧪 예시: const 제거
```cpp
void print(char* str) {
    std::cout << str << std::endl;
}

void call(const char* cstr) {
    print(const_cast<char*>(cstr)); // const 제거
}
```

### ⚠️ 위험 예시
```cpp
const int x = 10;
int* px = const_cast<int*>(&x);
*px = 20;  // 정의되지 않은 동작 (UB), 읽기 전용 메모리 변경 시도
```

---

## 🧨 4. `reinterpret_cast`

### ✅ 개요
- **비트 수준 변환**을 수행하며, 전혀 관련 없는 타입 간 캐스팅도 허용한다.
- 타입 시스템의 보호를 우회하는 가장 강력한 캐스팅 연산자.
- **사용 시 매우 주의**해야 하며, 반드시 메모리 구조를 정확히 이해하고 사용할 것.
- 잘못 사용할 경우 **크래시, 보안 취약점, 디버깅 어려움**을 유발할 수 있음.

### 🧪 예시: 포인터 → 정수
```cpp
int* ptr = new int(42);
uintptr_t address = reinterpret_cast<uintptr_t>(ptr);
```

### 🧪 예시: 정수 → 포인터
```cpp
uintptr_t address = 0x12345678;
int* ptr = reinterpret_cast<int*>(address);
```

### 🧪 예시: 다른 타입 포인터 간 변환
```cpp
struct PacketHeader {
    int type;
    int size;
};

char* buffer = new char[sizeof(PacketHeader)];
PacketHeader* header = reinterpret_cast<PacketHeader*>(buffer);
```

---

## 📌 비교 요약

| 캐스팅 종류        | 사용 목적                       | 안전성            | 실행 시 검사 | 사용 조건                   |
|------------------|------------------------------|------------------|--------------|-----------------------------|
| `static_cast`     | 타입 원칙에 맞는 변환             | 낮음 (컴파일러만 체크) | ❌           | 다운/업캐스팅, 기본형 변환 등     |
| `dynamic_cast`    | 상속 관계에서 안전한 다운캐스팅     | 높음               | ✅           | 반드시 virtual 함수 있어야 함 |
| `const_cast`      | const 속성 제거/추가             | 낮음               | ❌           | const 제거 목적에 한정       |
| `reinterpret_cast`| 포인터/비트 수준의 강제 변환       | 매우 낮음           | ❌           | 메모리 구조 완벽히 이해 필요     |

---

## 💬 면접에서 자주 묻는 질문

1. `static_cast`와 `dynamic_cast`의 차이는?
2. `dynamic_cast`는 왜 virtual 함수가 필요할까?
3. `reinterpret_cast`는 언제 써야 하고, 왜 위험한가?
4. const 객체의 값을 변경하면 어떻게 되나?

---

## 🧠 마무리 요약

- `static_cast`: 가장 많이 쓰이며, 타입 규칙에 맞는 변환 수행.
- `dynamic_cast`: **상속 관계의 안전한 다운캐스팅**을 위한 도구.
- `const_cast`: `const` 속성 제거용. 오용 시 위험.
- `reinterpret_cast`: **비트 레벨 우회 캐스팅**. 쓰기 전에 세 번 생각할 것.

