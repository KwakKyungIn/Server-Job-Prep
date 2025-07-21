# 🧠 타입 변환 Part 2: 포인터 타입 변환과 가상 소멸자

이 문서는 C++에서 **포인터 타입 간의 변환(Casting)**, 특히 **상속 관계에 있는 클래스들 사이의 포인터 변환**과 **소멸자에 `virtual` 키워드를 붙여야 하는 이유**를 중심으로 정리한 문서입니다.  
해당 개념은 **다형성, 메모리 해제, 런타임 타입 정보(RTTI)** 등과 밀접하게 연결되며, **게임 서버 프로그래밍 면접에서 자주 출제**되는 중요한 개념입니다.

---

## 🔁 포인터 타입 변환 개요

C++에서 포인터끼리의 타입 변환은 **참조 타입 변환**(비트열은 그대로 두고 관점만 바꾸는 변환)으로 취급되며, 다음과 같은 특징을 갖습니다.

- **상속 관계**에 있는 클래스 간 변환만 허용
- **자식 → 부모 포인터 변환**은 암시적으로 가능
- **부모 → 자식 포인터 변환**은 **명시적 캐스팅**만 허용
- 실제 객체의 타입과 맞지 않는 포인터로 접근하면 **정의되지 않은 동작(undefined behavior)** 발생 가능

---

## ✅ 기본 예제: 자식 → 부모 → 자식

```cpp
class Enemy {
public:
    virtual void Attack() { std::cout << "Enemy attacks\n"; }
    virtual ~Enemy() { std::cout << "Enemy destroyed\n"; }
};

class Boss : public Enemy {
public:
    void Attack() override { std::cout << "Boss attacks harder!\n"; }
    ~Boss() override { std::cout << "Boss destroyed\n"; }
};

int main() {
    Boss* boss = new Boss();

    Enemy* enemy = boss;            // 암시적 업캐스팅 (안전)
    Boss* bossAgain = (Boss*)enemy; // 명시적 다운캐스팅 (위험할 수도 있음)

    delete enemy; // virtual 소멸자 없으면 Boss 소멸자 호출 안 됨!
}
```

### ⚠️ 문제점
- `delete enemy;`는 Enemy*로 지우지만 실제 객체는 Boss*
- **Enemy에 virtual 소멸자가 없으면 Boss의 소멸자가 호출되지 않음**
  → **리소스 누수 및 예기치 않은 동작 발생**

---

## 🧨 부모 → 자식 캐스팅의 위험

부모 클래스 포인터를 자식 클래스 포인터로 **명시적 캐스팅**(C-style, `static_cast`, `dynamic_cast`)할 경우, **객체의 실제 타입이 자식이 아닐 수도 있음**  
그 상태에서 자식 멤버에 접근하면 **UB(Undefined Behavior)** 발생

### ❌ 잘못된 예시
```cpp
Enemy* e = new Enemy();
Boss* b = (Boss*)e; // 실제로 Boss 객체가 아님
b->Attack();        // UB! Boss::Attack에 접근 시 잘못된 가상 테이블 사용 가능성
```

---

## 🔐 가상 소멸자 (`virtual ~Class()`)

### 🔸 왜 필요한가?

다형성을 이용한 포인터(`Base* ptr = new Derived();`)를 사용하면, **삭제 시에도 다형성이 적용되어야 함**

```cpp
class Base {
public:
    ~Base() { std::cout << "Base destroyed\n"; }
};

class Derived : public Base {
public:
    ~Derived() { std::cout << "Derived destroyed\n"; }
};

int main() {
    Base* ptr = new Derived();
    delete ptr; // Derived 소멸자가 호출되지 않음! (virtual 아님)
}
```

### ✅ 올바른 예시
```cpp
class Base {
public:
    virtual ~Base() { std::cout << "Base destroyed\n"; }
};

class Derived : public Base {
public:
    ~Derived() override { std::cout << "Derived destroyed\n"; }
};
```

---

## 🧭 소멸자의 virtual 여부에 따른 차이

| 조건 | 결과 |
|------|------|
| `virtual` 없음 | 부모 소멸자만 호출됨 |
| `virtual` 있음 | 부모 + 자식 소멸자 모두 호출됨 |
| 순수 가상 소멸자 (`virtual ~A() = 0`) | 추상 클래스에서도 사용 가능 |

---

## 🧪 dynamic_cast와 typeid

### `dynamic_cast`
- 다운캐스팅 시 **런타임 타입 체크** 수행
- 실패 시 `nullptr` 반환
- RTTI(런타임 타입 정보)가 필요 → `virtual` 함수가 클래스에 있어야 함

```cpp
Enemy* e = new Enemy();
Boss* b = dynamic_cast<Boss*>(e);
if (b) {
    b->Attack();
} else {
    std::cout << "다운캐스팅 실패\n";
}
```

### `typeid`
- 객체의 **실제 타입**을 확인할 수 있음
- 역시 `virtual` 함수가 클래스에 있어야 작동 정확

---

## ✅ 요약

| 구분 | 안전 여부 | 캐스팅 방식 | 설명 |
|------|-----------|-------------|------|
| 자식 → 부모 | 안전 | 암시적 | Upcasting |
| 부모 → 자식 | 위험 | 명시적 (`static_cast`, `dynamic_cast`) | 객체의 실제 타입이 자식인지 확인 필요 |
| 소멸자 | 필수 | `virtual` 필요 | 안 붙이면 자식 소멸자 호출 안됨 |

---

## 💡 실전 팁 (게임 서버 면접 대비)

- `delete Base*`는 항상 virtual 소멸자 체크
- 자식 포인터에서 부모로 암시적 변환 가능하지만, 역방향은 항상 명시적
- `dynamic_cast`는 성능 이슈로 실무에서는 제한적으로 사용
- 가상 소멸자는 **리소스 누수와 직결되므로 반드시 습관화**

---

