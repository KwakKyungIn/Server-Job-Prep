# 🔄 C++ 타입 변환 (Type Conversion) - Part 1

> C++에서 타입 변환은 매우 중요한 주제로, 특히 **객체 지향 프로그래밍**, **상속**, **다형성**, **함수 오버로딩** 등에서 자주 발생합니다.  
> 이 문서에서는 타입 변환의 유형, 안전도, 암시적/명시적 구분, 상속 여부에 따른 동작 차이 등을 중심으로 설명합니다.

---

## 📌 1. 타입 변환의 유형 (비트열 재구성 여부 기준)

### 🧮 값 타입 변환 (Value Type Conversion)

- **정의**: 원본 객체와는 다른 **비트열을 재구성**하여 새 객체를 만드는 변환
- **특징**: 실제 값의 의미를 최대한 보존하려 시도함
- **대표 예시**: `int → double`, `float → int`, 사용자 정의 타입 간의 변환 등

```cpp
int score = 95;
double precise = score; // 암시적 값 타입 변환
```

---

### 🔎 참조 타입 변환 (Reference Type Conversion)

- **정의**: 객체의 **비트열은 그대로 두고**, 단지 **관점만 변경**
- **특징**: 타입만 바뀌었을 뿐, 실제 참조하는 메모리는 동일
- **대표 예시**: 포인터 타입 변환, 상속 관계 참조 변환 등

```cpp
class Unit { public: virtual void Print() {} };
class Player : public Unit {};
Player p;
Unit& u = p;  // 참조 타입 변환: Player → Unit
```

---

## ⚠️ 2. 타입 변환의 안전도 분류

### ✅ 안전한 변환 (Safe Conversion)

- **정의**: 변환 후에도 **의미가 항상 보존**됨
- **예시**: 더 큰 타입으로의 변환 (업캐스팅, upcasting)

```cpp
short s = 32000;
int i = s; // 안전한 암시적 변환
```

---

### ❌ 불안전한 변환 (Unsafe Conversion)

- **정의**: 변환 후 의미 손실이 있을 수 있음
- **예시**: 다운캐스팅, 부동소수 → 정수, 서로 다른 사용자 정의 클래스 간의 변환

```cpp
int large = 100000;
short small = large; // 데이터 손실 가능성 있음 → 불안전한 변환
```

---

## 🧭 3. 변환 방식: 암시적 vs 명시적

### 🔍 암시적 변환 (Implicit Conversion)

- 컴파일러가 자동으로 수행
- 안전하다고 판단된 변환만 허용됨

```cpp
float hp = 72;
double hpPrecise = hp; // 암시적으로 float → double 변환
```

---

### ✋ 명시적 변환 (Explicit Conversion)

- 프로그래머가 직접 의도 표현 (`static_cast`, `dynamic_cast`, `reinterpret_cast`, `const_cast`, C 스타일 캐스팅)
- 위험한 변환도 허용하므로 주의 필요

```cpp
double ratio = 1.75;
int r = static_cast<int>(ratio); // 명시적 변환 (값 손실)
```

---

## 🔄 4. 아무런 연관 없는 클래스 간 변환

### 🛑 값 타입 변환 불가 (타입 변환 생성자/연산자 예외)

```cpp
class Vector2D {
public:
    int x, y;
    Vector2D(int x, int y) : x(x), y(y) {}
};

class Position {
public:
    int x, y;
    // 타입 변환 생성자
    Position(const Vector2D& v) : x(v.x), y(v.y) {}
};

Vector2D vec(10, 20);
Position pos = vec; // 예외적으로 가능 (변환 생성자)
```

---

### 🔄 참조 타입 변환 불가

#### ❗ 연관 없는 클래스 간의 참조 타입 변환은 허용되지 않음

C++에서 상속 관계가 **전혀 없는 두 클래스 간의 참조 타입 변환은 컴파일 타임에 금지된다**.

```cpp
class A {};
class B {};

A a;
// B& b = a;  // ❌ 컴파일 에러
```

물론 `reinterpret_cast`를 이용하면 강제로 바꿀 수 있지만, 이는 **undefined behavior**를 유발하며 **절대 사용하면 안 된다**.

```cpp
B& b2 = reinterpret_cast<B&>(a);  // ❗ 위험한 코드: 사용 금지
```

→ 참조 타입 변환은 반드시 타입 간에 **상속 관계**가 있어야 안전하며 의미가 있다.


## 🧬 5. 상속 관계 클래스 간의 변환

### 5-1. 값 타입 변환

- **자식 → 부모**: 암시적 변환 가능
- **부모 → 자식**: 암시적 불가, **명시적**만 가능 (하지만 **위험**)

```cpp
class Entity { public: int hp = 100; };
class Player : public Entity { public: int level = 1; };

Player p;
Entity e = p; // ✅ 값 타입 변환 (자식 → 부모)

Entity e2;
Player p2 = static_cast<Player>(e2); // ❗ 위험, 자식 멤버 미정의
```

---

### 5-2. 참조 타입 변환

- **자식 → 부모**: 암시적 변환 가능
- **부모 → 자식**: 암시적 불가, 명시적만 가능하며 런타임 타입 확인 필요

```cpp
Player p;
Entity& eRef = p; // ✅ 자식 → 부모 (참조 타입 변환)

Player& pRef = static_cast<Player&>(eRef); // ❗ 명시적 요구 (정상)
```

→ 안전하게 처리하려면 `dynamic_cast` + `virtual` 필요

---

## 🧾 결론 요약

| 구분 | 가능 여부 | 변환 방식 | 주의사항 |
|------|-----------|------------|-----------|
| 자식 → 부모 (값) | ✅ 암시적 | 안전 |
| 부모 → 자식 (값) | ✅ 명시적만 | 위험 |
| 자식 → 부모 (참조) | ✅ 암시적 | 안전 |
| 부모 → 자식 (참조) | ✅ 명시적만 | 런타임 확인 필요 |
| 연관 없는 클래스 | ❌ 기본적으로 불가능 | 생성자/연산자 필요 |

---

## 📚 게임 서버 예시: 패킷 변환 클래스

```cpp
class RawPacket {
public:
    virtual void Parse() = 0;
};

class MovePacket : public RawPacket {
public:
    int x, y;
    void Parse() override {
        // 클라이언트로부터 받은 바이너리 데이터를 파싱
    }
};

void ProcessPacket(RawPacket* pkt) {
    if (MovePacket* move = dynamic_cast<MovePacket*>(pkt)) {
        // 안전한 참조 타입 변환 후 처리
        std::cout << "Move to: " << move->x << ", " << move->y << std::endl;
    }
}
```

---