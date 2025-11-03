## 1. struct와 class의 차이점은 무엇인가요?
<details close>
<summary><strong>💡 자세한 정답</strong></summary>

<div style="background-color:#f6f8fa; padding:15px; border-radius:10px;">

## 💡 **struct와 class의 차이점**

### 1️⃣ 접근 제어 기본값

| 구분           | `struct` | `class`   |
| ------------ | -------- | --------- |
| 멤버 기본 접근 제어자 | `public` | `private` |
| 상속 기본 접근 제어자 | `public` | `private` |

**예시**

```cpp
struct S {
    int x; // 기본적으로 public
};

class C {
    int x; // 기본적으로 private
};
```

---

### 2️⃣ 실제 내부 구현 차이는 없음

* C++에서 `struct`와 `class`는 **완전히 동일한 문법 구조체**임.
* 단지 **기본 접근 제어자(default access specifier)** 만 다를 뿐.
* 따라서 다음 두 코드는 동일하게 작동할 수도 있음:

```cpp
struct A { private: int x; };
class B { private: int x; };
```

→ 메모리 레이아웃, 성능, 생성자 동작 등은 완전히 동일.

---

### 3️⃣ 상속 시 기본 접근 제어자

```cpp
struct Base {};
struct Derived1 : Base {}; // 기본 public 상속
class  Derived2 : Base {}; // 기본 private 상속
```

즉,

* `struct Derived : Base` → `public Base`
* `class Derived : Base` → `private Base`
  으로 해석된다.
  → 이 차이는 **상속 관계를 외부에서 노출할지 말지**에 영향을 준다.

---

### 4️⃣ 사용 의도적 관례

| 관례       | 용도                                     |
| -------- | -------------------------------------- |
| `struct` | 데이터 중심 (Plain Old Data, DTO, Packet 등) |
| `class`  | 캡슐화·추상화 중심 (엔티티, 로직 포함 객체)             |

즉, **데이터 전달용 구조체(POD)** → `struct`,
**동작을 내포한 객체지향 클래스** → `class` 사용이 일반적.

---

### 5️⃣ [추가 포인트: C 구조체와의 차이]

* C의 `struct`는 함수 멤버, 접근 제어자, 상속, 생성자 등을 가질 수 없음.
* C++의 `struct`는 클래스와 동일하게 **멤버 함수, 생성자/소멸자, 상속, 접근 제어자** 모두 가능.

```cpp
struct Player {
    int hp;
    void Attack() { hp -= 10; } // 가능!
};
```

---

## 🎯 **면접용 정리 답변**

> C++에서 `struct`와 `class`는 내부적으로 동일한 개념이지만, 기본 접근 제어자가 다릅니다.
> `struct`는 멤버와 상속이 기본적으로 `public`, `class`는 `private`입니다.
> 보통 데이터 중심의 단순 구조체엔 `struct`, 캡슐화된 로직을 가진 객체엔 `class`를 사용합니다.
> 즉, 문법적 차이보다 **의도적 구분**과 **설계 컨벤션**의 차이로 보는 게 정확합니다.


</details>


## 2. 포인터와 참조의 차이점은?

<details close>
<summary><strong>💡 자세한 정답</strong></summary>

<div style="background-color:#f6f8fa; padding:15px; border-radius:10px;">

## 💡 **포인터(pointer)** vs **참조(reference)**

| 구분                | 포인터 (Pointer)                    | 참조 (Reference)                 |
| ----------------- | -------------------------------- | ------------------------------ |
| **null 가능 여부**    | `nullptr` 가능                     | 반드시 유효한 객체 참조 (null 불가)        |
| **재할당 가능 여부**     | 다른 주소로 변경 가능                     | 한 번 바인딩되면 대상 변경 불가             |
| **연산 가능 여부**      | `++`, `--`, `+`, `-` 등 포인터 연산 가능 | 불가능                            |
| **간접 접근 방식**      | `*ptr`, `ptr->` 사용               | 자동 역참조 (그냥 변수처럼 사용)            |
| **메모리 저장 형태**     | 주소 값을 저장하는 변수                    | 별도 저장 공간 없음(컴파일러가 단순히 별칭으로 취급) |
| **초기화 필요성**       | 선언 후 나중에 초기화 가능                  | 선언 시 반드시 초기화 필요                |
| **nullptr 여부 검사** | `if (ptr)` 가능                    | 검사 불필요 (항상 유효하다고 가정)           |
| **sizeof 결과**     | 플랫폼 주소 크기(4바이트 or 8바이트)          | 참조 대상 타입의 크기                   |
| **배열 표현 가능**      | 포인터 배열 가능                        | 참조 배열은 불가능                     |

---

### 📘 예시 코드

```cpp
int a = 10;
int b = 20;

// 포인터
int* p = &a;
p = &b;      // 재할당 가능
*p = 30;     // b = 30
p = nullptr; // 가능

// 참조
int& r = a;  
r = 30;      // a = 30
// r = &b;   // ❌ 불가능 (재할당 안 됨)
// &r = &b;  // ❌ 불가능 (참조는 별칭임)
```

---

### ⚙️ 동작 원리 차이

* **포인터**: 실제로 “주소값을 담는 변수”.
* **참조**: **이미 존재하는 객체의 별칭(alias)**. 별도 주소 변수로 존재하지 않음.
  → 단, 컴파일러는 내부적으로 포인터처럼 구현할 수도 있지만, 언어적으론 “별칭”이다.

---

### ⚠️ 주의점

1. 참조는 반드시 초기화해야 함:

   ```cpp
   int& ref; // ❌ 불가능
   ```
2. 참조는 null을 가질 수 없으므로, **참조가 유효한 대상인지 검사 불가.**
   (참조가 깨지면 UB — Undefined Behavior)
3. 참조는 선언 시점부터 lifetime이 끝날 때까지 **항상 동일한 객체를 가리킴**.

---

### 🎯 **면접용 요약 답변**

> 포인터는 “주소를 저장하는 변수”고, 참조는 “객체의 또 다른 이름”입니다.
> 포인터는 `nullptr`이 될 수 있고 다른 객체로 재할당도 가능하지만,
> 참조는 한 번 바인딩되면 바꿀 수 없고 항상 유효한 객체를 가리킵니다.
> 또한 포인터는 산술 연산이 가능하지만, 참조는 변수처럼만 사용할 수 있습니다.

---

### 🔥 꼬리질문 예시 (면접관 심화)

1. “참조를 멤버 변수로 가질 때 주의할 점은?”
   → 반드시 초기화해야 하고, 복사/대입 시 참조 불변 특성 때문에 의도치 않은 동작 가능.
2. “포인터와 참조 중 함수 매개변수에 어떤 걸 써야 하나요?”
   → 참조는 null 검사를 못 하므로, 선택은 **의도(옵션성 vs 필수성)** 기준으로 결정.
   → 예: 선택적 전달 → 포인터 / 반드시 존재해야 함 → 참조.

</details>


## 3. new/delete와 malloc/free의 차이는 무엇인가요?

<details close>
<summary><strong>💡 자세한 정답</strong></summary>

<div style="background-color:#f6f8fa; padding:15px; border-radius:10px;">

## 💡 **new/delete** vs **malloc/free**

| 구분                        | `new / delete` (C++)                            | `malloc / free` (C)          |
| ------------------------- | ----------------------------------------------- | ---------------------------- |
| **언어 차원**                 | C++ 연산자 (operator)                              | C 표준 라이브러리 함수                |
| **생성자/소멸자 호출 여부**         | ✅ 호출함 (객체 초기화/정리 자동 수행)                         | ❌ 호출하지 않음 (단순 메모리 블록만 할당/해제) |
| **리턴 타입**                 | 지정한 타입의 포인터 (타입 안정적)                            | `void*` (형 변환 필요)            |
| **오버로딩 가능 여부**            | ✅ 가능 (`operator new/delete` 재정의 가능)             | ❌ 불가능 (함수 고정)                |
| **할당 실패 시 동작**            | 예외(`std::bad_alloc`) 발생                         | `NULL` 반환                    |
| **배열 할당 지원**              | `new[]`, `delete[]` 지원                          | 직접 크기 계산해야 함                 |
| **플레이스먼트(placement) new** | ✅ 지원                                            | ❌ 지원 안 함                     |
| **사용 범위**                 | 객체 지향 코드, RAII 기반                               | 단순 C 호환 코드, 저수준 버퍼 관리용       |
| **내부 구현 위치**              | 런타임 연산자 (`operator new`) → 결국 `malloc` 기반일 수 있음 | CRT 힙 관리 함수                  |

---

### 📘 예시

```cpp
#include <cstdlib>
#include <new>
#include <iostream>

struct Player {
    Player()  { std::cout << "Constructed\n"; }
    ~Player() { std::cout << "Destructed\n"; }
};

// ✅ new/delete
Player* p1 = new Player();  // 생성자 호출됨
delete p1;                  // 소멸자 호출됨

// ⚠️ malloc/free
Player* p2 = (Player*)malloc(sizeof(Player)); // 단순 메모리만 확보
free(p2);                                     // 소멸자 호출 안 됨
```

**출력 결과:**

```
Constructed
Destructed
```

→ `malloc/free` 버전에서는 아무것도 출력되지 않는다. 생성자/소멸자가 아예 호출되지 않기 때문.

---

### ⚙️ 내부 동작 흐름

```cpp
// new 연산자 내부적으로
void* mem = operator new(sizeof(Player)); // 힙 할당
::new(mem) Player(); // placement new → 생성자 호출

// delete 연산자 내부적으로
p1->~Player(); // 소멸자 호출
operator delete(p1); // 메모리 해제
```

즉, **new는 malloc + 생성자 호출**,
**delete는 소멸자 호출 + free**의 조합으로 생각하면 된다.

---

### ⚠️ 혼용 금지

```cpp
Player* p = (Player*)malloc(sizeof(Player));
delete p; // ❌ UB (malloc/free로 잡은 메모리는 delete 금지)
```

→ 반드시 **짝지어 사용해야 함**

* `new` ↔ `delete`
* `new[]` ↔ `delete[]`
* `malloc` ↔ `free`

---

### 🎯 **면접용 정리 답변**

> `new/delete`는 C++의 연산자로서 **생성자와 소멸자를 자동 호출**해 객체 수명까지 관리합니다.
> 반면 `malloc/free`는 단순히 **바이트 단위 메모리 블록만 확보/해제**하며, 생성자/소멸자를 호출하지 않습니다.
> 또한 `new`는 타입 안정성을 제공하고 오버로딩이 가능하지만, `malloc`은 `void*`을 반환하며 재정의가 불가능합니다.
> 결국 `new/delete`는 객체 지향 메모리 관리용, `malloc/free`는 저수준 버퍼용으로 쓰입니다.

---

### 🔥 꼬리질문 예상

1. “`new`가 내부적으로 malloc을 쓰나요?”
   → 구현에 따라 다름. 대부분의 런타임에서 `operator new`가 `malloc`을 호출하지만, 반드시 그런 건 아님.
2. “placement new는 뭐죠?”
   → 이미 확보된 메모리에 직접 객체를 생성할 때 쓰는 문법. 예: 커스텀 메모리 풀.

   ```cpp
   void* mem = malloc(sizeof(Player));
   Player* p = new (mem) Player(); // 생성자 호출
   ```



</details>


## 4. const 키워드의 의미를 설명해보세요.

<details close>
<summary><strong>💡 자세한 정답</strong></summary>

<div style="background-color:#f6f8fa; padding:15px; border-radius:10px;">

## 💡 `const` 키워드 — 위치에 따라 달라지는 의미

---

### 1️⃣ **일반 변수에서의 const**

> **값을 변경할 수 없는 불변 변수 (read-only variable)**

```cpp
const int x = 10;  
x = 20; // ❌ 컴파일 에러
```

* **초기화 시 반드시 값 지정** 필요.
* 메모리에는 저장되지만 수정 불가.
* 의미: "이 이름을 통해 값 변경을 허용하지 않겠다."

---

### 2️⃣ **포인터에서의 const**

**별(*)의 좌우 위치에 따라 의미가 완전히 달라진다.**

| 선언                   | 의미                  | 해석                |
| -------------------- | ------------------- | ----------------- |
| `const int* p`       | **포인터가 가리키는 값이 상수** | “읽기 전용 데이터 가리킴”   |
| `int* const p`       | **포인터 자체가 상수**      | “다른 주소를 가리킬 수 없음” |
| `const int* const p` | **포인터와 값 모두 상수**    | 완전 불변             |

**예시:**

```cpp
int a = 10, b = 20;
const int* p1 = &a; // *p1 = 30 ❌, p1 = &b ✅
int* const p2 = &a; // *p2 = 30 ✅, p2 = &b ❌
const int* const p3 = &a; // 둘 다 ❌
```

---

### 3️⃣ **함수 매개변수에서의 const**

> **함수 안에서 인자를 수정하지 않겠다는 약속**

```cpp
void PrintName(const std::string& name) {
    // name.append("X"); ❌
    std::cout << name;
}
```

* `const &`를 자주 쓰는 이유:

  * **복사 비용 절감 (참조)**
  * **데이터 보호 (const)**
    → 읽기 전용 참조(reference to const)

---

### 4️⃣ **함수 반환값에서의 const**

> **반환된 값을 수정하지 못하게 보호**

```cpp
const std::string GetName() {
    return "Kwak";
}
```

* `GetName() = "Kim";` 이런 문장을 방지.
* 특히 참조 반환 시 안전성 보장:

  ```cpp
  const int& GetScore() { return _score; }
  ```

  → 호출자가 `_score`를 수정 불가.

---

### 5️⃣ **클래스 멤버 함수에서의 const**

> **객체의 상태를 변경하지 않는 멤버 함수**

```cpp
class Player {
    int hp = 100;
public:
    int GetHp() const { return hp; } // ✅ 읽기 전용
    void Damage(int d) { hp -= d; }  // ❌ const 불가
};
```

* `const`가 붙으면 **멤버 변수 수정 금지**, **비-const 멤버 함수 호출 불가**
* 호출자 입장에서도 차이 있음:

  ```cpp
  const Player p;
  p.GetHp();  // ✅
  p.Damage(); // ❌
  ```

→ 즉, **객체를 보호하는 “읽기 전용 인터페이스”를 보장**한다.

---

### 6️⃣ **멤버 변수 앞 const (클래스 내부 상수)**

> **객체마다 동일한 불변 데이터**

```cpp
class Player {
    const int maxHp = 100; // 생성자 초기화 리스트 필요 시도 있음
};
```

* 반드시 **생성자 초기화 리스트로 초기화**해야 함.

  ```cpp
  Player() : maxHp(100) {}
  ```

---

### 7️⃣ **매크로 vs const**

```cpp
#define MAX_HP 100       // 전처리기, 타입 없음
const int MaxHp = 100;   // 타입 안전, 스코프 있음
```

→ C++에서는 **매크로보다 const 상수**를 선호.

---

## 🎯 **면접용 답변**

> `const`는 “읽기 전용 보장”을 의미하지만, 위치에 따라 의미가 달라집니다.
> 변수 앞에 오면 값 변경을 막고,
> `const int* p`처럼 포인터 왼쪽에 오면 **가리키는 값이 상수**,
> `int* const p`처럼 오른쪽에 오면 **포인터 자체가 상수**가 됩니다.
>
> 함수 인자에서는 “읽기 전용 참조”,
> 멤버 함수에서는 “객체 상태를 바꾸지 않는 함수”를 의미합니다.
> 즉, const는 **타입 안전성과 불변성을 명시적으로 보장하는 장치**입니다.

---

### 🔥 꼬리질문 예상

1. “`mutable` 키워드는 언제 쓰나요?”
   → `const` 함수 내에서도 수정 가능한 멤버 변수에 사용. (예: 캐싱, 통계 카운트)

   ```cpp
   mutable int cacheHits;
   ```
2. “const와 constexpr의 차이는요?”
   → `const`는 런타임 상수, `constexpr`은 **컴파일타임 상수**.
3. “포인터에 const를 잘못 써서 버그 난 경험 있나요?”
   → 메모리 풀, 네트워크 패킷 등에서 `const` 버퍼를 잘못 캐스팅했을 때 undefined behavior 발생 가능.

</details>


## 5. mutable / constexpr / consteval를 비교하고, 설명해주세요.

<details close>
<summary><strong>💡 자세한 정답</strong></summary>

<div style="background-color:#f6f8fa; padding:15px; border-radius:10px;">

## 💡 `const` / `constexpr` / `consteval` / `mutable` 비교 요약

| 키워드         | 평가 시점               | 의미                  | 사용 위치                 | 대표 예시                                           |
| :---------- | :------------------ | :------------------ | :-------------------- | :---------------------------------------------- |
| `const`     | 런타임                 | 변경 불가 변수            | 변수, 포인터, 함수 인자, 멤버 함수 | `const int hp = 100;`                           |
| `constexpr` | 컴파일 타임              | 컴파일 시 계산 가능한 상수     | 변수, 함수, 생성자           | `constexpr int MaxHp() { return 100; }`         |
| `consteval` | 컴파일 타임 **즉시 평가 필수** | 반드시 컴파일 타임에 실행      | 함수                    | `consteval int Square(int x) { return x * x; }` |
| `mutable`   | 런타임                 | const 함수 내에서도 수정 허용 | 클래스 멤버 변수             | `mutable int cacheHits;`                        |

---

## ⚙️ **1️⃣ const — “값을 바꿀 수 없는 변수”**

* 런타임에서 상수로 취급.
* 컴파일러가 **최적화 힌트로만 사용하는 read-only 제약.**
* 단, **컴파일 타임 상수**는 아님.

```cpp
const int a = 10;
int b = a + 5; // 런타임 시 계산됨
```

---

## ⚙️ **2️⃣ constexpr — “컴파일 시점 상수”**

> **컴파일 타임에 값이 결정되어야 함.**
> 단순히 “불변”이 아니라 “미리 계산 가능한 값”.

```cpp
constexpr int Square(int x) { return x * x; }

constexpr int result = Square(5); // ✅ 컴파일 타임 계산
int runtime = Square(rand());     // ⚠️ 런타임 계산 (rand()는 constexpr 아님)
```

**조건:**

* 함수 내부에 **컴파일타임 평가 가능한 표현식**만 포함되어야 함.
* 모든 인자가 컴파일 타임 상수라면, 결과도 컴파일 타임 상수.

```cpp
struct Vec {
    int x, y;
    constexpr Vec(int a, int b) : x(a), y(b) {}
};
constexpr Vec v(1, 2); // ✅ 가능
```

---

## ⚙️ **3️⃣ consteval — “무조건 컴파일 시 실행”**

> **C++20에서 추가됨.**
> “이 함수는 반드시 컴파일타임에 평가돼야 한다.”
> 즉, 런타임 호출이 불가능하다.

```cpp
consteval int Square(int x) { return x * x; }

constexpr int a = Square(10); // ✅ 컴파일타임 계산
int b = Square(rand());       // ❌ 오류: rand()는 컴파일 시 알 수 없음
```

💬 **비유하자면:**

* `constexpr`: 컴파일 타임 평가 *가능하지만* 런타임 호출도 허용
* `consteval`: 컴파일 타임 *강제 평가 (런타임 호출 불가)*

---

## ⚙️ **4️⃣ mutable — “const 함수에서도 바꿀 수 있다”**

> **const 멤버 함수 내에서도 변경 허용되는 멤버 변수.**

```cpp
class Player {
    mutable int accessCount = 0;
    int hp = 100;
public:
    int GetHp() const {
        ++accessCount; // ✅ 가능 (mutable)
        return hp;     // ⚠️ hp는 수정 불가
    }
};
```

* `mutable`은 **const correctness 예외 장치**야.
  → 논리적으론 불변이지만, 통계나 캐시같은 보조 데이터는 수정할 필요가 있을 때 사용.
* 서버 코드에서도 자주 써.
  예: Room::GetPlayerCount()에서 “호출 횟수 로그”를 찍을 때 `mutable` counter 사용.

---

## ⚖️ **5️⃣ 세 가지 const 계열 비교 요약**

| 구분               | `const`  | `constexpr`  | `consteval`    |
| ---------------- | -------- | ------------ | -------------- |
| **의미**           | 값 변경 불가  | 컴파일 타임 계산 가능 | 컴파일 타임에 반드시 계산 |
| **평가 시점**        | 런타임      | 컴파일타임 (가능)   | 컴파일타임 (강제)     |
| **적용 대상**        | 변수/함수/멤버 | 변수/함수/생성자    | 함수만            |
| **런타임 호출 가능 여부** | 가능       | 가능           | 불가능            |
| **목적**           | 불변성 보장   | 컴파일 타임 상수화   | 컴파일 타임 강제 실행   |

---

## 🎯 **면접용 답변**

> `const`는 값을 바꾸지 못하게 막는 런타임 제약이고,
> `constexpr`은 **컴파일 시점에서 계산할 수 있는 상수**,
> `consteval`은 **컴파일 타임에 반드시 계산되어야 하는 함수**입니다.
> 반대로 `mutable`은 `const` 함수 안에서도 수정할 수 있게 하는 예외 키워드입니다.
>
> 정리하면,
>
> * `const`는 "읽기 전용",
> * `constexpr`은 "컴파일 타임 계산 가능",
> * `consteval`은 "컴파일 타임 계산 강제",
> * `mutable`은 "const 예외 허용"입니다.

---

## 🔥 꼬리질문 예상

1. **"constexpr 함수가 런타임에 실행될 수도 있나요?"**
   → 네, 인자가 런타임 값이면 런타임에 계산됩니다.
2. **"constexpr과 inline의 차이점은?"**
   → inline은 코드 복사 최적화, constexpr은 컴파일 타임 상수 평가. 목적이 완전히 다름.
3. **"mutable을 남용하면 뭐가 문제인가요?"**
   → 논리적 불변성(logical constness)을 깨뜨려 코드 신뢰성을 떨어뜨림.
   즉, “논리적으로 바뀌지 않는 객체만 const 함수로 보장하라.”



</details>


## 6. 함수 오버로딩(Overloading)과 오버라이딩(Overriding)의 차이점은?

<details close>
<summary><strong>💡 자세한 정답</strong></summary>

<div style="background-color:#f6f8fa; padding:15px; border-radius:10px;">


## 💡 함수 오버로딩(Overloading) vs 오버라이딩(Overriding)

| 구분              | **오버로딩 (Overloading)** | **오버라이딩 (Overriding)**               |
| --------------- | ---------------------- | ------------------------------------ |
| **관계**          | 같은 클래스 내에서             | 상속 관계(부모–자식 클래스)                     |
| **목적**          | 같은 이름으로 다른 인자 처리       | 부모 클래스 기능 재정의                        |
| **조건**          | 함수 이름 동일, 매개변수 시그니처 다름 | 함수 이름/매개변수/리턴타입 모두 동일 + `virtual` 필요 |
| **키워드 필요 여부**   | 없음                     | `virtual` / `override` 필요            |
| **바인딩 시점**      | 컴파일 타임 (정적 바인딩)        | 런타임 (동적 바인딩, vtable)                 |
| **리턴 타입 영향 여부** | 불가능 (리턴 타입만 다르면 에러)    | 가능 (공변 반환 가능, covariant return)      |
| **접근 위치**       | 같은 클래스 내부              | 파생 클래스에서 부모 클래스 상속 후 재정의             |
| **대표 사용 예시**    | 함수 다중 정의               | 다형성(Polymorphism) 구현                 |

---

## ⚙️ 1️⃣ **오버로딩 (Overloading)**

> **같은 이름, 다른 매개변수 시그니처**

* 컴파일러가 **함수 시그니처(Signature)**를 보고 구분.
* “정적 다형성(static polymorphism)”이라 불림.

```cpp
void Print(int n)    { std::cout << "int: " << n << "\n"; }
void Print(double d) { std::cout << "double: " << d << "\n"; }
void Print(std::string s) { std::cout << "string: " << s << "\n"; }

Print(10);       // Print(int)
Print(3.14);     // Print(double)
Print("Kwak");   // Print(std::string)
```

📍 **주의점**:
리턴 타입만 다르고 매개변수 동일하면 **컴파일 에러**

```cpp
int   Foo();  
float Foo(); // ❌ 불가능
```

---

## ⚙️ 2️⃣ **오버라이딩 (Overriding)**

> **상속 관계에서 부모의 함수를 자식이 재정의**

* `virtual` 함수만 오버라이드 가능.
* 런타임에 **가상 테이블(vtable)**을 통해 호출 대상이 결정됨.
* “동적 다형성(dynamic polymorphism)”의 핵심.

```cpp
class Player {
public:
    virtual void Attack() { std::cout << "Basic attack\n"; }
};

class Warrior : public Player {
public:
    void Attack() override { std::cout << "Sword slash\n"; }
};

Player* p = new Warrior();
p->Attack();  // ✅ "Sword slash" (런타임 결정)
```

📍 **핵심 포인트**

* `virtual`이 없다면 단순한 **함수 숨김(name hiding)**이 된다.
* `override` 키워드를 쓰면 **실수 방지** (부모 함수와 정확히 매칭 안 되면 컴파일 에러).

---

## ⚙️ 3️⃣ **Binding 차이 (면접관이 진짜 듣고 싶은 부분)**

| 구분        | 정적 바인딩 (Static Binding) | 동적 바인딩 (Dynamic Binding) |
| --------- | ----------------------- | ------------------------ |
| **대표**    | 오버로딩                    | 오버라이딩                    |
| **결정 시점** | 컴파일 타임                  | 런타임                      |
| **실행 속도** | 빠름                      | 약간 느림 (vtable lookup)    |
| **메커니즘**  | 함수 시그니처 매칭              | vtable 통해 실제 객체 타입 참조    |

---

## ⚙️ 4️⃣ **Covariant Return Type (공변 반환형)**

> 오버라이딩 시 **리턴 타입이 부모보다 더 구체적인 타입**으로 변경 가능

```cpp
class Base {
public:
    virtual Base* Clone() const { return new Base(*this); }
};

class Derived : public Base {
public:
    Derived* Clone() const override { return new Derived(*this); } // ✅ OK
};
```

→ 리턴 타입이 “공변적(covariant)”이면 허용됨.

---

## ⚙️ 5️⃣ **오버로딩 + 오버라이딩 동시에 존재할 때**

> 이름이 같을 때 헷갈리는 상황

```cpp
class Base {
public:
    virtual void Show(int n) { std::cout << "Base int\n"; }
};

class Derived : public Base {
public:
    void Show(double d) { std::cout << "Derived double\n"; } // ❗ Base::Show 숨김(name hiding)
};

Derived d;
d.Show(10); // "Derived double" 호출 (Base 버전 숨겨짐)
```

➡ `using Base::Show;`를 추가하면 부모 버전 복구 가능.

---

## 🎯 **면접용 요약 답변**

> 오버로딩은 **같은 이름, 다른 매개변수로 컴파일 시점에 구분되는 정적 다형성**,
> 오버라이딩은 **상속 관계에서 virtual 함수를 재정의해 런타임에 호출 대상을 결정하는 동적 다형성**입니다.
>
> 즉, **오버로딩은 컴파일러가 선택하고**,
> **오버라이딩은 런타임에 실제 객체 타입이 선택합니다.**

---

## 🔥 꼬리질문 예상

1. “오버라이딩할 때 override 키워드 왜 써야 하나요?”
   → 실수 방지. 함수 시그니처가 안 맞으면 컴파일 에러로 바로 잡음.
2. “virtual을 안 붙이면 어떻게 되나요?”
   → 단순한 이름 숨김(name hiding) 발생, 다형성 깨짐.
3. “vtable이 뭐예요?”
   → 런타임에 어떤 함수가 호출될지 저장해둔 **가상 함수 테이블**. 각 객체는 vptr(포인터)을 통해 vtable 참조.
4. “오버로딩 해석 순서는?”
   → 컴파일러가 정확도 높은 매칭 → 표준 변환 → 사용자 정의 변환 순으로 탐색.

</details>


## 7. inline 함수란 무엇이고, 언제 사용하나요?

<details close>
<summary><strong>💡 자세한 정답</strong></summary>

<div style="background-color:#f6f8fa; padding:15px; border-radius:10px;">

## 💡 **inline 함수란?**

> **함수 호출 코드를 호출부에 그대로 삽입하는 함수**
> — 즉, 호출 오버헤드(call overhead)를 줄이기 위한 **컴파일러 힌트(keyword)**

---

### ✅ **핵심 동작 원리**

일반 함수 호출:

```cpp
int Add(int a, int b) {
    return a + b;
}

int main() {
    int result = Add(1, 2); // [1] 스택 인자 푸시 → [2] 함수 호출 → [3] 복귀
}
```

→ 호출 시 스택 프레임 구성, 인자 복사, 리턴 처리 등 **런타임 오버헤드 발생**

---

**inline 함수:**

```cpp
inline int Add(int a, int b) {
    return a + b;
}

int main() {
    int result = 1 + 2; // 컴파일러가 호출부에 함수 본문을 직접 삽입
}
```

→ **함수 호출 과정이 사라지고, 코드가 복사되어 들어감.**
→ **컴파일 타임 대체(substitution)**

---

## ⚙️ **inline의 진짜 의미**

* **“함수를 인라인화하라”는 *요청(hint)***이지, **보장이 아님**.
  → 최종 인라인 여부는 **컴파일러가 판단**한다.
  → 함수가 너무 크거나, 루프가 복잡하면 무시될 수 있음.

---

## 🧩 **사용 목적**

1. **호출 오버헤드 제거**

   * 매우 짧고, 자주 호출되는 함수에서 성능 향상
   * 예: `GetHp()`, `IsDead()` 같은 1~2줄짜리 getter 함수

2. **헤더 정의 시 중복 링크 방지**

   * 여러 cpp에서 동일한 함수를 include할 때 **One Definition Rule (ODR)** 위반 방지

   ```cpp
   // Util.h
   inline int Clamp(int v, int min, int max) {
       return (v < min) ? min : (v > max ? max : v);
   }
   ```

   → inline 없으면 링커에서 “중복 정의 에러”

3. **매크로 대체**

   * 매크로보다 안전한, 타입 체크 가능한 함수 대체 수단

   ```cpp
   #define SQUARE(x) ((x)*(x))     // 부작용 위험
   inline int Square(int x) { return x * x; } // 타입 안전
   ```

---

## ⚠️ **주의점 (면접에서 꼭 나오는 부분)**

| 문제                      | 설명                                               |
| ----------------------- | ------------------------------------------------ |
| **코드 부풀음 (Code bloat)** | 짧은 함수라도 반복 호출이 많으면 함수 본문이 계속 복사되어 **바이너리 크기 증가** |
| **디버깅 어려움**             | 스택 트레이스에서 함수 경계가 사라짐                             |
| **재귀함수 비적합**            | 인라인 확장이 불가능하거나 비효율적                              |
| **컴파일러 판단 우선**          | 인라인 지시어 무시될 수 있음 (`-O2` 이상 최적화 시 자동 인라인화도 발생)    |

---

## 🔥 **현대 C++ 관점**

* `inline`은 **성능 최적화 키워드라기보다 “ODR 중복 방지용”** 으로 더 자주 쓰임.
* 최적화는 대부분 **컴파일러의 자동 인라이닝(optimizer)**이 더 잘 처리함.
* 따라서 직접 inline을 쓰는 경우는 보통:

  1. 헤더에 정의해야 하는 **짧은 유틸 함수**
  2. 템플릿 함수 정의부 (`template <typename T> inline T Add(T a, T b) {...}`)

---

## 🎯 **면접용 정리**

> `inline`은 컴파일러에게 “이 함수의 호출 코드를 본문으로 대체하라”는 힌트를 주는 키워드입니다.
> 호출 오버헤드를 줄일 수 있지만, 함수가 크면 오히려 코드가 부풀고 캐시 효율이 떨어질 수 있습니다.
>
> 현대 C++에서는 성능보다는 **헤더 중복 정의 방지**, **간단한 유틸리티 함수 정의** 용도로 더 많이 사용합니다.

---

## 🔥 꼬리질문 예상

1. “inline 함수가 헤더에 있어야 하는 이유는?”
   → 여러 cpp에 포함돼도 **ODR(One Definition Rule)** 위반을 피하기 위해.
2. “inline과 매크로의 차이점은?”
   → 매크로는 단순 텍스트 치환(타입 체크 불가), inline은 **진짜 함수**(타입 체크·디버깅 가능).
3. “컴파일러가 자동으로 inline하는 경우도 있나요?”
   → 네, `-O2`, `-O3` 같은 최적화 옵션에서 **컴파일러가 판단하여 자동 인라이닝**함.


</details>


## 8. C++에서 함수 포인터란 무엇인가요?

<details close>
<summary><strong>💡 자세한 정답</strong></summary>

<div style="background-color:#f6f8fa; padding:15px; border-radius:10px;">


## 💡 **함수 포인터(Function Pointer)란?**

> “**함수의 주소를 저장하는 포인터**”
> 즉, **함수 자체를 값처럼 넘기거나, 나중에 호출할 수 있게 하는 변수**다.

---

### ⚙️ 1️⃣ 함수의 메모리 구조

```cpp
int Add(int a, int b) { return a + b; }
```

이 함수는 **메모리에 코드 영역(Code Segment)**에 저장된다.
→ “함수 이름”은 곧 “함수의 시작 주소”.

```cpp
int (*funcPtr)(int, int) = Add; // Add 함수의 주소 저장
int result = funcPtr(2, 3);     // 포인터를 통한 호출
```

결국 `Add(2,3)`과 `(*funcPtr)(2,3)`은 **완전히 동일한 동작**이다.

---

### ⚙️ 2️⃣ 기본 문법

| 선언 형태                   | 의미                            |
| ----------------------- | ----------------------------- |
| `int (*f)(int, int);`   | int(int, int) 함수 주소를 저장하는 포인터 |
| `f = &Add;`             | 함수 주소 대입 ( & 생략 가능 )          |
| `(*f)(3,4)` 또는 `f(3,4)` | 호출 (둘 다 동일)                   |

```cpp
#include <iostream>
int Add(int a, int b) { return a + b; }
int Sub(int a, int b) { return a - b; }

int main() {
    int (*op)(int, int); // 함수 포인터 선언
    op = Add;
    std::cout << op(5, 3) << '\n'; // 8

    op = Sub;
    std::cout << op(5, 3) << '\n'; // 2
}
```

---

### ⚙️ 3️⃣ 배열/테이블과 함께 쓰면 강력하다

> 조건문 없이 **함수 포인터 테이블로 분기** 가능 — 서버 코드에서 자주 씀.

```cpp
int Add(int a, int b) { return a + b; }
int Sub(int a, int b) { return a - b; }
int Mul(int a, int b) { return a * b; }

int main() {
    int (*ops[3])(int, int) = { Add, Sub, Mul };
    int a = 5, b = 2;

    for (int i = 0; i < 3; i++)
        std::cout << ops[i](a, b) << '\n';
}
```

→ switch 없이 **동적 실행 테이블**을 구성할 수 있다.
→ 게임 서버에서 `PacketHandlerTable[]`로 이런 구조 자주 쓰인다.

---

### ⚙️ 4️⃣ 함수 포인터 매개변수

> 다른 함수를 **인자로 전달하는 콜백(callback)** 구조를 만들 수 있다.

```cpp
void Process(int a, int b, int (*callback)(int, int)) {
    std::cout << "Result: " << callback(a, b) << '\n';
}

int Add(int a, int b) { return a + b; }
int Mul(int a, int b) { return a * b; }

int main() {
    Process(3, 4, Add);
    Process(3, 4, Mul);
}
```

📌 콜백 기반 설계는
→ “이벤트 루프”, “패킷 처리”, “JobQueue 콜백” 등에서 필수적으로 사용됨.

---

### ⚙️ 5️⃣ 멤버 함수 포인터 (살짝 심화)

클래스 멤버는 `this` 포인터를 포함하므로 **일반 함수 포인터와 다르다.**

```cpp
class Player {
public:
    void Attack(int dmg) { std::cout << "Damage: " << dmg << '\n'; }
};

int main() {
    void (Player::*fptr)(int) = &Player::Attack; // 멤버 함수 포인터
    Player p;
    (p.*fptr)(100); // 호출 시 반드시 객체 필요
}
```

| 구분        | 선언 형태               | 호출 방식          |
| --------- | ------------------- | -------------- |
| 일반 함수 포인터 | `int (*f)(int)`     | `f(10)`        |
| 멤버 함수 포인터 | `void (C::*f)(int)` | `(obj.*f)(10)` |

---

### ⚙️ 6️⃣ Modern C++에서는 std::function / 람다로 대체 가능

> 함수 포인터보다 유연한 “함수 래퍼(Function Wrapper)”.

```cpp
#include <functional>

void Hello() { std::cout << "Hi\n"; }

int main() {
    std::function<void()> f = Hello;
    f(); // 호출 가능

    f = [] { std::cout << "Lambda!\n"; };
    f(); // 람다 대입도 가능
}
```

💥 장점:

* 일반 함수, 멤버 함수, 람다 모두 호환.
* 인자 개수, 캡처 처리 유연.
* 서버 설계 시 “JobQueue” 같은 비동기 시스템에서 **콜백 저장용 타입**으로 자주 사용됨.

---

## 🎯 **면접용 요약**

> 함수 포인터는 “함수의 주소를 저장하고, 나중에 호출할 수 있는 포인터”입니다.
> 호출 오버헤드를 줄이거나, 실행 함수를 동적으로 바꿔야 할 때 사용합니다.
>
> 예를 들어 서버에서 패킷 ID에 따라 함수 포인터 테이블을 만들어
> `gPacketHandler[id](session, buffer);` 형태로 처리하면,
> 조건문 없이 빠르고 깔끔한 분기 구조를 만들 수 있습니다.
>
> 현대 C++에서는 `std::function`이나 람다가 함수 포인터의 안전한 대체재로 쓰입니다.

---

### 🔥 꼬리질문 예상

1. “함수 포인터와 `std::function`의 차이점은?”
   → `std::function`은 일반 함수뿐 아니라 **람다, 멤버 함수, functor**까지 담을 수 있음.
2. “함수 포인터를 이용한 콜백 구조를 실제로 써본 적 있나요?”
   → IOCP 서버의 `DispatchPacket()` 같은 곳에서 PacketHandler 테이블로 활용.
3. “멤버 함수 포인터 호출 시 주의할 점은?”
   → 반드시 객체 인스턴스(`this`) 필요, 시그니처 다르면 호출 불가.


</details>


## 9. 참조자 반환 시 주의할 점은?

<details close>
<summary><strong>💡 자세한 정답</strong></summary>

<div style="background-color:#f6f8fa; padding:15px; border-radius:10px;">

## 💡 **참조자 반환 시 주의할 점**

> **지역 변수(Local Variable)** 를 참조로 반환하면 **dangling reference (매달린 참조)** 가 발생한다.
> → 반환 시점에 객체가 소멸되기 때문에, 그 주소를 참조하면 **정의되지 않은 동작(UB)** 이다.

---

### ⚙️ 1️⃣ 잘못된 예시 (❌ 위험)

```cpp
int& GetValue() {
    int x = 10;     // 지역 변수 (스택에 존재)
    return x;       // ❌ 함수 끝나면 스택에서 사라짐
}

int main() {
    int& ref = GetValue(); // ref는 이미 해제된 메모리를 참조
    std::cout << ref;      // UB (쓰레기값 or 크래시)
}
```

📉 이건 전형적인 **dangling reference** 문제.
스택 프레임이 pop되면 `x`는 사라지고, 참조는 쓰레기 주소를 가리키게 된다.

---

### ⚙️ 2️⃣ 안전한 경우 (✅ OK)

| 경우                     | 설명                |
| ---------------------- | ----------------- |
| **전역/정적 변수 참조 반환**     | 함수 종료 후에도 메모리 유지됨 |
| **멤버 변수 참조 반환**        | 객체가 살아있는 동안 유효    |
| **함수 인자로 받은 객체 참조 반환** | 인자의 수명이 더 길 경우 안전 |

```cpp
// ✅ static 변수
int& GetStatic() {
    static int val = 42;
    return val;  // OK
}

// ✅ 멤버 변수 반환
class Player {
    int hp = 100;
public:
    int& GetHp() { return hp; } // OK (객체 살아있을 동안 유효)
};
```

---

### ⚙️ 3️⃣ 잠재적 함정 (주의)

**임시 객체(temporary) 반환 시**도 마찬가지로 위험하다.

```cpp
std::string& GetName() {
    return std::string("곽삣삐"); // ❌ 임시 객체 → 함수 끝나면 소멸
}
```

→ 해결책: **값 반환(value return)** 으로 바꿔야 한다.

```cpp
std::string GetName() { return "곽삣삐"; } // ✅ 복사생성 or 이동생성 (RVO 최적화)
```

현대 컴파일러는 RVO(Return Value Optimization)로 이 복사조차 제거해준다.
따라서 **참조로 리턴하려다 수명 꼬일 바엔 그냥 값으로 리턴하는 게 더 안전**하다.

---

### ⚙️ 4️⃣ 성능이 걱정될 때

> "복사비용 때문에 참조로 돌려야 하는데, 수명 문제를 피하고 싶다"
> 이때 쓸 수 있는 방법:

* 상위 객체(싱글턴, 매니저 클래스)의 멤버를 참조로 리턴
* `std::reference_wrapper` 사용
* `const &` 반환 (읽기 전용이라 그나마 안전)

```cpp
const std::string& GetConfig() {
    static std::string config = "server.ini";
    return config; // OK: static 객체, 읽기 전용
}
```

---

## ⚙️ 5️⃣ Modern C++ 추가 포인트

* `[[nodiscard]]`로 반환값 무시 방지 가능.
* `std::move()`로 명시적 이동 반환 시 참조보다 안전.
* 스마트 포인터(`std::shared_ptr`, `std::unique_ptr`)로 반환하면 수명 관리까지 자동화.

---

## 🎯 **면접용 정리**

> 참조 반환 시 가장 중요한 건 **수명(lifetime)**이다.
> 지역 변수처럼 함수 종료 시 사라지는 객체를 참조로 반환하면
> 해제된 메모리를 참조하게 되어 **dangling reference**가 발생한다.
>
> 따라서 참조로 반환할 때는
> **① 전역/정적 변수, ② 멤버 변수, ③ 호출자보다 오래 사는 객체**만 반환해야 한다.
> 그 외에는 **값 반환**이 더 안전하고, RVO가 대부분 성능도 보장해준다.

---

### 🔥 꼬리질문 예상

1. “그럼 const 참조로 반환하면 안전한가요?”
   → 아니다. `const`는 수정만 막을 뿐, 수명은 보장하지 않는다.
2. “왜 RVO(Return Value Optimization)가 중요하죠?”
   → 값 반환이라도 컴파일러가 복사 제거 → 참조보다 안전하면서 빠름.
3. “클래스 멤버 참조를 리턴할 때 주의점은?”
   → 객체가 소멸되면 참조도 무효. 즉, **객체 생명주기(lifetime)**가 더 길어야 한다.

</details>


## 10. L-value와 R-value의 차이를 설명하세요

<details close>
<summary><strong>💡 자세한 정답</strong></summary>

<div style="background-color:#f6f8fa; padding:15px; border-radius:10px;">

## 💡 **L-value vs R-value**

| 구분              | L-value               | R-value               |
| --------------- | --------------------- | --------------------- |
| **의미**          | “메모리 상에 이름(주소)이 있는 값” | “임시로 존재하는 값, 이름 없는 값” |
| **수명**          | 지속적 (스코프 동안 유지)       | 일시적 (표현식 끝나면 사라짐)     |
| **주소 취득 가능 여부** | `&` 연산자 사용 가능         | 불가능                   |
| **예시**          | 변수, 배열 원소, 참조로 반환된 값  | 상수, 연산 결과, 임시 객체      |
| **참조 유형**       | `T&` (L-value 참조)     | `T&&` (R-value 참조)    |

---

### ⚙️ 1️⃣ **예시로 직관 잡기**

```cpp
int x = 10;   // x → L-value (메모리 주소 존재)
x = 20;       // ✅ OK, L-value는 대입 대상 가능

10 = x;       // ❌ 불가능, 10은 R-value (임시 상수)

int y = x + 5; // (x + 5)는 R-value, 식이 끝나면 사라짐
```

| 표현        | 분류      | 이유               |
| --------- | ------- | ---------------- |
| `x`       | L-value | 이름 있고, 주소 가능     |
| `10`      | R-value | 이름 없음            |
| `x + 5`   | R-value | 계산 결과 임시         |
| `"Hello"` | R-value | 문자열 리터럴 (임시 데이터) |

---

### ⚙️ 2️⃣ **함수 반환 시의 L-value / R-value**

```cpp
int& RefFunc() {
    static int data = 10;
    return data;   // L-value 반환
}

int ValFunc() {
    return 10;     // R-value 반환
}

int main() {
    RefFunc() = 20; // ✅ 가능 (L-value 반환)
    ValFunc() = 20; // ❌ 불가능 (R-value 반환)
}
```

→ **참조를 반환하면 L-value**,
→ **값을 반환하면 R-value**.

---

### ⚙️ 3️⃣ **L-value 참조 & R-value 참조**

```cpp
int a = 10;
int& lref = a;       // ✅ L-value 참조
int&& rref = 20;     // ✅ R-value 참조 (임시값 참조 가능)

// rref는 실제로 임시 객체에 대한 이름이 생긴 것과 같음
rref = 30;           // OK (rref는 L-value가 됨)
```

💡 핵심:

* R-value 참조(`T&&`)는 **임시 객체에 이름을 붙일 수 있게 하는 문법**.
* 이것 덕분에 **move semantics**, **perfect forwarding**이 가능해진다.

---

### ⚙️ 4️⃣ **이동 의미론과의 관계**

```cpp
std::string MakeName() {
    return "곽삣삐";
}

std::string name = MakeName(); // RVO or move

std::string name2 = std::move(name); // R-value 캐스팅 → 자원 이동
```

* `std::move()`는 단순히 `T&&` 캐스팅.
* “이 객체를 더 이상 안 쓸 테니, 복사 말고 이동해라.”
* 즉, **R-value는 소유권을 이전해도 되는 임시값**으로 취급한다.

---

### ⚙️ 5️⃣ **왼쪽(L)과 오른쪽(R)의 어원**

* L-value: assignment **Left** (대입 연산자 왼쪽 가능)
* R-value: assignment **Right** (대입 연산자 오른쪽만 가능)

```cpp
x = 10;   // x ← Left-value
          // 10 → Right-value
```

---

### ⚙️ 6️⃣ **Modern C++에서 확장된 개념**

C++11 이후엔 더 세분화된 분류가 있음 👇
(면접에선 간단히 요약만 해도 충분)

| 용어           | 의미                                                   |
| ------------ | ---------------------------------------------------- |
| **L-value**  | 이름 있는 객체                                             |
| **X-value**  | “eXpiring” value, soon-to-die 객체 (`std::move(name)`) |
| **Pr-value** | “Pure” R-value, 완전 임시 (`10`, `"Hi"`)                 |

→ L-value + R-value → 통합적으로 **"value category"**라고 부른다.

---

## 🎯 **면접용 요약**

> L-value는 “메모리 상 이름이 있는 값”,
> R-value는 “임시로 존재하는 값”입니다.
>
> L-value는 대입의 왼쪽에 올 수 있고, 주소를 얻을 수 있지만
> R-value는 식이 끝나면 사라지는 일시적인 값입니다.
>
> 현대 C++에서는 R-value 참조(`T&&`)를 통해 임시 객체에 이름을 부여할 수 있게 되어,
> **이동 의미론(move semantics)** 과 **자원 효율적인 전달(perfect forwarding)** 이 가능해졌습니다.

---

### 🔥 꼬리질문 예상

1. “`std::move`는 내부적으로 뭘 하나요?”
   → 단순히 `static_cast<T&&>`로 **R-value로 캐스팅**만 함.
   실제 이동은 `move constructor`가 수행.

2. “임시 객체를 const L-value 참조로 받을 수 있는 이유는요?”
   → 임시 객체의 lifetime이 **const 참조로 연장되기 때문**.

   ```cpp
   const std::string& ref = std::string("temp"); // OK, 수명 연장
   ```

3. “R-value 참조가 없다면 이동 의미론은 구현이 불가능할까요?”
   → 거의 불가능함. R-value 참조(`T&&`)가 있어야 “임시 객체를 파괴 전 재활용” 가능.

</details>


## 11. explicit 키워드는 언제 사용하나요?

<details close>
<summary><strong>💡 자세한 정답</strong></summary>

<div style="background-color:#f6f8fa; padding:15px; border-radius:10px;">

## 💡 **`explicit` 키워드란?**

> **암시적(implicit) 변환을 막고, 명시적(explicit) 변환만 허용하는 키워드**

즉, **생성자나 변환 연산자**가 의도치 않게 자동 호출되어
타입이 몰래 변환되는 걸 막기 위한 장치야.

---

### ⚙️ 1️⃣ 암시적 변환의 문제 예시 (❌ 위험 코드)

```cpp
class Vector2 {
public:
    Vector2(float x, float y) : _x(x), _y(y) {}
    Vector2(float x) : _x(x), _y(0) {} // 암시적 변환 가능
private:
    float _x, _y;
};

void Print(Vector2 v) { /* ... */ }

int main() {
    Print(5); // ❗ int → float → Vector2(5,0) 자동 변환됨
}
```

위 예시는 의도하지 않은 타입 변환이 일어남.
→ 컴파일러가 `Vector2(float)` 생성자를 **암시적으로 호출**해서 `Vector2`로 바꿔버림.
→ 이게 바로 **암시적 변환(implicit conversion)**.

---

### ⚙️ 2️⃣ `explicit`로 안전하게 막기 (✅ 올바른 사용)

```cpp
class Vector2 {
public:
    explicit Vector2(float x) : _x(x), _y(0) {}
    Vector2(float x, float y) : _x(x), _y(y) {}
private:
    float _x, _y;
};

void Print(Vector2 v) { /* ... */ }

int main() {
    Print(5);        // ❌ 컴파일 에러: 암시적 변환 차단됨
    Print(Vector2(5)); // ✅ 명시적 생성자 호출만 허용
}
```

`explicit`을 붙이면,
→ `Vector2 v = 5;` 같은 암시적 변환이 **금지**,
→ `Vector2 v(5);` 처럼 **명시적 생성자 호출만 허용**된다.

---

### ⚙️ 3️⃣ 변환 연산자에도 적용 가능

```cpp
class Player {
    int _id = 100;
public:
    explicit operator int() const { return _id; } // 명시적 변환만 허용
};

int main() {
    Player p;
    int id1 = (int)p; // ✅ 명시적 캐스팅
    int id2 = p;      // ❌ 암시적 변환 금지
}
```

→ `explicit operator`는 **operator overloading으로 인한 자동 형변환 버그** 방지용.

---

### ⚙️ 4️⃣ **C++11 이후 확장: `explicit` 생성자 + `constexpr` / `noexcept`**

* `explicit constexpr` 조합도 가능함.
* 헤더 유틸 함수에서 **암시적 변환 없이 상수 생성자**로 자주 사용.

```cpp
struct Degree {
    explicit constexpr Degree(float d) : deg(d) {}
    float deg;
};
```

---

### ⚙️ 5️⃣ **서버 코드에서 자주 발생하는 실제 함정 예시**

```cpp
class SessionID {
public:
    SessionID(uint64_t id) : _id(id) {}
private:
    uint64_t _id;
};

void Disconnect(SessionID id);

int main() {
    Disconnect(42); // ❗ 자동 변환되어 Disconnect(SessionID(42)) 호출됨
}
```

→ 숫자를 잘못 넘겼는데, **자동으로 변환되어 컴파일 에러 없이 동작**해버림.
→ 런타임 버그로 이어질 수 있음.

✅ 해결:

```cpp
explicit SessionID(uint64_t id) : _id(id) {}
```

이러면 `Disconnect(42);`가 바로 컴파일 에러로 막힌다.
→ 안전한 타입 설계(Type-safe design).

---

### ⚙️ 6️⃣ **규칙 요약**

| 사용 위치             | 역할          |
| ----------------- | ----------- |
| `explicit` 생성자    | 암시적 변환 방지   |
| `explicit` 변환 연산자 | 자동 캐스팅 방지   |
| 복사 생성자에는 보통 안 붙임  | 타입 동일하므로 안전 |
| 단일 인자 생성자에는 거의 필수 | 타입 안전성 확보   |

---

## 🎯 **면접용 요약**

> `explicit`은 **암시적 변환을 막는 키워드**입니다.
> 단일 인자를 받는 생성자나 변환 연산자에서
> 컴파일러가 자동으로 타입 변환을 시도하지 못하게 막고,
> 반드시 **명시적인 생성자 호출만 허용**하게 만듭니다.
>
> 즉, 타입 안전성을 높이고, 실수로 인한 **자동 변환 버그를 예방**하기 위해 사용합니다.

---

### 🔥 꼬리질문 예상

1. “explicit을 붙이면 성능에 영향이 있나요?”
   → 전혀 없음. 컴파일러 수준에서 변환 경로만 막는 **컴파일 타임 제약**일 뿐.

2. “explicit 생성자는 언제 생략하나요?”
   → 여러 인자를 받는 생성자는 암시 변환이 안 일어나므로 굳이 안 붙인다.

3. “explicit을 붙여야 하는 기준은요?”
   → **단일 인자 생성자** + **타입 변환 가능성이 있는 경우**엔 무조건 붙여라.
   예: `Vector2(float)`, `SessionID(uint64_t)`, `String(const char*)`

</details>



## 12. 캡슐화, 상속, 다형성의 의미를 설명해보세요.

<details close>
<summary><strong>💡 자세한 정답</strong></summary>

<div style="background-color:#f6f8fa; padding:15px; border-radius:10px;">


## 💡 객체지향 프로그래밍(OOP)의 3대 핵심 개념

| 개념                      | 핵심 의미                       | C++ 관점 핵심 키워드                    |
| ----------------------- | --------------------------- | -------------------------------- |
| **캡슐화 (Encapsulation)** | 데이터와 메서드를 하나로 묶고, 외부 접근을 제한 | `private`, `protected`, `public` |
| **상속 (Inheritance)**    | 기존 클래스를 확장·재사용              | `: public Base`                  |
| **다형성 (Polymorphism)**  | 같은 인터페이스로 다양한 동작 수행         | `virtual`, `override`            |

---

## ⚙️ 1️⃣ **캡슐화 (Encapsulation)**

> **데이터(속성)와 동작(함수)을 하나의 단위로 묶고, 외부로부터 보호하는 것.**

### 🧩 예시

```cpp
class Player {
private:
    int hp;               // 외부에서 직접 접근 불가
public:
    void Damage(int d) { hp -= d; }
    int GetHp() const { return hp; }
};
```

* `hp`는 `private`이라 직접 접근 불가 (`player.hp = 0; ❌`)
* 대신 **공개된 인터페이스(public method)**를 통해서만 조작 가능.

### 💬 면접 답변 포인트

* 데이터 보호(무결성 유지)
* 내부 구현 변경 시 외부 코드 영향 최소화 (유지보수성 향상)
* **정보 은닉(Information Hiding)**을 통한 설계 안정성 확보

🧠 예시 한마디:

> “서버에서 Player 상태를 캡슐화하면, 외부 모듈이 hp를 임의 조작할 수 없게 해서 안정성을 확보합니다.”

---

## ⚙️ 2️⃣ **상속 (Inheritance)**

> **기존 클래스의 속성과 기능을 물려받아 새로운 클래스를 정의하는 것.**

### 🧩 예시

```cpp
class Player {
public:
    virtual void Attack() { std::cout << "기본 공격\n"; }
};

class Warrior : public Player {
public:
    void Attack() override { std::cout << "검 휘두르기\n"; }
};

class Mage : public Player {
public:
    void Attack() override { std::cout << "파이어볼\n"; }
};
```

### 💬 면접 답변 포인트

* **코드 재사용성(Reusability)**
  → 공통 기능을 부모 클래스에 두고, 중복 최소화
* **확장성(Extensibility)**
  → 새로운 타입을 손쉽게 추가 가능 (`Player` 상속만 하면 됨)
* **유지보수성(Maintainability)**
  → 공통 코드 수정 시 전체 자식 클래스에 자동 반영

📌 C++에서는

* **접근 제어자(public/protected/private)**에 따라 상속의 공개 범위가 달라짐.
* “다중 상속”도 가능하지만, **가상 상속(virtual inheritance)**으로 모호성 해결 필요.

---

## ⚙️ 3️⃣ **다형성 (Polymorphism)**

> **같은 인터페이스로 다양한 객체를 동일하게 다루는 것.**

### 🧩 예시

```cpp
void Battle(Player* p) {
    p->Attack(); // Warrior면 검, Mage면 파이어볼
}

int main() {
    Warrior w; Mage m;
    Battle(&w);
    Battle(&m);
}
```

* **런타임 다형성**: virtual 함수 테이블(vtable)을 통해 실제 객체 타입에 맞게 호출됨.
* **컴파일타임 다형성**: 함수 오버로딩, 템플릿 등도 다형성의 일종.

### 💬 면접 답변 포인트

* 같은 포인터(`Player*`)로 다양한 행동 가능 → **확장성 + 유지보수성 향상**
* 게임 서버에서 “공통 인터페이스 기반 객체 관리”에 자주 쓰임.
  예: `GameObject`, `Session`, `Job` 등 상위 클래스로 다루기.

---

## ⚙️ 4️⃣ **세 가지의 관계**

| 개념  | 비유                         | C++ 관점                       |
| --- | -------------------------- | ---------------------------- |
| 캡슐화 | “데이터를 금고에 넣고, 키를 함수로만 제공”  | private 멤버 + public 메서드      |
| 상속  | “기본 설계도를 물려받아 새 건물 짓기”     | 재사용, 확장                      |
| 다형성 | “같은 리모컨 버튼이지만 TV마다 동작이 다름” | virtual / override 기반 동적 바인딩 |

---

## 🎯 **면접용 정리**

> 객체지향의 핵심은 **캡슐화, 상속, 다형성**입니다.
>
> * **캡슐화**는 데이터를 숨기고, 안정된 인터페이스를 제공해 모듈 간 결합도를 낮춥니다.
> * **상속**은 기존 기능을 재사용하고, 확장성을 높여 유지보수를 쉽게 합니다.
> * **다형성**은 동일한 인터페이스로 서로 다른 객체를 유연하게 처리하게 해줍니다.
>
> 특히 C++에서는 **virtual 함수 기반의 런타임 다형성**이 가장 중요한 특징입니다.
> 이 세 가지가 결합되어, 대규모 시스템을 구조적으로 확장 가능하게 만듭니다.

---

### 🔥 꼬리질문 예상

1. “캡슐화와 정보 은닉의 차이는요?”
   → 캡슐화는 “묶는 것”, 정보 은닉은 “가리는 것”. 캡슐화는 은닉을 실현하는 수단.

2. “상속 대신 조합(Composition)을 쓰는 이유는?”
   → 상속은 강한 결합, 조합은 느슨한 결합. 유지보수성 측면에서 조합이 더 안전한 경우 많음.

3. “다형성이 없으면 코드에 어떤 문제가 생기나요?”
   → 조건문(`if (type == X)`)이 폭발적으로 늘어나고, 확장 시 수정 범위가 커짐 → OCP 위반(Open–Closed Principle).

</details>



## 13. 가상 함수(virtual function)의 역할은 무엇인가요?

<details close>
<summary><strong>💡 자세한 정답</strong></summary>

<div style="background-color:#f6f8fa; padding:15px; border-radius:10px;">


## 💡 **가상 함수(virtual function)**

> **상속 관계에서, 객체의 실제 타입에 따라 호출되는 함수를 결정하는 메커니즘.**
>
> 즉, “컴파일 타임이 아닌 **런타임**에 호출 대상을 결정”해서
> **다형성(Polymorphism)** 을 구현하는 핵심 장치다.

---

### ⚙️ 1️⃣ **가상 함수 없는 경우 (정적 바인딩)**

```cpp
class Player {
public:
    void Attack() { std::cout << "기본 공격\n"; }
};

class Warrior : public Player {
public:
    void Attack() { std::cout << "검 휘두르기\n"; }
};

int main() {
    Player* p = new Warrior();
    p->Attack(); // "기본 공격" 호출 ❌ (부모 버전)
}
```

> 포인터는 `Player*` 타입이므로 **컴파일 타임에** Player의 `Attack()`이 고정됨.
> → **정적 바인딩(static binding)**.

---

### ⚙️ 2️⃣ **virtual 사용 시 (동적 바인딩)**

```cpp
class Player {
public:
    virtual void Attack() { std::cout << "기본 공격\n"; }
};

class Warrior : public Player {
public:
    void Attack() override { std::cout << "검 휘두르기\n"; }
};

int main() {
    Player* p = new Warrior();
    p->Attack(); // ✅ "검 휘두르기"
}
```

> 이제는 런타임에 `p`가 **Warrior 객체**임을 인식하고,
> 실제 객체의 `Attack()`을 호출한다.
> → **동적 바인딩(dynamic dispatch)**.

---

### ⚙️ 3️⃣ **vtable과 vptr의 동작 원리 (면접 핵심)**

C++은 내부적으로 “가상 함수 테이블(vtable)” 구조로 다형성을 구현한다.

```
[ Player 객체 메모리 구조 ]
+------------------------+
| vptr → vtable 주소     |  ---> vtable: { &Player::Attack }
| 멤버 변수들 ...        |
+------------------------+

[ Warrior 객체 메모리 구조 ]
+------------------------+
| vptr → vtable 주소     |  ---> vtable: { &Warrior::Attack }
| 멤버 변수들 ...        |
+------------------------+
```

* **vptr (virtual pointer)**: 각 객체가 갖는 숨겨진 포인터. 자신의 vtable 주소를 가리킴.
* **vtable (virtual table)**: 가상 함수들의 실제 주소를 담은 테이블.

👉 즉, `p->Attack()` 호출 시,
`p->vptr` → `vtable` → `Attack()` 주소를 찾아가 실행한다.

이게 바로 **런타임 다형성의 기초 메커니즘**이다.

---

### ⚙️ 4️⃣ **override 키워드**

* 자식 클래스에서 가상 함수를 재정의할 때 `override`를 붙여
  함수 시그니처 불일치 실수를 방지.

```cpp
void Attack() override; // 부모에 virtual 있어야만 유효
```

→ override는 “컴파일 타임 검증용” 키워드야.
→ 없으면 단순한 “함수 숨김(name hiding)”이 발생할 수 있음.

---

### ⚙️ 5️⃣ **가상 소멸자 (virtual destructor)**

> 부모 클래스의 소멸자는 반드시 virtual로 선언하자.

```cpp
class Player {
public:
    virtual ~Player() { std::cout << "Player 소멸\n"; }
};

class Warrior : public Player {
public:
    ~Warrior() { std::cout << "Warrior 소멸\n"; }
};

int main() {
    Player* p = new Warrior();
    delete p; // ✅ Warrior → Player 순서로 정상 소멸
}
```

* virtual이 없으면 Warrior의 소멸자가 **호출되지 않아 메모리 누수 발생.**

---

### ⚙️ 6️⃣ **성능 & 메모리 관점**

| 항목  | 영향                             |
| --- | ------------------------------ |
| 메모리 | 객체당 1개의 vptr 추가                |
| 속도  | 호출 시 vtable lookup(간접 참조) 오버헤드 |
| 최적화 | 컴파일러가 inline 최적화를 제한함          |

→ 하지만 현실적으로 이 오버헤드는 **거의 무시 가능한 수준**.
→ IOCP 서버 같은 고성능 시스템에서도 다형성은 충분히 사용됨 (핵심 부분만 함수 포인터로 대체하면 됨).

---

### ⚙️ 7️⃣ **pure virtual (순수 가상 함수)**

> 자식 클래스에게 **“이 함수는 반드시 구현하라”** 강제하는 인터페이스 개념.

```cpp
class IJob {
public:
    virtual void Execute() = 0; // 순수가상함수
};
```

* `= 0` 표시는 **“구현 없음”**
* 이 함수가 하나라도 있으면 **추상 클래스(Abstract Class)**

---

## 🎯 **면접용 정리**

> 가상 함수는 **상속 관계에서 다형성을 구현하기 위한 핵심 메커니즘**입니다.
>
> `virtual`을 붙이면 함수 호출이 **컴파일 타임이 아니라 런타임에 결정**되며,
> 내부적으로는 객체가 가진 **vptr**이 **vtable**을 참조해 실제 함수 주소를 찾아 호출합니다.
>
> 이를 통해 `Player*` 타입으로도 실제 객체(`Warrior`, `Mage`)의 행동을 수행할 수 있습니다.
>
> 단, 부모 소멸자는 반드시 `virtual`로 선언해 메모리 누수를 방지해야 합니다.

---

### 🔥 꼬리질문 예상

1. “가상 함수 호출은 왜 느리다고 하나요?”
   → 직접 호출이 아니라 vtable을 거치는 **간접 호출(indirect call)**이라 한 단계 늦음. 하지만 미미한 수준.

2. “virtual 안 붙이면 다형성이 안 되나요?”
   → 네. virtual 없으면 **정적 바인딩**으로 부모 함수만 호출됨.

3. “vtable은 클래스당 하나인가요, 객체당 하나인가요?”
   → vtable은 클래스당 1개, vptr은 객체마다 1개씩 존재.

4. “pure virtual 함수와 abstract class의 차이?”
   → pure virtual 함수가 하나라도 있으면 abstract class가 된다.



</details>


## 14. 다형성(polymorphism)을 어떻게 구현하나요?

<details close>
<summary><strong>💡 자세한 정답</strong></summary>

<div style="background-color:#f6f8fa; padding:15px; border-radius:10px;">

## 💡 **다형성(Polymorphism)**

> **하나의 인터페이스로 여러 타입의 객체를 다룰 수 있게 하는 개념.**

C++에서는 크게 두 가지 방식으로 구현된다 👇
1️⃣ **런타임 다형성 (Virtual Function 기반)**
2️⃣ **컴파일타임 다형성 (Template, Overloading 기반)**

---

## ⚙️ 1️⃣ 런타임 다형성 — **가상 함수 기반 (Dynamic Polymorphism)**

> 실행 중(runtime)에 어떤 함수가 호출될지 결정되는 방식.
> 핵심: `virtual` 키워드 + **vtable** 메커니즘.

### 🧩 예시

```cpp
class Player {
public:
    virtual void Attack() { std::cout << "기본 공격\n"; }
    virtual ~Player() {}
};

class Warrior : public Player {
public:
    void Attack() override { std::cout << "검 휘두르기\n"; }
};

class Mage : public Player {
public:
    void Attack() override { std::cout << "파이어볼!\n"; }
};

void Battle(Player* p) {
    p->Attack(); // 객체 타입에 따라 실제 호출 다름
}

int main() {
    Warrior w; Mage m;
    Battle(&w);  // "검 휘두르기"
    Battle(&m);  // "파이어볼!"
}
```

* 포인터 타입은 `Player*`로 같지만,
  실제로는 `Warrior`나 `Mage` 객체의 `Attack()`이 호출된다.
* 즉, **런타임에 vtable을 참조해 실제 함수를 결정한다.**

---

### ⚙️ 동작 메커니즘 (요약)

1. `virtual` 함수가 있는 클래스 → 컴파일러가 **vtable** 생성.
2. 객체 생성 시 **vptr**이 해당 vtable 주소를 가리킴.
3. 호출 시 `obj->vptr->Attack()` 형태로 **간접 호출**.

📌 → 이게 바로 **“동적 바인딩(dynamic dispatch)”**
vs
**“정적 바인딩(static binding)”** (virtual 없는 일반 함수 호출)

---

### ✅ 장점

* 인터페이스 기반의 유연한 설계 (확장성↑)
* 새로운 클래스 추가해도 기존 코드 수정 최소화

### ⚠️ 단점

* vtable lookup 비용 (미미하지만 존재)
* 인라인화 불가능, 최적화 제약
* 클래스 구조 복잡해질 수 있음

---

## ⚙️ 2️⃣ 컴파일타임 다형성 — **템플릿 기반 (Static Polymorphism)**

> 실행 전에(컴파일 타임) 어떤 함수가 호출될지 결정되는 방식.
> 즉, **타입에 따라 다른 함수 버전이 자동 생성.**

### 🧩 예시

```cpp
template <typename T>
void Attack(T& unit) {
    unit.Attack();
}

class Warrior {
public:
    void Attack() { std::cout << "검 휘두르기\n"; }
};

class Mage {
public:
    void Attack() { std::cout << "파이어볼!\n"; }
};

int main() {
    Warrior w; Mage m;
    Attack(w);  // Warrior::Attack()
    Attack(m);  // Mage::Attack()
}
```

→ `Attack()` 함수가 Warrior/Mage 타입 각각에 대해 **컴파일 시점에 인스턴스화**됨.
→ 런타임 비용 없이 완전한 인라인 처리 가능.

---

### ⚙️ 동작 차이 요약

| 구분        | 런타임 다형성                   | 컴파일타임 다형성                         |
| --------- | ------------------------- | --------------------------------- |
| **기반**    | `virtual` 함수, vtable      | 템플릿 / 오버로딩                        |
| **결정 시점** | 런타임                       | 컴파일타임                             |
| **속도**    | 약간 느림 (간접 호출)             | 매우 빠름 (인라인 가능)                    |
| **유연성**   | 높은 확장성, 인터페이스 기반          | 타입 종속적, 코드 중복 가능                  |
| **대표 예시** | `Player* p; p->Attack();` | `template<typename T> Attack(T&)` |

---

### ⚙️ 둘을 합친 고급 기법 — **CRTP (Curiously Recurring Template Pattern)**

> 템플릿을 이용해 “가상 함수 오버헤드 없는 다형성” 구현

```cpp
template <typename Derived>
class Player {
public:
    void Attack() { static_cast<Derived*>(this)->AttackImpl(); }
};

class Warrior : public Player<Warrior> {
public:
    void AttackImpl() { std::cout << "검 휘두르기\n"; }
};
```

→ virtual 없이도 **정적 다형성(static polymorphism)** 달성
→ vtable 없는 lightweight polymorphism
→ IOCP 서버나 ECS(Entity Component System) 같은 성능 민감 코드에 자주 씀.

---

## 🎯 **면접용 정리**

> 다형성은 **같은 인터페이스로 다른 동작을 수행할 수 있게 하는 객체지향의 핵심 개념**입니다.
>
> C++에서는
>
> * `virtual` 함수 기반으로 **런타임 다형성(dynamic polymorphism)**,
> * `template` 기반으로 **컴파일타임 다형성(static polymorphism)**
>   을 제공합니다.
>
> 전자는 **vtable을 통한 동적 바인딩**,
> 후자는 **컴파일 시점 타입 인스턴스화**로 구현됩니다.
>
> 런타임 다형성은 유연성, 컴파일타임 다형성은 성능이 강점입니다.

---

### 🔥 꼬리질문 예상

1. “virtual 대신 템플릿으로 다형성을 구현할 수 있나요?”
   → 가능. CRTP나 concept를 이용하면 가상 함수 오버헤드 없는 정적 다형성 구현 가능.

2. “왜 템플릿 기반 다형성은 런타임에 타입 교체가 불가능한가요?”
   → 이미 컴파일 시점에 구체화돼서, 런타임에 타입이 바뀔 수 없음.

3. “서버 코드에서 어느 다형성을 더 많이 쓰나요?”
   → 런타임엔 virtual (Session, Job, PacketHandler),
   성능 critical 구간은 템플릿 기반(static polymorphism)으로 설계.


</details>



## 15. this 포인터는 언제, 왜 필요한가요?

<details close>
<summary><strong>💡 자세한 정답</strong></summary>

<div style="background-color:#f6f8fa; padding:15px; border-radius:10px;">

## 💡 **`this` 포인터란?**

> **멤버 함수가 호출된 객체 자기 자신을 가리키는 포인터.**
>
> 즉, 클래스 내부에서 “지금 호출한 객체가 누구인지”를 식별할 수 있도록
> 컴파일러가 **자동으로 전달해주는 숨은 매개변수(hidden parameter)**야.

---

## ⚙️ 1️⃣ **`this` 포인터의 기본 원리**

```cpp
class Player {
    int hp;
public:
    void Damage(int d) {
        this->hp -= d;   // 실제로는 this->hp
    }
};
```

컴파일러는 내부적으로 다음과 같이 바꿔서 처리한다 👇

```cpp
void Damage(Player* const this, int d) {
    this->hp -= d;
}
```

즉,

* `Damage()`는 **전역 함수처럼 동작하지만**,
* `this`라는 **포인터 인자**를 자동으로 받는 구조야.

그래서 `p1.Damage(10);` 호출 시
→ 내부적으로 `Damage(&p1, 10);` 로 변환된다.

---

## ⚙️ 2️⃣ **언제 필요한가?**

| 상황                     | 이유                   |
| ---------------------- | -------------------- |
| 멤버 변수와 지역 변수 이름이 같을 때  | 구분을 위해               |
| 자기 자신을 반환할 때 (메서드 체이닝) | `return *this;`      |
| 현재 객체 주소를 외부 함수에 넘길 때  | 콜백, 등록, 비교용          |
| 연산자 오버로딩에서 자기 자신 참조 필요 | ex) `operator=` 구현 시 |

---

### 🧩 예시 1 — 이름 충돌 시 구분

```cpp
class Player {
    int hp;
public:
    void SetHp(int hp) { this->hp = hp; } // this 없으면 매개변수 hp와 혼동
};
```

---

### 🧩 예시 2 — 메서드 체이닝(Builder 패턴)

```cpp
class Player {
    int hp, mp;
public:
    Player& SetHp(int v) { hp = v; return *this; }
    Player& SetMp(int v) { mp = v; return *this; }
};

Player p;
p.SetHp(100).SetMp(50); // ✅ 체이닝 가능
```

---

### 🧩 예시 3 — 자기 주소를 전달할 때

```cpp
class Session {
public:
    void Register() {
        RegisterToManager(this); // 현재 세션의 주소 전달
    }
};
```

→ 이 패턴은 **IOCP 서버 구조에서 매우 흔함.**
예: `AcceptEvent`나 `RecvEvent`가 `this`를 넘겨서
이벤트 완료 시 자기 자신으로 복귀하게끔 한다.

---

### 🧩 예시 4 — 대입 연산자 구현

```cpp
class Vector2 {
    float x, y;
public:
    Vector2& operator=(const Vector2& rhs) {
        if (this == &rhs) // 자기 자신 대입 방지
            return *this;
        x = rhs.x;
        y = rhs.y;
        return *this;
    }
};
```

→ `this`는 “왼쪽 피연산자 객체 주소”
→ 자기 자신인지 비교하고, 자기 자신 반환으로 체이닝 지원.

---

## ⚙️ 3️⃣ **`this` 포인터의 특징**

| 특징                                | 설명                       |
| --------------------------------- | ------------------------ |
| 자동 전달                             | 멤버 함수 호출 시 컴파일러가 자동으로 넣음 |
| 멤버 함수에서만 존재                       | static 함수에는 없음           |
| 상수 객체에서는 `const Player* this`로 전달 | 상수 멤버 함수에서는 수정 불가        |
| 객체 주소 그대로 가리킴                     | 참조가 아니라 실제 포인터           |
| 반환 시 `*this`로 값 또는 참조 반환 가능       | 체이닝 패턴에 활용               |

---

### 🧩 const 함수와 this

```cpp
class Player {
    int hp;
public:
    void Heal(int v) const {
        // this는 const Player* 타입
        // this->hp += v; ❌ 불가능
    }
};
```

`const` 멤버 함수에서는 `this`가 `const Player*`로 바뀌어서
객체 수정이 허용되지 않는다.

---

## ⚙️ 4️⃣ **`this` 포인터는 언제 생기지 않는가?**

* **`static` 멤버 함수**에서는 없다.
  왜냐면 static 함수는 **객체 없이 호출 가능**하기 때문.

```cpp
class Player {
public:
    static void Hello() {
        // this ❌ 없음
    }
};
```

---

## 🎯 **면접용 정리**

> `this` 포인터는 **현재 객체 자신을 가리키는 포인터**로,
> 멤버 함수 호출 시 컴파일러가 **자동으로 전달하는 숨은 인자**입니다.
>
> 주로 멤버 변수와 지역 변수 구분, 자기 자신 반환(`return *this`),
> 자기 주소 전달(`Register(this)`), 연산자 오버로딩 등에서 사용됩니다.
>
> 단, `static` 멤버 함수에는 객체가 없으므로 `this` 포인터도 존재하지 않습니다.

---

### 🔥 꼬리질문 예상

1. “const 멤버 함수에서 this는 어떤 타입인가요?”
   → `const ClassName* const this` (객체 내용 수정 불가)

2. “static 멤버 함수에서 this가 없는 이유는?”
   → 객체 없이 호출되므로, 가리킬 인스턴스가 없음.

3. “this 포인터가 메모리상 어디에 저장되나요?”
   → 일반 인자처럼 스택 프레임에 저장됨 (레지스터 최적화 가능).

4. “return *this;”가 어떤 의미인가요?”
   → 현재 객체를 **참조 형태로 반환** — 체이닝 호출 가능하게 함.

</details>


## 16. 연산자 오버로딩(operator overloading)은 왜 사용하나요?

<details close>
<summary><strong>💡 자세한 정답</strong></summary>

<div style="background-color:#f6f8fa; padding:15px; border-radius:10px;">

## 💡 **연산자 오버로딩(operator overloading)이란?**

> **사용자 정의 타입(클래스)에 대해 기존 연산자(`+`, `-`, `==`, `[]`, `()`, `<<` 등)의 의미를 재정의하는 기능.**
>
> 즉, **객체 간 연산을 자연스럽고 직관적으로 표현**할 수 있게 해준다.

---

## ⚙️ 1️⃣ **기본 아이디어**

```cpp
class Vector2 {
public:
    float x, y;

    Vector2(float x, float y) : x(x), y(y) {}

    // 연산자 오버로딩
    Vector2 operator+(const Vector2& rhs) const {
        return Vector2(x + rhs.x, y + rhs.y);
    }
};

int main() {
    Vector2 a(1, 2), b(3, 4);
    Vector2 c = a + b;  // ✅ 자연스러운 연산
}
```

→ `a + b` 문장은 사실

```cpp
a.operator+(b);
```

로 해석된다.
즉, **연산자는 단순 문법적 설탕(Syntax Sugar)** 이고
컴파일러는 내부적으로 함수 호출로 바꿔서 처리한다.

---

## ⚙️ 2️⃣ **멤버 함수 vs 전역 함수 오버로딩**

| 방식        | 형태                                                              | 특징                        |
| --------- | --------------------------------------------------------------- | ------------------------- |
| **멤버 함수** | `Vector2 operator+(const Vector2& rhs) const;`                  | 왼쪽 피연산자가 해당 클래스 타입일 때만 가능 |
| **전역 함수** | `friend Vector2 operator+(const Vector2& a, const Vector2& b);` | 양쪽 피연산자 모두 자유롭게 가능        |

### 🧩 예시

```cpp
class Vector2 {
    float x, y;
public:
    Vector2(float x, float y) : x(x), y(y) {}
    friend Vector2 operator+(const Vector2& a, const Vector2& b) {
        return Vector2(a.x + b.x, a.y + b.y);
    }
};
```

→ `a + b`, `b + a` 모두 가능 (멤버 함수는 왼쪽 타입이 고정되기 때문).

---

## ⚙️ 3️⃣ **입출력 연산자 오버로딩 (`<<`, `>>`)**

> `std::cout`이나 `std::cin` 같은 스트림에 객체를 출력하려면 꼭 필요.

```cpp
#include <iostream>
class Player {
    std::string name;
public:
    Player(std::string n) : name(std::move(n)) {}
    friend std::ostream& operator<<(std::ostream& os, const Player& p) {
        return os << "Player: " << p.name;
    }
};

int main() {
    Player p("곽삣삣");
    std::cout << p << '\n'; // ✅ Player: 곽삣삣
}
```

→ `std::ostream&` 반환을 통해 연속 출력 가능 (`std::cout << a << b;`)

---

## ⚙️ 4️⃣ **비교 연산자 (`==`, `<`)**

```cpp
class Player {
    int id;
public:
    Player(int id) : id(id) {}
    bool operator==(const Player& rhs) const { return id == rhs.id; }
    bool operator<(const Player& rhs) const  { return id < rhs.id; }
};
```

→ 이런 정의 덕분에
`std::sort`, `std::set`, `std::map` 등 **STL 컨테이너**에서도 정상 작동한다.

---

## ⚙️ 5️⃣ **대입/복사 관련 오버로딩**

> **얕은 복사 / 깊은 복사** 구분이 필요한 경우 반드시 직접 구현.

```cpp
class Buffer {
    char* data;
public:
    Buffer(const char* src) {
        data = new char[strlen(src) + 1];
        strcpy(data, src);
    }
    Buffer& operator=(const Buffer& rhs) {
        if (this == &rhs) return *this; // 자기 자신 대입 방지
        delete[] data;
        data = new char[strlen(rhs.data) + 1];
        strcpy(data, rhs.data);
        return *this;
    }
};
```

---

## ⚙️ 6️⃣ **주의사항 (면접 핵심 포인트)**

| 주의점                                   | 설명                            |
| ------------------------------------- | ----------------------------- |
| **의미가 자연스러워야 함**                      | `a + b`는 덧셈 의미여야 함. 이상한 동작(X) |
| **부작용 최소화**                           | 연산자 하나로 여러 상태 변경 금지           |
| **`=` / `==` / `<` 등은 논리적으로 일관되어야 함** | 비교 연산자 간 관계 유지                |
| **불필요한 오버로딩 지양**                      | 가독성 해치거나 디버깅 복잡해짐             |
| **멤버 함수 vs 전역 함수 선택 주의**              | 왼쪽 피연산자가 내 클래스인지에 따라 다름       |

---

## ⚙️ 7️⃣ **컴파일러 자동 제공 오버로딩**

C++은 다음 기본 연산자를 자동 생성한다 (필요 시 재정의 가능):

* **복사 생성자**
* **복사 대입 연산자**
* **소멸자**
* **이동 생성자 / 이동 대입 연산자**

→ RAII 클래스나 리소스 소유 클래스에서는 반드시 직접 정의해서 **얕은 복사 버그 방지**해야 한다.

---

## 🎯 **면접용 정리**

> 연산자 오버로딩은 사용자 정의 타입 간의 연산을 **자연스럽게 표현하고, 코드 가독성을 높이기 위한 기능**입니다.
> 내부적으로는 단순히 **연산자를 함수 호출로 치환**할 뿐이며,
> 의미가 명확하고 논리적인 경우에만 사용하는 것이 좋습니다.
>
> 예를 들어 `Vector a + b`, `p1 == p2`, `cout << obj` 같은 연산을 지원하면
> 코드가 직관적이고 유지보수가 쉬워집니다.
> 하지만 의미가 불분명하거나 부작용이 큰 연산은 오버로딩하지 않는 게 원칙입니다.

---

### 🔥 꼬리질문 예상

1. “`operator<<`는 왜 전역 함수로 구현하나요?”
   → 왼쪽 피연산자(`std::ostream`)가 내 클래스가 아니기 때문.

2. “멤버 함수 vs 전역 함수의 차이?”
   → 멤버는 왼쪽 피연산자 고정, 전역은 양쪽 자유.

3. “오버로딩 남용의 문제는?”
   → 의미 모호, 유지보수 어려움. `*`, `/`, `++` 같은 연산은 오버로딩 시 오해 유발.

4. “대입 연산자 오버로딩에서 `return *this;` 하는 이유는?”
   → 연쇄 대입 지원 (`a = b = c;`)


</details>



## 17. 복사 생성자와 대입 연산자의 차이점은?

<details close>
<summary><strong>💡 자세한 정답</strong></summary>

<div style="background-color:#f6f8fa; padding:15px; border-radius:10px;">


## 💡 **핵심 요약**

| 구분            | 복사 생성자 (Copy Constructor)       | 대입 연산자 (Copy Assignment Operator)    |
| ------------- | ------------------------------- | ------------------------------------ |
| **호출 시점**     | 새로운 객체를 만들 때                    | 이미 존재하는 객체에 다른 객체의 값을 덮어쓸 때          |
| **형태**        | `Class(const Class& rhs)`       | `Class& operator=(const Class& rhs)` |
| **생성/초기화 여부** | 새 객체 생성 + 복사                    | 기존 객체의 리소스 정리 후 복사                   |
| **기존 데이터 처리** | 없음 (새로 생성)                      | 필요 (기존 자원 해제 등)                      |
| **자동 호출 예시**  | `Class b = a;`, 함수 인자/리턴 시 값 전달 | `b = a;`                             |
| **리턴 타입**     | 없음 (생성자)                        | `Class&` (체이닝 지원)                    |

---

## ⚙️ 1️⃣ **복사 생성자 (Copy Constructor)**

> 새로운 객체를 **기존 객체로 초기화할 때 호출**됨.

```cpp
class Player {
    int hp;
public:
    Player(int h) : hp(h) {}
    Player(const Player& rhs) {  // 복사 생성자
        hp = rhs.hp;
        std::cout << "복사 생성자 호출\n";
    }
};

int main() {
    Player p1(100);
    Player p2 = p1;  // ✅ 복사 생성자 호출
}
```

📍 **호출 시점**

* 객체 초기화(`Player p2 = p1;`)
* 함수 인자로 값 전달 시(`func(p1);`)
* 함수에서 객체를 값으로 반환할 때 (RVO 최적화 전)

---

## ⚙️ 2️⃣ **대입 연산자 (Copy Assignment Operator)**

> **이미 생성된 객체에 다른 객체의 내용을 복사할 때 호출됨.**

```cpp
class Player {
    int hp;
public:
    Player(int h) : hp(h) {}
    Player& operator=(const Player& rhs) {
        if (this == &rhs) return *this; // 자기 자신 대입 방지
        hp = rhs.hp;
        std::cout << "대입 연산자 호출\n";
        return *this;
    }
};

int main() {
    Player p1(100);
    Player p2(50);
    p2 = p1; // ✅ 복사 대입 연산자 호출
}
```

📍 **호출 시점**

* 객체가 이미 존재함 (`p2`는 이미 생성된 상태)
* **새 객체 생성 X**
* 기존 데이터가 있으면 정리 후 복사해야 함.

---

## ⚙️ 3️⃣ **실제 동작 차이**

```cpp
Player p1(100);
Player p2 = p1; // 복사 생성자 (새 객체 생성)
Player p3(50);
p3 = p1;        // 대입 연산자 (기존 객체 덮어씀)
```

* `p2 = p1;` → **생성 + 초기화 (copy constructor)**
* `p3 = p1;` → **기존 객체에 값 복사 (assignment operator)**

즉,

> 복사 생성자는 “새로 태어나는 순간 복사”
> 대입 연산자는 “이미 있는 놈 덮어쓰기”

---

## ⚙️ 4️⃣ **얕은 복사 vs 깊은 복사**

얕은 복사는 단순 포인터 복사라서 위험하다.
복사 생성자와 대입 연산자 둘 다 **깊은 복사(deep copy)**가 필요할 수 있다.

```cpp
class Buffer {
    char* data;
public:
    Buffer(const char* str) {
        data = new char[strlen(str) + 1];
        strcpy(data, str);
    }
    // 복사 생성자
    Buffer(const Buffer& rhs) {
        data = new char[strlen(rhs.data) + 1];
        strcpy(data, rhs.data);
    }
    // 대입 연산자
    Buffer& operator=(const Buffer& rhs) {
        if (this == &rhs) return *this;
        delete[] data;  // 기존 자원 정리
        data = new char[strlen(rhs.data) + 1];
        strcpy(data, rhs.data);
        return *this;
    }
    ~Buffer() { delete[] data; }
};
```

📍 **복사 생성자**
→ `data` 새로 할당. 기존 데이터 없음.

📍 **대입 연산자**
→ 기존 `data` 해제 후 새로 복사해야 함.

---

## ⚙️ 5️⃣ **컴파일러의 기본 제공 (The Rule of 3)**

컴파일러는 기본적으로 다음을 자동 생성함:

1. 복사 생성자
2. 복사 대입 연산자
3. 소멸자

하지만 클래스가 **리소스(동적 메모리, 파일 핸들 등)**를 직접 관리한다면
👉 반드시 세 개 모두 직접 정의해야 함.
(하나만 정의하고 나머지를 기본값 쓰면 **얕은 복사로 메모리 중복 해제 발생**)

이게 바로 **Rule of 3**,
C++11 이후 이동 생성자/대입 연산자까지 포함하면 **Rule of 5**로 확장된다.

---

## ⚙️ 6️⃣ **move와의 비교 (참고)**

| 구분       | 복사 생성자        | 이동 생성자               |
| -------- | ------------- | -------------------- |
| 자원 복사 방식 | 새 메모리 할당 후 복사 | 포인터 소유권 이전 (얕은 이동)   |
| 비용       | 높음            | 매우 낮음                |
| 호출 조건    | l-value 복사 시  | r-value (임시 객체) 이동 시 |

---

## 🎯 **면접용 정리**

> 복사 생성자는 **새 객체를 기존 객체로 초기화할 때**,
> 대입 연산자는 **이미 존재하는 객체에 다른 객체를 덮어쓸 때** 호출됩니다.
>
> 복사 생성자는 “생성 + 복사”이고,
> 대입 연산자는 “기존 자원 정리 + 복사”입니다.
>
> 두 연산 모두 얕은 복사로 인한 문제를 막기 위해
> 리소스를 소유하는 클래스에서는 반드시 깊은 복사로 구현해야 합니다.

---

### 🔥 꼬리질문 예상

1. “복사 생성자와 대입 연산자 중 어떤 게 먼저 호출되나요?”
   → 객체 생성 시 복사 생성자, 이미 존재하면 대입 연산자.

2. “자기 자신에 대한 대입(`a = a;`)을 처리 안 하면 어떤 문제가 생기나요?”
   → 자원 해제 후 자기 자신을 참조 → 크래시 (self-assignment 방지 필요).

3. “복사 생성자 생략 가능한 경우는요?”
   → RVO(Return Value Optimization)나 이동 시맨틱(`std::move()`)이 적용될 때.

4. “Rule of 3 / Rule of 5란?”
   → 자원 관리 클래스는 복사 생성자, 대입 연산자, 소멸자를 함께 정의해야 한다는 원칙.
   (C++11 이후엔 이동 생성자/대입 연산자까지 포함해 Rule of 5)


</details>



## 18. 복사 생성자를 금지하고 싶다면 어떻게 하나요?

<details close>
<summary><strong>💡 자세한 정답</strong></summary>

<div style="background-color:#f6f8fa; padding:15px; border-radius:10px;">

## 💡 **복사 생성자를 금지하는 이유**

> 어떤 클래스는 **복사되면 안 되는 자원**을 소유하기 때문이다.
> 예:
>
> * 파일 핸들, 소켓, DB 연결, mutex, thread, IOCP session 등
> * 복사하면 “자원 중복 소유” → double free, use-after-free 같은 **심각한 버그** 발생

---

## ⚙️ 1️⃣ **C++11 이후 방식 — `= delete`**

```cpp
class Socket {
public:
    Socket() = default;
    Socket(const Socket&) = delete;            // 복사 생성자 금지
    Socket& operator=(const Socket&) = delete; // 대입 연산자 금지
};
```

### ✅ 특징

* 명시적으로 “이 함수는 사용할 수 없다”고 선언.
* **컴파일 타임에 오류 발생.**
* 읽기만 봐도 **복사 금지 의도**가 명확함.
* “금지된 함수라도 링크 단계까지 가지 않음 (컴파일러가 막음).”

---

### 🧩 예시 — 복사 시도 시

```cpp
Socket s1;
Socket s2 = s1; // ❌ error: use of deleted function 'Socket::Socket(const Socket&)'
```

---

## ⚙️ 2️⃣ **C++11 이전 방식 — private 선언 + 정의 안 함**

```cpp
class Socket {
private:
    Socket(const Socket&);            // 선언만
    Socket& operator=(const Socket&); // 정의 없음
};
```

### ⚠️ 단점

* 복사 시도 시 “private 접근 불가” 에러만 나고,
  의도가 명확하지 않음 (delete보다 불친절).
* 클래스 내부(friend 등)에서는 여전히 복사 가능.
* 유지보수 어려움 (컴파일러 메시지 모호함).

👉 그래서 C++11 이후엔 전부 `= delete`로 교체됐다.

---

## ⚙️ 3️⃣ **이동은 허용하는 패턴 (자원 이전용 클래스)**

```cpp
class Socket {
public:
    Socket() = default;
    Socket(const Socket&) = delete;            // 복사 금지
    Socket& operator=(const Socket&) = delete; // 복사 대입 금지

    Socket(Socket&&) noexcept = default;            // 이동 허용
    Socket& operator=(Socket&&) noexcept = default; // 이동 대입 허용
};
```

→ 복사는 막되, **소유권 이전(move)** 은 허용.
→ “유일한 소유권” 패턴: **unique_ptr**, **socket handle**, **thread** 등이 전형적인 예.

---

## ⚙️ 4️⃣ **예시: 복사 금지 클래스의 활용**

```cpp
class ThreadGuard {
    std::thread t;
public:
    explicit ThreadGuard(std::thread th) : t(std::move(th)) {}
    ~ThreadGuard() { if (t.joinable()) t.join(); }

    ThreadGuard(const ThreadGuard&) = delete;            // 복사 금지
    ThreadGuard& operator=(const ThreadGuard&) = delete; // 복사 금지
};
```

* `ThreadGuard` 객체를 복사하면 스레드를 두 번 join할 수도 있음 → **UB**
* 따라서 복사 자체를 금지해야 함.

---

## ⚙️ 5️⃣ **std::unique_ptr도 같은 원리**

```cpp
std::unique_ptr<int> p1 = std::make_unique<int>(10);
std::unique_ptr<int> p2 = p1;       // ❌ 복사 금지
std::unique_ptr<int> p3 = std::move(p1); // ✅ 이동 허용
```

→ 내부적으로 복사 생성자/대입 연산자가 `= delete` 되어 있다.
→ 대신 `move` 생성자만 허용됨.

---

## 🎯 **면접용 정리**

> 복사 생성자를 금지하려면 `MyClass(const MyClass&) = delete;` 로 명시합니다.
> 이렇게 하면 컴파일러가 복사를 **컴파일 타임에 차단**합니다.
>
> 복사는 보통 자원 중복 소유를 유발할 수 있기 때문에,
> 파일 핸들, 소켓, 쓰레드 등 **“소유권이 유일해야 하는 객체”**는 반드시 복사를 막아야 합니다.
>
> C++11 이전에는 `private` 선언으로 막았지만,
> 지금은 `= delete`가 더 명시적이고 안전한 방법입니다.

---

### 🔥 꼬리질문 예상

1. “복사 금지인데 이동은 가능하게 하려면?”
   → 복사 생성자/대입만 `= delete`, 이동 생성자/대입은 `= default`.

2. “`delete`와 `default`의 차이는?”
   → `delete`: 사용 금지.
   `default`: 컴파일러의 기본 구현 강제 사용.

3. “`= delete`는 다른 함수에도 쓸 수 있나요?”
   → 가능. 예를 들어, 특정 타입만 받는 함수 제한:

   ```cpp
   void Foo(int) = delete;   // int 인자 금지
   void Foo(double);         // double만 허용
   ```


</details>



## 19. 복사 생성자와 이동 생성자의 호출 우선순위를 설명하세요.

<details close>
<summary><strong>💡 자세한 정답</strong></summary>

<div style="background-color:#f6f8fa; padding:15px; border-radius:10px;">

## 💡 **복사 생성자 vs 이동 생성자 — 호출 우선순위**

| 상황                      | 객체 특성          | 호출되는 생성자                        |
| ----------------------- | -------------- | ------------------------------- |
| **R-value (임시 객체)**     | 이동 가능 타입       | ✅ **이동 생성자 (Move Constructor)** |
| **R-value (임시 객체)**     | 이동 불가능 타입      | ✅ **복사 생성자 (Copy Constructor)** |
| **L-value (이름 있는 객체)**  | 복사 or 이동 모두 가능 | ✅ **복사 생성자**                    |
| **컴파일러 최적화 (RVO/NRVO)** | 임시 복사 제거       | ✅ **생성자 호출 자체 생략**              |

---

## ⚙️ 1️⃣ **기본 규칙**

> R-value(임시 객체)는 이동(move),
> L-value(이름 있는 객체)는 복사(copy).

```cpp
class Obj {
public:
    Obj() { std::cout << "기본 생성\n"; }
    Obj(const Obj&) { std::cout << "복사 생성\n"; }
    Obj(Obj&&) noexcept { std::cout << "이동 생성\n"; }
};

Obj MakeObj() {
    Obj temp;
    return temp;  // R-value (임시 객체)
}

int main() {
    Obj a = MakeObj();  // ✅ 이동 생성 (복사 X)
    Obj b(a);           // ✅ 복사 생성 (a는 L-value)
}
```

**출력:**

```
기본 생성
이동 생성
복사 생성
```

---

## ⚙️ 2️⃣ **이동 생성자 우선 규칙**

* 컴파일러는 다음 순서로 생성자를 선택한다:

```
if (T has move constructor)
    use move constructor;
else if (T has copy constructor)
    use copy constructor;
else
    compile error;
```

즉, **이동 생성자(move constructor)** 가 정의되어 있으면 R-value에서 **항상 우선 호출**된다.

---

### 📘 예시 — 이동 금지 시 복사로 fallback

```cpp
class Obj {
public:
    Obj(const Obj&) { std::cout << "복사 생성\n"; }
    Obj(Obj&&) = delete; // 이동 금지
};

Obj MakeObj() { return Obj(); }

int main() {
    Obj a = MakeObj(); // ✅ 이동 불가 → 복사 생성 호출
}
```

출력:

```
복사 생성
```

---

## ⚙️ 3️⃣ **복사 생성자 삭제 시 이동으로 대체**

```cpp
class Obj {
public:
    Obj(Obj&&) noexcept { std::cout << "이동 생성\n"; }
    Obj(const Obj&) = delete;
};

Obj MakeObj() { return Obj(); }

int main() {
    Obj a = MakeObj(); // ✅ 복사 불가 → 이동 생성 사용
}
```

출력:

```
이동 생성
```

---

## ⚙️ 4️⃣ **복사도 이동도 금지 시**

→ 컴파일 에러.

```cpp
class Obj {
public:
    Obj(const Obj&) = delete;
    Obj(Obj&&) = delete;
};
Obj MakeObj() { return Obj(); }

int main() {
    Obj a = MakeObj(); // ❌ 컴파일 에러
}
```

---

## ⚙️ 5️⃣ **함수 리턴 시 (RVO / NRVO 적용 여부)**

> 컴파일러가 최적화로 **복사나 이동 자체를 없앨 수도 있다.**

```cpp
Obj MakeObj() {
    return Obj(); // RVO 적용
}

int main() {
    Obj a = MakeObj(); // ✅ 생성자 1번만 호출됨 (복사/이동 생략)
}
```

* **RVO(Return Value Optimization)**: C++17부터 **보장된 최적화(Guaranteed RVO)**
  → 복사/이동 생성자 호출조차 없음
  → 오직 “직접 생성”만 일어남.

---

## ⚙️ 6️⃣ **noexcept와 이동의 관계**

> 이동 생성자가 `noexcept`가 아니면, **컨테이너(std::vector 등)** 가 복사를 선택할 수 있다.

```cpp
class Obj {
public:
    Obj(const Obj&) { std::cout << "복사 생성\n"; }
    Obj(Obj&&) { std::cout << "이동 생성\n"; } // noexcept 없음
};

int main() {
    std::vector<Obj> v;
    v.push_back(Obj()); // 복사 생성 호출될 수도 있음 (이동이 예외 안전하지 않다고 판단)
}
```

→ 즉, “이동 생성자는 반드시 `noexcept`로 선언하라.”
→ `std::move` 써도 `noexcept` 아니면 컨테이너가 복사 쓸 수 있다.

---

## 🎯 **면접용 요약**

> 이동 생성자와 복사 생성자의 선택은 **값이 L-value인지 R-value인지**로 결정됩니다.
>
> * R-value면 **이동 생성자(move constructor)** 가 우선,
> * 이동이 불가능할 경우 **복사 생성자(copy constructor)** 로 대체됩니다.
> * L-value는 항상 복사 생성자를 사용합니다.
>
> C++17부터는 **RVO(Return Value Optimization)** 으로
> 임시 객체 반환 시 이동/복사 자체가 생략되며,
> 성능과 예외 안전성을 위해 **이동 생성자는 `noexcept`로 선언하는 게 원칙**입니다.

---

### 🔥 꼬리질문 예상

1. “L-value를 이동하려면 어떻게 하나요?”
   → `std::move()`로 R-value 캐스팅해야 이동 생성자 호출 가능.

   ```cpp
   Obj b = std::move(a);
   ```

2. “RVO와 이동 생성자 중 뭐가 더 우선인가요?”
   → RVO가 우선. 복사/이동 생성자 호출 자체가 생략됨.

3. “이동 생성자 정의 안 하면 어떻게 되나요?”
   → 컴파일러가 자동 생성. 단, 복사 생성자나 소멸자 정의 시 자동 생성 안 될 수도 있음.

4. “컨테이너에서 이동이 안 되는 이유?”
   → 이동 생성자가 `noexcept` 아니면 복사로 fallback 됨 (예외 안전성 보장 위해).



</details>



## 20. 스택(stack)과 힙(heap)의 차이점은 무엇인가요?

<details close>
<summary><strong>💡 자세한 정답</strong></summary>

<div style="background-color:#f6f8fa; padding:15px; border-radius:10px;">


## 💡 **스택(Stack) vs 힙(Heap)**

| 구분               | **스택(Stack)**        | **힙(Heap)**                                     |
| ---------------- | -------------------- | ----------------------------------------------- |
| **관리 주체**        | 컴파일러 / 시스템 자동 관리     | 프로그래머 직접 관리 (`new`, `malloc`, `free`, `delete`) |
| **메모리 위치**       | 연속된 메모리 영역 (LIFO 구조) | 자유롭게 할당되는 영역 (비연속적)                             |
| **할당 시점**        | 컴파일 시 or 함수 호출 시 자동  | 런타임 시 명시적 할당                                    |
| **해제 시점**        | 함수 종료 시 자동 해제        | 명시적 해제 필요 (안 하면 누수)                             |
| **생성 속도**        | 매우 빠름 (포인터 이동만으로 관리) | 느림 (OS 요청 필요)                                   |
| **메모리 크기 한계**    | 작음 (보통 수 MB)         | 큼 (시스템 메모리 한도까지)                                |
| **수명(lifetime)** | 스코프 내 존재             | 프로그래머가 해제할 때까지 유지                               |
| **대표 예시**        | 지역 변수, 함수 인자         | 동적 배열, 객체, 버퍼                                   |

---

## ⚙️ 1️⃣ **스택(Stack) 메모리**

> 함수 호출 시 자동으로 생성되는 **지역 변수(Local Variable)** 들이 저장되는 공간.

```cpp
void Foo() {
    int x = 10;    // 스택에 저장
    int arr[3] = {1, 2, 3}; // 스택에 연속적으로 저장
} // 함수 끝나면 x, arr 모두 자동 해제
```

📌 특징:

* **LIFO (Last In, First Out)** 구조
* 함수가 리턴하면 자동으로 pop됨.
* 매우 빠르고, 메모리 단편화 없음.
* 하지만 **큰 데이터나 동적 크기 데이터**에는 부적합.

---

### ⚠️ 스택의 한계

```cpp
void TooBig() {
    int arr[10000000]; // 약 40MB → Stack overflow 가능
}
```

→ 스택은 보통 **1MB~8MB** 정도로 제한됨.
→ 초과 시 **Stack Overflow** (프로세스 크래시).

---

## ⚙️ 2️⃣ **힙(Heap) 메모리**

> 프로그램 실행 중 동적으로 메모리를 요청(`new`, `malloc`)해서 사용하는 공간.

```cpp
void Foo() {
    int* p = new int(10);       // 힙에 메모리 할당
    int* arr = new int[3]{1,2,3};
    delete p;                   // 반드시 해제
    delete[] arr;               // 배열은 delete[]
}
```

📌 특징:

* **동적 할당 (Dynamic Allocation)**
  → 런타임 시 크기를 결정 가능.
* **수동 관리 필요**
  → `delete` 안 하면 **메모리 누수(memory leak)**.
* **비연속적 구조**
  → 할당·해제 반복 시 **단편화(fragmentation)** 발생 가능.

---

### 🧠 예시 비교

```cpp
void Example() {
    int a = 10;           // 스택: 자동 해제
    int* b = new int(20); // 힙: 명시적 해제 필요
} // a는 자동 해제, b는 메모리 누수 발생
```

---

## ⚙️ 3️⃣ **메모리 해제 책임**

| 구분     | 해제 주체 | 해제 시점         |
| ------ | ----- | ------------- |
| **스택** | 시스템   | 함수 종료 시 자동    |
| **힙**  | 프로그래머 | `delete` 호출 시 |

---

## ⚙️ 4️⃣ **성능 차이**

| 구분            | 이유                              |
| ------------- | ------------------------------- |
| **스택이 빠른 이유** | 단순히 스택 포인터(SP) 한 칸 내리거나 올리면 끝   |
| **힙이 느린 이유**  | OS 메모리 관리자를 거쳐야 함 (프리 리스트 탐색 등) |

즉,

* 스택: **O(1)** 수준의 빠른 할당
* 힙: 내부 탐색, 병합, 분할 등으로 **상대적으로 느림**

---

## ⚙️ 5️⃣ **객체 수명(lifetime)과 연결**

```cpp
class Player {
public:
    Player() { std::cout << "생성\n"; }
    ~Player() { std::cout << "소멸\n"; }
};

void StackObj() {
    Player p; // 스택에 생성
} // 스코프 종료 시 자동 소멸

void HeapObj() {
    Player* p = new Player(); // 힙에 생성
    delete p;                 // 직접 해제 필요
}
```

📌 결과:

```
StackObj(): 생성 → 스코프 끝 → 자동 소멸
HeapObj(): 생성 → delete 필요 (안 하면 누수)
```

---

## ⚙️ 6️⃣ **서버 프로그래밍에서의 적용**

| 영역                                   | 사용 메모리            |
| ------------------------------------ | ----------------- |
| 세션 객체, 버퍼, 패킷                        | 힙 (동적 크기)         |
| 함수 내부의 카운터, 임시 변수                    | 스택                |
| IOCP Completion Key, Thread-local 변수 | 스택 or TLS 기반      |
| 대규모 메모리 풀                            | 힙 기반 + Pooling 관리 |

→ 즉, 고성능 서버는 **힙 할당을 직접 쓰지 않고, 재사용 가능한 Pooling 구조**로 관리함.

---

## 🎯 **면접용 정리**

> 스택은 함수 호출 시 자동으로 할당·해제되는 고정적이고 빠른 메모리 공간이고,
> 힙은 런타임에 프로그래머가 직접 관리해야 하는 동적 메모리 공간입니다.
>
> 스택은 수명이 짧고 크기가 제한적이지만, 빠르고 안전합니다.
> 힙은 유연하고 크지만, 느리고 메모리 누수 위험이 있습니다.
>
> 따라서 서버 개발에서는 힙 할당 자체를 줄이고
> 메모리 풀, 객체 재사용 구조를 통해 성능을 최적화합니다.

---

### 🔥 꼬리질문 예상

1. “힙 메모리 누수는 왜 위험하나요?”
   → 해제되지 않으면 시스템 메모리 고갈 → 장기 서버에서 치명적.

2. “스택 변수의 주소를 리턴하면 안 되는 이유는?”
   → 함수 종료 시 스택 프레임 해제 → Dangling Pointer 발생.

3. “스택은 왜 빠르나요?”
   → 단순 포인터 연산으로 메모리 접근, 힙은 OS 호출 필요.

4. “C++에서는 스택/힙을 명시적으로 구분할 수 있나요?”
   → 직접 제어는 불가능하지만, `new`/`delete` 사용 여부로 사실상 구분됨.


</details>



## 21. placement new는 언제 사용하나요?

<details close>
<summary><strong>💡 자세한 정답</strong></summary>

<div style="background-color:#f6f8fa; padding:15px; border-radius:10px;">

## 💡 **placement new란?**

> 이미 확보된 메모리 공간(포인터)에 **객체를 직접 생성하는 연산자 오버로딩 형태의 new**.
>
> 즉, **메모리 할당은 하지 않고, 생성자만 호출하는 new**.

---

### ⚙️ 문법

```cpp
#include <new>  // placement new 정의 헤더

void* mem = malloc(sizeof(MyClass));
MyClass* obj = new (mem) MyClass(); // placement new
```

📌 차이점

* 일반 `new`: **할당 + 생성자 호출**
* placement `new`: **이미 있는 메모리에 생성자만 호출**

---

## ⚙️ 1️⃣ **일반 new와 비교**

| 구분        | 일반 new                    | placement new                 |
| --------- | ------------------------- | ----------------------------- |
| 메모리 할당    | O (operator new로 OS 힙 호출) | X (이미 할당된 공간 사용)              |
| 생성자 호출    | O                         | O                             |
| delete 방식 | `delete`                  | 소멸자 직접 호출 (`obj->~MyClass()`) |
| 사용 목적     | 동적 객체 생성                  | 메모리 풀, 커스텀 할당, 컨테이너 내부 객체 재생성 |

---

## ⚙️ 2️⃣ **언제 쓰는가 (실무 기준)**

| 사용 상황                  | 설명                                               |
| ---------------------- | ------------------------------------------------ |
| **1️⃣ 메모리 풀 기반 객체 생성** | 이미 확보된 블록 위에 객체를 “덮어써서” 생성                       |
| **2️⃣ 커스텀 메모리 관리**     | new/delete 대신 PoolAllocator, ArenaAllocator 사용 시 |
| **3️⃣ 성능 최적화**         | 할당/해제 반복 제거 (힙 접근 방지)                            |
| **4️⃣ 객체 재생성**         | 기존 버퍼 영역 재활용 (e.g. 게임 엔진 ECS, job 시스템)           |

---

### 🧩 예시 — 메모리 풀에서 객체를 생성할 때

```cpp
class Player {
public:
    Player(int id) : _id(id) { std::cout << "생성자\n"; }
    ~Player() { std::cout << "소멸자\n"; }
private:
    int _id;
};

char pool[sizeof(Player) * 10]; // 메모리 풀
Player* p1 = new (&pool[0]) Player(1); // placement new
p1->~Player(); // 명시적 소멸
```

> `&pool[0]`은 **메모리만 제공**,
> `new (addr) Player()`는 그 위에 **생성자 호출**.
>
> 💡 delete 하면 안 된다. (할당 자체를 안 했기 때문)
> 반드시 `obj->~Player()`로 **직접 소멸자 호출**.

---

## ⚙️ 3️⃣ **STL 내부에서도 자주 쓰인다**

예를 들어 `std::vector`나 `std::list`는
요소 추가 시 실제로 이렇게 동작한다 👇

```cpp
void push_back(const T& val) {
    new (&_storage[_size]) T(val); // placement new
    ++_size;
}
```

* 벡터는 이미 메모리를 `reserve()`로 확보해둠
* 거기서 **객체만 새로 생성**할 때 placement new 사용

즉, **STL 컨테이너의 핵심도 placement new 기반**이야.

---

## ⚙️ 4️⃣ **주의할 점**

| 주의사항            | 설명                                      |
| --------------- | --------------------------------------- |
| ❌ delete 사용 금지  | 할당을 안 했으므로 delete 하면 undefined behavior |
| ✅ 명시적 소멸자 호출 필요 | `obj->~T();` 직접 호출해야 함                  |
| ⚠️ 정렬(align) 보장 | 전달한 주소가 타입의 정렬 요건을 만족해야 함               |
| ⚠️ 예외 안전성 주의    | 생성자에서 예외 나면 소멸자 직접 처리 필요                |

---

## ⚙️ 5️⃣ **커스텀 Allocator에서의 활용 예시**

```cpp
template<typename T>
class PoolAllocator {
    void* pool;
public:
    PoolAllocator() { pool = malloc(sizeof(T) * 100); }

    T* Allocate() {
        return new ((char*)pool) T(); // placement new
    }

    void Free(T* obj) {
        obj->~T(); // 명시적 소멸
    }
};
```

➡️ IOCP 서버의 `JobQueue`, `SendBufferManager`, `MemoryPool` 같은 구조는
실제로 이런 방식으로 **“메모리 할당과 객체 생성을 분리”**한다.

---

## ⚙️ 6️⃣ **placement new 오버로딩 구조**

C++에서는 `operator new` 자체가 오버로딩되어 있음:

```cpp
void* operator new(size_t size, void* ptr) noexcept {
    return ptr; // 그냥 전달받은 주소 리턴
}
```

즉, “메모리를 새로 만들지 않고, 지정된 주소로 생성자만 호출하겠다”는 문법적 장치임.

---

## 🎯 **면접용 정리**

> placement new는 **이미 할당된 메모리 공간에 객체를 직접 생성**할 때 사용합니다.
>
> 일반 new처럼 메모리를 새로 할당하지 않고,
> 단순히 **생성자만 호출하기 때문에 빠르고 효율적**입니다.
>
> 메모리 풀이나 커스텀 할당기, STL 컨테이너 내부 구현 등에서 사용되며,
> delete 대신 반드시 **소멸자를 직접 호출**해야 합니다.

---

### 🔥 꼬리질문 예상

1. “placement new와 일반 new의 차이는?”
   → 일반 new는 메모리 + 생성자 호출, placement new는 생성자 호출만.

2. “placement new로 만든 객체는 어떻게 해제하나요?”
   → `obj->~T();`로 명시적 소멸, delete 사용 X.

3. “메모리 풀에서 placement new를 쓰는 이유는?”
   → 이미 확보한 블록에 객체를 덮어쓰기 위해 (힙 호출 제거, 단편화 방지).

4. “placement new 사용 시 주의점?”
   → 정렬(align) 보장, delete 금지, 예외 시 소멸자 호출 누락 주의.

</details>

## 22. 가상 함수(virtual function)는 내부적으로 어떻게 동작하나요? vtable과 vptr 개념을 포함해서 설명해보세요.

<details close>
<summary><strong>💡 자세한 정답</strong></summary>

<div style="background-color:#f6f8fa; padding:15px; border-radius:10px;">

## 💡 **가상 함수의 핵심 개념**

> 가상 함수는 **런타임 다형성(Runtime Polymorphism)**을 구현하기 위한 메커니즘으로,
> 실제 객체 타입에 따라 **함수 호출을 동적으로 결정**하기 위해 사용된다.

---

## ⚙️ 1️⃣ **vtable과 vptr 구조**

| 구성 요소                      | 설명                                                |
| -------------------------- | ------------------------------------------------- |
| **vtable (Virtual Table)** | 클래스 단위로 존재하는 **함수 포인터 배열**. 각 가상 함수의 주소를 저장.      |
| **vptr (Virtual Pointer)** | 객체마다 하나씩 존재하는 **vtable을 가리키는 포인터**. 생성자에서 자동 설정됨. |

---

### 📘 예시 코드

```cpp
class Base {
public:
    virtual void Speak() { std::cout << "Base\n"; }
};

class Derived : public Base {
public:
    void Speak() override { std::cout << "Derived\n"; }
};
```

| 클래스     | vtable 내용                |
| ------- | ------------------------ |
| Base    | `Speak → Base::Speak`    |
| Derived | `Speak → Derived::Speak` |

객체 생성 시 다음처럼 동작 👇

```cpp
Derived d;
Base* ptr = &d;
ptr->Speak(); // Derived::Speak 호출
```

➡️ 실행 시점에 `ptr`이 가리키는 **vptr → Derived의 vtable**을 참조
➡️ vtable 안의 함수 포인터를 따라가 `Derived::Speak()`가 호출된다.

---

## ⚙️ 2️⃣ **동작 과정**

1. 클래스에 `virtual` 함수가 하나라도 있으면 **vtable이 생성**됨.
2. 객체 생성 시 생성자가 **vptr을 해당 클래스의 vtable로 초기화**.
3. 가상 함수 호출 시,
   → 컴파일 타임에는 **오프셋**만 알고,
   → 런타임에 vptr이 가리키는 vtable에서 **실제 함수 주소를 읽어 호출**.

---

### 📊 호출 시 내부적으로 일어나는 일

```text
Base* p = new Derived();
p->Speak();
```

실제 내부 동작 (개념도)

```
p (object)
 └── vptr ──► [ vtable(Derived) ]
                     └── [0] -> &Derived::Speak
```

➡️ `p->Speak()` 호출 시
`(*(p->vptr)[0])(p);` 형태로 변환되어 실행됨.

---

## ⚙️ 3️⃣ **생성자 / 소멸자에서의 주의점**

| 상황            | 동작                                               |
| ------------- | ------------------------------------------------ |
| **생성자 내부 호출** | vptr이 아직 완전히 초기화되지 않아 **자기 클래스 버전**만 호출됨.        |
| **소멸자 내부 호출** | 이미 vptr이 **기본 클래스 vtable로 되돌아감** → 파생 함수 호출 안 됨. |

---

## ⚙️ 4️⃣ **메모리 오버헤드**

| 항목   | 설명                             |
| ---- | ------------------------------ |
| 객체당  | vptr(보통 8바이트) 추가               |
| 클래스당 | vtable 하나 생성                   |
| 호출 시 | 간접 호출(포인터 dereference 1회) 오버헤드 |

---

## ⚙️ 5️⃣ **vtable 생성 위치**

* 클래스 단위로 전역 메모리에 존재 (`.rdata` 영역)
* vptr은 객체 내부에 삽입되어 있음 (대체로 첫 번째 멤버 위치)

---

## ⚙️ 6️⃣ **다중 상속 시 vtable 구조**

다중 상속 시 각 기본 클래스마다 vptr이 따로 존재하며,
캐스팅 시 offset 조정이 필요하다.

```cpp
class A { virtual void f(); };
class B { virtual void g(); };
class C : public A, public B {};
```

➡️ `C` 객체 안에는 `A`용 vptr, `B`용 vptr 두 개 존재.

---

## 🎯 **면접용 정리**

> 가상 함수는 **객체마다 vptr을 통해 vtable을 참조**하고,
> 호출 시 vtable에서 **실제 함수 주소를 찾아 동적 바인딩**을 수행합니다.
>
> 즉, **컴파일 타임에 결정되지 않은 함수 호출을 런타임에 결정**하게 하는 구조입니다.

---

### 🔥 꼬리질문 예상

1. **Q. 가상 함수 호출 시 비용이 있나요?**
   → 함수 포인터 간접 참조 1회 오버헤드 (인라인 불가).

2. **Q. 생성자 안에서 가상 함수 호출 시 어떤 함수가 호출되나요?**
   → 현재 클래스의 버전만 호출. (아직 vptr이 파생 클래스용으로 세팅 전)

3. **Q. 다중 상속 시 vtable은 어떻게 되나요?**
   → 각 상속 체인마다 vtable 별도로 존재. vptr도 여러 개 생김.

4. **Q. vtable은 어디에 저장되나요?**
   → 전역 상수 메모리 영역(.rdata)에 클래스별로 하나씩 존재.

</details>



## 23. C++에서 다중 상속을 할 때 vtable이 복잡해진다고 합니다. 왜 그런가요?

<details close>
<summary><strong>💡 자세한 정답</strong></summary>

<div style="background-color:#f6f8fa; padding:15px; border-radius:10px;">

## 💡 **다중 상속에서의 vtable 복잡성**

> 다중 상속(Multiple Inheritance) 시,
> 각 기본 클래스(Base Class)의 **가상 함수 테이블(vtable)** 과
> **가상 함수 포인터(vptr)** 관리가 **복수 개로 분리되기 때문**이다.

---

## ⚙️ 1️⃣ **단일 상속 구조에서는 단순함**

단일 상속의 경우,
모든 가상 함수는 하나의 vtable에 저장되며
객체는 vptr 하나만 가진다.

```cpp
class Base {
public:
    virtual void Foo() {}
};

class Derived : public Base {
public:
    void Foo() override {}
};
```

* Base → vtable: `{ &Base::Foo }`
* Derived → vtable: `{ &Derived::Foo }`
* Derived 객체 안에는 vptr **하나만 존재**

---

## ⚙️ 2️⃣ **다중 상속 시의 구조**

다중 상속에서는 각 Base 클래스가 **독립적인 vtable을 가짐**
→ 파생 클래스는 이들을 **모두 상속**하므로 **vptr이 여러 개** 생긴다.

```cpp
class A { virtual void f(); };
class B { virtual void g(); };
class C : public A, public B {
    void f() override;
    void g() override;
};
```

| 클래스   | vtable 내용          |
| ----- | ------------------ |
| **A** | f → A::f           |
| **B** | g → B::g           |
| **C** | f → C::f, g → C::g |

> C 객체 내부에는
> A용 vptr, B용 vptr **2개**가 들어 있다.

---

### 📘 내부 메모리 구조 개념도

```
[C 객체 메모리]
 ├─ A 부분 (vptr_A → vtable_A)
 ├─ B 부분 (vptr_B → vtable_B)
```

➡️ 각각의 부분이 자신의 vtable을 가리킴.
➡️ 포인터 캐스팅 시 offset 조정 필요.

---

## ⚙️ 3️⃣ **포인터 캐스팅 복잡성**

다중 상속에서 `A*`, `B*`, `C*` 간 캐스팅 시
객체의 시작 주소가 달라질 수 있다.

```cpp
C obj;
A* pa = &obj; // obj의 시작 주소
B* pb = &obj; // + offset 만큼 떨어진 주소
```

* `A*`는 C 객체의 **첫 번째 부분**
* `B*`는 C 객체의 **두 번째 부분**
* vptr도 각각 별도 위치에 존재 → **offset 조정 필요**

---

## ⚙️ 4️⃣ **가상 함수 호출 시 동작**

```cpp
pb->g(); // B* 포인터로 호출
```

1. `pb`가 가리키는 B 부분의 vptr 확인
2. 해당 vtable(B chain)을 참조
3. `C::g()`로 오버라이드되어 있으면 해당 주소 호출

➡️ “어느 vptr을 타고 가느냐”에 따라 호출 함수 달라짐.

---

## ⚙️ 5️⃣ **가상 상속(Virtual Inheritance)과의 차이**

| 구분     | 일반 다중 상속    | 가상 상속             |
| ------ | ----------- | ----------------- |
| vptr 수 | 여러 개 존재     | 공유 가능             |
| 메모리 구조 | Base별 영역 분리 | 중복된 Base는 1개로 합쳐짐 |
| 복잡도    | 높음          | 더 높음(포인터 체인 따라감)  |

---

## ⚙️ 6️⃣ **다중 상속 시의 문제점**

| 문제                    | 설명                         |
| --------------------- | -------------------------- |
| 💥 **vptr 다중화**       | 객체당 여러 vptr 생성, 관리 복잡      |
| 💥 **주소 오프셋 계산**      | Base 간 캐스팅 시 주소 보정 필요      |
| 💥 **모호성(Ambiguity)** | 같은 함수명이 여러 Base에 있을 경우 충돌  |
| 💥 **메모리 오버헤드**       | vtable/vptr 중복으로 메모리 낭비 가능 |

---

## 🎯 **면접용 정리**

> 다중 상속에서는 각 Base 클래스가 고유한 vtable을 가지므로
> 파생 클래스 객체에 **여러 개의 vptr**이 생기고,
> 포인터 변환 시 **주소 오프셋 조정과 테이블 선택이 복잡**해집니다.

---

### 🔥 꼬리질문 예상

1. **Q. vptr이 여러 개 생기는 이유는?**
   → 각 Base 클래스의 가상 함수 테이블을 독립적으로 관리하기 때문.

2. **Q. 다중 상속 시 함수 호출은 어떻게 이뤄지나요?**
   → 호출 시 해당 Base 영역의 vptr을 따라 해당 vtable을 참조.

3. **Q. 다중 상속과 가상 상속의 차이는?**
   → 가상 상속은 중복 Base를 하나로 합치지만, 더 복잡한 포인터 구조를 가진다.

4. **Q. 다중 상속 시 모호성 문제는 어떻게 해결하나요?**
   → `A::Func()`처럼 **범위 지정 연산자(::)** 를 명시적으로 사용한다.

</details>

## 24. 순수 가상 함수(pure virtual)로만 이루어진 클래스를 ‘추상 클래스’라고 부르는데, 이런 구조를 왜 쓰나요?

<details close>
<summary><strong>💡 자세한 정답</strong></summary>

<div style="background-color:#f6f8fa; padding:15px; border-radius:10px;">

## 💡 **추상 클래스(Abstract Class)란?**

> 하나 이상의 **순수 가상 함수(pure virtual function)** 를 포함한 클래스.
> 객체를 직접 생성할 수 없고, **공통 인터페이스(규약)** 를 정의하기 위해 사용한다.

```cpp
class IShape {
public:
    virtual void Draw() = 0; // 순수 가상 함수
};
```

---

## ⚙️ 1️⃣ **순수 가상 함수의 의미**

```cpp
virtual void Func() = 0;
```

* `= 0` 은 **“이 함수는 반드시 파생 클래스에서 구현하라”**는 표시
* 기본 클래스는 인터페이스만 제공, 구현은 없음
* 이 함수가 하나라도 있으면 해당 클래스는 **추상 클래스**로 간주됨

---

## ⚙️ 2️⃣ **왜 사용하는가**

| 사용 목적               | 설명                                 |
| ------------------- | ---------------------------------- |
| **1️⃣ 인터페이스 정의**    | 공통된 함수 이름과 시그니처를 강제해 일관성 유지        |
| **2️⃣ 다형성 구현**      | 파생 클래스 객체를 기본 클래스 포인터로 다룰 수 있음     |
| **3️⃣ 설계 분리**       | “무엇을 할지”는 정의, “어떻게 할지”는 파생 클래스에 위임 |
| **4️⃣ 의존성 역전(DIP)** | 상위 모듈이 하위 구현에 의존하지 않도록 설계 가능       |

---

### 📘 예시

```cpp
class IWeapon {
public:
    virtual void Attack() = 0;
};

class Sword : public IWeapon {
public:
    void Attack() override { std::cout << "검 공격\n"; }
};

class Bow : public IWeapon {
public:
    void Attack() override { std::cout << "활 공격\n"; }
};
```

```cpp
void PlayerAttack(IWeapon* weapon) {
    weapon->Attack(); // 다형성 호출
}
```

➡️ `PlayerAttack()`은 `IWeapon` 인터페이스만 알고,
실제 공격 방식은 파생 클래스(Sword/Bow)가 정의.

---

## ⚙️ 3️⃣ **실무 사용 예**

| 분야          | 예시                                                 |
| ----------- | -------------------------------------------------- |
| **게임 엔진**   | `IRenderer`, `IComponent`, `IBehaviour` 등 인터페이스 정의 |
| **서버 구조**   | `ISession`, `IService`, `IPacketHandler` 형태로 추상화   |
| **그래픽스/DB** | API 추상화 (`IDatabase`, `ITexture`, `ISocket`)       |

---

## ⚙️ 4️⃣ **객체 생성 불가 이유**

> 추상 클래스는 구현이 완전하지 않기 때문에 **인스턴스화 불가능**.

```cpp
IShape s; // ❌ 오류: 순수 가상 함수 미구현
```

단, 포인터나 참조 형태로는 사용 가능하다.

```cpp
IShape* shape = new Circle(); // ✅ 가능
```

---

## ⚙️ 5️⃣ **상속 설계 시 장점**

| 장점     | 설명                      |
| ------ | ----------------------- |
| 확장성    | 새 기능은 파생 클래스에서 추가 구현 가능 |
| 결합도 감소 | 상위 모듈이 하위 구현에 종속되지 않음   |
| 일관성    | 팀 단위 협업 시 인터페이스 강제 가능   |

---

## 🎯 **면접용 정리**

> 추상 클래스는 **공통 인터페이스를 정의하고, 구현을 강제하기 위해** 사용됩니다.
>
> 이를 통해 **다형성(polymorphism)** 을 실현하고,
> **의존성 역전**과 **확장 가능한 구조 설계**가 가능합니다.

---

### 🔥 꼬리질문 예상

1. **Q. 추상 클래스와 인터페이스의 차이는?**
   → C++에서는 별도 구분이 없고, 순수 가상 함수만 가진 클래스를 인터페이스처럼 사용.

2. **Q. 추상 클래스를 인스턴스화할 수 없다는 의미는?**
   → 순수 가상 함수가 구현되지 않아 객체 완전성이 없기 때문.

3. **Q. 다형성과의 관계는?**
   → 기본 클래스 포인터로 파생 클래스 객체를 가리켜 동적 바인딩 수행.

4. **Q. 서버 구조에서의 사용 예?**
   → `ISession`, `IService` 같은 기반 추상 클래스로 공통 동작 정의 후 파생 클래스가 구현.

</details>

## 25. 클래스의 static 멤버 변수는 객체마다 하나씩 생기는 게 아니라 공유된다고 하는데, 실제로는 어디에 저장되고 어떻게 초기화하나요?

<details close>
<summary><strong>💡 자세한 정답</strong></summary>

<div style="background-color:#f6f8fa; padding:15px; border-radius:10px;">

## 💡 **static 멤버 변수의 본질**

> `static` 멤버 변수는 **클래스에 속하지만, 객체에 속하지 않는 변수**다.
> 즉, 모든 객체가 **하나의 공용 저장 공간을 공유**한다.

---

## ⚙️ 1️⃣ **저장 위치**

| 항목    | 설명                                               |
| ----- | ------------------------------------------------ |
| 저장 영역 | 전역 변수와 동일하게 **정적 메모리 영역(Static / Data Segment)** |
| 수명    | 프로그램 시작 ~ 종료까지 유지                                |
| 소속    | 클래스가 아닌, **프로그램 전체 단위**에 1개 존재                   |

```cpp
class Player {
public:
    static int playerCount;
    Player() { ++playerCount; }
};

int Player::playerCount = 0; // 정적 메모리 영역에 실제 정의
```

📌 `Player::playerCount`는 객체 생성과 관계없이 항상 한 군데만 존재함.

---

## ⚙️ 2️⃣ **초기화 방식**

| 초기화 시점                     | 설명                                         |
| -------------------------- | ------------------------------------------ |
| **클래스 내부 선언 시**            | 선언만 가능 (`int playerCount;` → 초기값 X)        |
| **클래스 외부 정의 시**            | 실제 메모리 할당 + 초기화 수행                         |
| **정적 지역 변수(static local)** | 최초 호출 시 한 번만 초기화 (thread-safe since C++11) |

```cpp
int Player::playerCount = 0; // ✅ 반드시 한 번 정의 필요
```

---

## ⚙️ 3️⃣ **객체 간 공유 구조**

```cpp
Player p1;
Player p2;
std::cout << p1.playerCount; // 동일한 변수 접근
std::cout << p2.playerCount; // 동일한 메모리 참조
```

➡️ `p1`, `p2` 모두 **같은 메모리 주소**를 바라본다.
➡️ 실제로 `Player::playerCount`와 동일.

---

## ⚙️ 4️⃣ **접근 방식**

| 접근 형태   | 예시                    | 설명                |
| ------- | --------------------- | ----------------- |
| 클래스명 기반 | `Player::playerCount` | 일반적이며 권장          |
| 객체 기반   | `p1.playerCount`      | 문법적으로 허용되지만 혼동 유발 |

---

## ⚙️ 5️⃣ **static 멤버 함수와의 관계**

* `static` 멤버 변수는 `static` 멤버 함수에서만 직접 접근 가능.
* 멤버 함수처럼 `this` 포인터가 없기 때문.

```cpp
class Player {
public:
    static int playerCount;
    static void ShowCount() { std::cout << playerCount; }
};
```

---

## ⚙️ 6️⃣ **템플릿 클래스의 static 멤버 주의점**

템플릿은 타입마다 static 멤버가 따로 존재한다.

```cpp
template<typename T>
class Pool {
public:
    static int allocCount;
};

template<typename T>
int Pool<T>::allocCount = 0;

Pool<int>::allocCount++;   // int 타입 전용 static
Pool<double>::allocCount++; // double 타입 전용 static
```

➡️ `Pool<int>`와 `Pool<double>`은 **별도 static 변수**를 가짐.

---

## ⚙️ 7️⃣ **초기화 순서 주의점**

* 전역/static 객체의 초기화 순서는 **컴파일 단위 간 보장되지 않음 (static initialization order fiasco)**
* 해결법: 함수 내부의 `static local` 변수로 lazy initialization

```cpp
static Player& Instance() {
    static Player instance; // 최초 접근 시 초기화
    return instance;
}
```

---

## 🎯 **면접용 정리**

> static 멤버 변수는 **객체가 아닌 클래스 단위로 하나만 존재**하며,
> 실제로는 **전역(static) 메모리 영역에 저장**됩니다.
>
> 외부에서 한 번 정의해야 메모리가 할당되며,
> 모든 객체는 이를 **공유**합니다.

---

### 🔥 꼬리질문 예상

1. **Q. static 멤버 변수는 어디에 저장되나요?**
   → 전역(static) 메모리 영역에 저장됩니다.

2. **Q. 초기화는 어디서 하나요?**
   → 클래스 외부에서 한 번 정의해야 실제 메모리가 할당됩니다.

3. **Q. 객체마다 다르게 존재하나요?**
   → 아닙니다. 클래스 전체에서 하나만 존재합니다.

4. **Q. 전역 변수와 다른 점은?**
   → 접근 범위가 클래스 내부로 제한되고, 이름 충돌 방지 효과가 있습니다.

</details>

## 26. friend 키워드를 쓰면 캡슐화가 깨진다고들 하는데, 그럼에도 불구하고 어떤 경우에는 꼭 써야 하나요?

<details close>
<summary><strong>💡 자세한 정답</strong></summary>

<div style="background-color:#f6f8fa; padding:15px; border-radius:10px;">

## 💡 **friend의 개념**

> `friend`는 **다른 클래스나 함수가 해당 클래스의 private/protected 멤버에 직접 접근할 수 있게 허용**하는 키워드다.
>
> 즉, **캡슐화를 의도적으로 일부 해제**하는 문법이다.

```cpp
class Player {
    int hp = 100;
    friend void Heal(Player& p);
};

void Heal(Player& p) { p.hp += 50; } // private 접근 가능
```

---

## ⚙️ 1️⃣ **friend의 종류**

| 형태               | 예시                                    | 설명                   |
| ---------------- | ------------------------------------- | -------------------- |
| **friend 함수**    | `friend void Heal(Player& p);`        | 전역 함수에 접근 권한 부여      |
| **friend 클래스**   | `friend class GameManager;`           | 특정 클래스 전체에 접근 권한 부여  |
| **friend 멤버 함수** | `friend void Logger::Print(Player&);` | 다른 클래스의 특정 멤버 함수만 허용 |

---

## ⚙️ 2️⃣ **캡슐화가 깨지는 이유**

* 원래 private 멤버는 클래스 외부에서 접근 불가해야 함.
* friend를 선언하면 “특정 외부 코드가 내부 구조를 직접 조작 가능” → **정보 은닉 위반**
* 유지보수 시 내부 구현이 바뀌면 friend 함수도 같이 수정해야 하는 문제 발생.

---

## ⚙️ 3️⃣ **그럼에도 꼭 필요한 경우**

| 상황                               | 설명                                            |
| -------------------------------- | --------------------------------------------- |
| **1️⃣ 연산자 오버로딩 (특히 이항 연산자)**     | 피연산자 양쪽을 다 접근해야 하므로 외부 friend 함수로 구현 필요       |
| **2️⃣ 서로 강하게 결합된 클래스 간 협력**      | 예: `LinkedList`와 `Node` — 서로 내부 포인터 구조를 알아야 함 |
| **3️⃣ 디버깅/로깅 유틸리티**              | `Logger`나 `Inspector` 클래스가 내부 상태를 직접 출력해야 할 때 |
| **4️⃣ 특정 관리 클래스가 하위 객체 내부 제어 시** | 예: `GameManager`가 `Player`의 체력/위치 직접 조작해야 할 때 |

---

### 📘 예시 — 이항 연산자 오버로딩

```cpp
class Vec2 {
    float x, y;
public:
    Vec2(float x, float y) : x(x), y(y) {}
    friend Vec2 operator+(const Vec2& a, const Vec2& b); // friend 필요
};

Vec2 operator+(const Vec2& a, const Vec2& b) {
    return Vec2(a.x + b.x, a.y + b.y);
}
```

➡️ `a`, `b` 모두 private 멤버 접근해야 하므로 friend 필요.

---

### 📘 예시 — 클래스 간 협력 관계

```cpp
class Node {
    int data;
    Node* next;
    friend class LinkedList; // LinkedList가 Node 내부 접근 허용
};

class LinkedList {
    Node* head;
public:
    void Add(int value) {
        Node* n = new Node();
        n->data = value; // private 접근 가능
        n->next = head;
        head = n;
    }
};
```

➡️ Node를 외부에서 수정 불가하게 하면서, LinkedList만 조작 가능하도록 설계.

---

## ⚙️ 4️⃣ **friend 선언의 특징**

| 항목            | 설명                              |
| ------------- | ------------------------------- |
| **접근 제어 무시**  | private/protected 멤버 접근 가능      |
| **상속되지 않음**   | 파생 클래스에는 friend 관계가 이어지지 않음     |
| **양방향 아님**    | A가 B의 friend라도, B는 A의 friend 아님 |
| **명시적 선언 필요** | 한쪽 클래스 안에서 반드시 선언해야 함           |

---

## ⚙️ 5️⃣ **남용 시 문제점**

| 문제             | 설명                              |
| -------------- | ------------------------------- |
| 🔴 **결합도 증가**  | friend가 많아질수록 내부 구조 노출          |
| ⚠️ **재사용성 저하** | 내부 변경 시 friend 코드 전부 수정 필요      |
| ⚠️ **테스트 어려움** | 외부 코드가 객체 내부를 직접 바꾸므로 상태 추적 어려움 |

---

## 🎯 **면접용 정리**

> `friend`는 **캡슐화를 일시적으로 해제**하는 문법으로,
> 보통 **연산자 오버로딩이나 밀접한 협력 클래스 설계** 시 사용됩니다.
>
> 하지만 **남용 시 결합도가 급격히 증가**하므로,
> 꼭 필요한 경우(두 클래스가 논리적으로 하나의 단위일 때)만 사용하는 것이 좋습니다.

---

### 🔥 꼬리질문 예상

1. **Q. friend를 써야만 가능한 상황이 있나요?**
   → 외부 함수에서 private 멤버 양쪽 접근이 필요한 이항 연산자 오버로딩.

2. **Q. friend 관계가 상속되나요?**
   → 아니요, 파생 클래스에는 자동으로 전달되지 않습니다.

3. **Q. friend 남용 시 어떤 문제가 있나요?**
   → 결합도 상승, 내부 구현 노출, 캡슐화 위반.

4. **Q. friend 대신 사용할 수 있는 대안은?**
   → public getter/setter, 내부 helper 함수, interface 제공 방식.

</details>


## 27. auto가 C++11 이후로 많이 쓰이는데, 이게 단순히 “귀찮아서” 쓰는 게 아니라 템플릿 타입 추론 규칙을 공유한다는 말이 있습니다. 무슨 뜻인가요?

<details close>
<summary><strong>💡 자세한 정답</strong></summary>

<div style="background-color:#f6f8fa; padding:15px; border-radius:10px;">

## 💡 **auto의 본질**

> `auto`는 단순히 “타입을 자동으로 써주는 문법”이 아니라,
> **템플릿 타입 추론(template type deduction)** 과 **같은 규칙**으로 타입을 결정한다.
>
> 즉, `auto`는 **템플릿의 `T`처럼 추론된다.**

---

## ⚙️ 1️⃣ **기본 개념**

```cpp
auto x = 10;          // int
auto y = 3.14;        // double
auto s = "hello";     // const char*
```

* 컴파일러가 우변을 보고 타입을 추론.
* 하지만 **단순 타입 치환이 아니라**,
  “템플릿 타입 추론 규칙(deduction rule)”을 따른다.

---

## ⚙️ 2️⃣ **템플릿과의 동일 규칙**

다음 두 코드는 **동일하게 동작**한다.

```cpp
template<typename T>
void func(T param);

int main() {
    int a = 5;
    const int b = a;
    func(b);     // T는 int로 추론됨
    auto c = b;  // c도 int (const 무시됨)
}
```

📌 `auto`는 내부적으로 “템플릿 T”처럼 동작한다.
`T param` → `auto var = ...` 와 완전히 동일한 추론 규칙.

---

## ⚙️ 3️⃣ **추론 규칙 요약**

| 선언 형태                   | 추론 결과                                         | 설명       |
| ----------------------- | --------------------------------------------- | -------- |
| `auto x = expr;`        | `expr` 타입에서 **top-level const, reference 제거** | 기본 변수 선언 |
| `auto& x = expr;`       | `expr` 타입 그대로 유지                              | 참조 선언    |
| `const auto& x = expr;` | 어떤 타입이든 참조 가능 (복사 방지)                         | 상수 참조    |
| `auto&& x = expr;`      | **universal reference** (lvalue/rvalue 모두 가능) | 완벽 전달    |

---

### 📘 예시 1 — const 제거

```cpp
const int n = 100;
auto a = n;     // int (const 제거됨)
auto& b = n;    // const int& (참조 유지)
```

> 변수 선언에서는 const가 사라지지만,
> 참조 선언에서는 const 특성이 유지된다.

---

### 📘 예시 2 — 포인터 타입 추론

```cpp
int x = 10;
const int* p = &x;

auto q = p;     // const int* (const 유지)
auto* r = p;    // const int* (포인터 직접 명시 시 동일)
```

---

### 📘 예시 3 — universal reference 추론 (C++11 이후)

```cpp
auto&& val = GetValue();
```

* rvalue → `val`은 rvalue reference
* lvalue → `val`은 lvalue reference
  ➡️ **템플릿의 `T&&`와 동일한 동작**

---

## ⚙️ 4️⃣ **템플릿 규칙을 공유한다는 의미**

| 비교                     | `auto`       | `template<typename T>` |
| ---------------------- | ------------ | ---------------------- |
| 타입 결정 기준               | 우변 표현식(expr) | 함수 인자(arg)             |
| const 제거 여부            | 동일           | 동일                     |
| 참조 유지 여부               | 동일           | 동일                     |
| universal reference 지원 | O (`auto&&`) | O (`T&&`)              |

➡️ 따라서 `auto`는 **템플릿 함수 인자 타입 추론과 완전히 동일한 로직**을 따른다.

---

## ⚙️ 5️⃣ **차이점 (단, decltype(auto)는 예외)**

* `auto`는 **템플릿 추론 규칙** 사용
* `decltype(auto)`는 **decltype 규칙** 사용 (식의 정확한 타입 유지)

```cpp
int x = 10;
int& ref = x;

auto a = ref;          // int
decltype(auto) b = ref; // int&
```

---

## ⚙️ 6️⃣ **실무에서의 활용**

| 상황                  | 예시                                               | 효과               |
| ------------------- | ------------------------------------------------ | ---------------- |
| **STL 반복자 타입 추론**   | `for (auto it = v.begin(); it != v.end(); ++it)` | 긴 iterator 타입 생략 |
| **템플릿 내 반환 타입 자동화** | `auto func() -> decltype(expr)`                  | 반환 타입 추론         |
| **범위 기반 for문**      | `for (auto& e : container)`                      | 요소 타입 자동 추론      |

---

## 🎯 **면접용 정리**

> `auto`는 단순한 “타입 생략 문법”이 아니라
> **템플릿 타입 추론 규칙을 그대로 적용하는 문법**입니다.
>
> 즉, `auto`는 `T`처럼 동작하며,
> const 제거·참조 유지·universal reference 추론이 동일하게 작동합니다.

---

### 🔥 꼬리질문 예상

1. **Q. `auto`와 `decltype(auto)`의 차이는?**
   → `auto`는 템플릿 추론 규칙, `decltype(auto)`는 표현식 그대로의 타입 유지.

2. **Q. 왜 const가 사라지나요?**
   → 템플릿 인자 추론 시 top-level const는 제거되기 때문.

3. **Q. `auto&&`가 특별한 이유는?**
   → universal reference로, lvalue/rvalue 모두 완벽하게 전달 가능.

4. **Q. 템플릿과 완전히 동일하다는 예시는?**
   → `auto x = expr;` ⇔ `template<typename T> void f(T param); f(expr);`

</details>

## 28. move semantics(이동 의미론)은 어떤 상황에서 유용하며, copy semantics와 비교했을 때 어떤 이점을 가지나요?

<details close>
<summary><strong>💡 자세한 정답</strong></summary>

<div style="background-color:#f6f8fa; padding:15px; border-radius:10px;">

## 💡 **이동 의미론(move semantics)이란?**

> 기존 객체의 데이터를 **복사하지 않고, 자원의 소유권만 이동(move)** 하는 개념이다.
> 즉, **복사 비용 없이 자원을 “넘겨받는” 최적화 기법**이다.
>
> C++11에서 도입된 **rvalue reference(`T&&`)** 기반 기능이다.

---

## ⚙️ 1️⃣ **기본 개념**

```cpp
class Buffer {
    int* data;
    size_t size;
public:
    Buffer(size_t s) : size(s), data(new int[s]) {}
    ~Buffer() { delete[] data; }

    // 복사 생성자
    Buffer(const Buffer& other)
        : size(other.size), data(new int[other.size]) {
        std::copy(other.data, other.data + size, data);
    }

    // 이동 생성자
    Buffer(Buffer&& other) noexcept
        : size(other.size), data(other.data) {
        other.data = nullptr;
        other.size = 0;
    }
};
```

📌 복사는 **새 메모리를 할당하고 데이터 복제**,
이동은 **포인터만 넘기고 원본은 null 처리**.

---

## ⚙️ 2️⃣ **copy vs move 차이 요약**

| 구분     | Copy Semantics    | Move Semantics             |
| ------ | ----------------- | -------------------------- |
| 동작     | 자원을 새로 복제         | 자원 소유권 이전                  |
| 비용     | 메모리 할당 + 데이터 복사   | 단순 포인터 이동                  |
| 대상     | lvalue (이름 있는 객체) | rvalue (임시 객체)             |
| 키워드    | `const T&`        | `T&&`                      |
| 소멸자 영향 | 원본과 사본 모두 해제      | 원본은 자원 해제 안 함 (nullptr 처리) |

---

## ⚙️ 3️⃣ **언제 유용한가**

| 상황                     | 설명                                                  |
| ---------------------- | --------------------------------------------------- |
| **1️⃣ 대용량 데이터 이동 시**   | 대규모 배열, 문자열, 버퍼 등 복사 부담이 큰 객체                       |
| **2️⃣ 임시 객체 반환 시**     | 함수 리턴 시 임시 객체를 효율적으로 반환 (`return std::move(obj)`)   |
| **3️⃣ STL 컨테이너 재배치 시** | `std::vector::push_back()`에서 기존 요소 재배치 시 불필요한 복사 방지 |
| **4️⃣ 리소스 래퍼 클래스**     | 파일, 소켓, 메모리 핸들 등 소유권 개념이 중요한 클래스에 적합                |

---

### 📘 예시 — 대용량 문자열 복사 방지

```cpp
std::string MakeString() {
    std::string temp = "Hello World";
    return temp; // RVO or move semantics
}

std::string s = MakeString(); // 복사 없이 이동 발생
```

➡️ 임시 객체(`temp`)는 rvalue이므로 move constructor 호출됨.
➡️ 불필요한 복사 제거 → 성능 향상.

---

## ⚙️ 4️⃣ **rvalue reference의 역할**

```cpp
void SetBuffer(Buffer&& buf) { // rvalue만 허용
    this->buffer = std::move(buf);
}
```

* `Buffer&&`는 **임시 객체**나 **소유권을 넘길 의도**가 있는 객체만 받음
* `std::move()`는 단순히 **lvalue를 rvalue로 캐스팅**하는 함수
  (실제 복사는 없음)

---

## ⚙️ 5️⃣ **STL에서의 활용**

| 컨테이너                             | 이동 의미론 사용 예시                     |
| -------------------------------- | -------------------------------- |
| `std::vector`                    | `emplace_back()`, 재할당 시 기존 요소 이동 |
| `std::unique_ptr`                | 복사 불가, 이동만 허용                    |
| `std::string`                    | 내부 버퍼 이동으로 복사 최소화                |
| `std::map`, `std::unordered_map` | insert 시 임시 객체 이동 생성             |

---

## ⚙️ 6️⃣ **성능 비교**

| 항목     | Copy       | Move                 |
| ------ | ---------- | -------------------- |
| 메모리 할당 | 새로 할당      | 포인터만 이동              |
| 시간 복잡도 | O(n)       | O(1)                 |
| 리소스 중복 | 있음         | 없음                   |
| 안전성    | 안전하지만 비효율적 | 효율적, 단 이동 후 원본 사용 불가 |

---

## ⚙️ 7️⃣ **주의할 점**

| 주의사항         | 설명                                       |
| ------------ | ---------------------------------------- |
| 이동 후 원본 상태   | 정의되지만 비어 있음(null 등으로 초기화)                |
| noexcept 필요성 | STL 컨테이너는 move 생성자가 `noexcept`일 때만 이동 사용 |
| 명시적 이동 필요    | 명시적으로 `std::move()` 호출해야 rvalue로 인식됨     |

---

## 🎯 **면접용 정리**

> 이동 의미론은 **복사 대신 자원 소유권을 이전**해
> 불필요한 메모리 복사와 할당을 줄이는 기술입니다.
>
> 복사는 새 메모리 생성 + 데이터 복제지만,
> 이동은 단순히 **포인터를 넘겨받아 원본 자원을 재활용**하므로
> 대용량 객체나 임시 객체 처리 시 성능 이점이 큽니다.

---

### 🔥 꼬리질문 예상

1. **Q. std::move는 실제로 데이터를 이동하나요?**
   → 아니요, 단순히 lvalue를 rvalue로 캐스팅할 뿐입니다.

2. **Q. 이동 후 원본 객체는 어떤 상태가 되나요?**
   → 유효하지만 자원을 잃은 “빈 상태(null)”로 남습니다.

3. **Q. move가 copy보다 빠른 이유는?**
   → 메모리 복사 없이 포인터만 전달하기 때문입니다.

4. **Q. STL 컨테이너에서 move가 작동하지 않는 경우는?**
   → move 생성자가 `noexcept`가 아니면, 안전을 위해 copy가 대신 호출됩니다.

</details>

## 29. unique_ptr과 shared_ptr은 어떤 차이를 가지며, 내부적으로는 어떻게 소유권을 관리하나요?

<details close>
<summary><strong>💡 자세한 정답</strong></summary>

<div style="background-color:#f6f8fa; padding:15px; border-radius:10px;">

## 💡 **스마트 포인터의 개념**

> C++의 스마트 포인터는 **RAII(Resource Acquisition Is Initialization)** 개념을 기반으로,
> 동적 할당된 객체의 **소유권과 생명주기를 자동으로 관리**하기 위한 클래스 템플릿이다.
>
> 대표적으로 `unique_ptr`, `shared_ptr`, `weak_ptr`이 있다.

---

## ⚙️ 1️⃣ **unique_ptr**

| 특징                 | 설명                              |
| ------------------ | ------------------------------- |
| **소유권 단독(unique)** | 하나의 `unique_ptr`만이 객체를 소유       |
| **복사 불가**          | 복사 생성자/대입 연산자가 삭제됨 (`= delete`) |
| **이동만 가능**         | `std::move()`로 소유권 이전 가능        |
| **소멸 시점**          | 포인터가 범위를 벗어나면 자동으로 `delete` 호출  |

```cpp
std::unique_ptr<Player> p1 = std::make_unique<Player>();
std::unique_ptr<Player> p2 = std::move(p1); // 소유권 이동
```

📌 `p1`은 이동 후 nullptr, `p2`만 객체를 소유.

---

## ⚙️ 2️⃣ **shared_ptr**

| 특징                              | 설명                               |
| ------------------------------- | -------------------------------- |
| **참조 카운트 기반(shared ownership)** | 여러 포인터가 같은 객체를 공유                |
| **복사 가능**                       | 카운트를 1 증가시킴                      |
| **모든 포인터가 해제될 때 소멸**            | 참조 카운트가 0이 되면 객체 delete          |
| **내부 구현**                       | 별도의 **control block**에서 참조 수를 관리 |

```cpp
std::shared_ptr<Player> p1 = std::make_shared<Player>();
std::shared_ptr<Player> p2 = p1; // 참조 카운트 +1
```

📌 `p1`, `p2` 모두 같은 객체를 가리키며, 둘 다 사라질 때 해제됨.

---

## ⚙️ 3️⃣ **내부 구조 비교**

| 구분                | `unique_ptr` | `shared_ptr`                          |
| ----------------- | ------------ | ------------------------------------- |
| **소유권 수**         | 1개만 존재       | 여러 개 공유 가능                            |
| **참조 카운트**        | 없음           | 있음 (`use_count()`)                    |
| **Control Block** | 없음           | 있음 (ref count, deleter, allocator 포함) |
| **복사 가능 여부**      | ❌ (이동만)      | ✅ (참조 카운트 증가)                         |
| **메모리 오버헤드**      | 작음           | 큼 (Control Block 관리 필요)               |
| **스레드 안전성**       | X (직접 관리)    | O (ref count는 atomic)                 |

---

### 📘 Control Block 구조 (shared_ptr 내부)

```
[Control Block]
 ├─ strong_count (atomic)
 ├─ weak_count (atomic)
 ├─ deleter function ptr
 ├─ allocator info
```

* 객체는 **Control Block과 별도로 할당**
* 모든 `shared_ptr`은 같은 Control Block을 참조
* `use_count() == 0` → 객체 delete
* `weak_count() == 0` → Control Block 해제

---

## ⚙️ 4️⃣ **make_unique / make_shared**

| 함수                 | 설명                                                 |
| ------------------ | -------------------------------------------------- |
| `make_unique<T>()` | C++14 이후 도입. `unique_ptr` 생성 시 예외 안전 보장            |
| `make_shared<T>()` | Control Block과 객체를 **하나의 메모리 블록**에 같이 할당 (캐시 효율 ↑) |

📌 `shared_ptr`을 직접 생성 (`new` 사용)하면
Control Block과 객체가 따로 할당되어 **예외 안전성 저하**.

---

### 📘 예시 — shared_ptr 내부 카운트 변화

```cpp
auto p1 = std::make_shared<int>(5);
{
    auto p2 = p1; // use_count = 2
    auto p3 = p1; // use_count = 3
}
std::cout << p1.use_count(); // 1 (p2, p3 소멸)
```

➡️ 참조 카운트는 `atomic`으로 관리되어 **멀티스레드 환경에서도 안전**.

---

## ⚙️ 5️⃣ **소유권 이동 시 차이**

| 구분           | 설명                                      |
| ------------ | --------------------------------------- |
| `unique_ptr` | 이동 시 **원본이 nullptr**, 새 포인터만 소유         |
| `shared_ptr` | 이동 시 **Control Block 공유**, 참조 카운트 변경 없음 |

---

## ⚙️ 6️⃣ **주의할 점**

| 항목                         | 설명                                             |
| -------------------------- | ---------------------------------------------- |
| ❌ **순환 참조 문제**             | `shared_ptr`이 서로를 가리키면 해제 안 됨 → `weak_ptr`로 해결 |
| ⚠️ **비용**                  | 참조 카운트 증가/감소는 atomic 연산 → 약간의 오버헤드             |
| ⚠️ **unique_ptr의 move 주의** | 이동 후 원본 포인터 접근 금지 (`nullptr`)                  |

---

### 📘 순환 참조 문제 예시

```cpp
struct A {
    std::shared_ptr<B> bptr;
};
struct B {
    std::shared_ptr<A> aptr;
};

auto a = std::make_shared<A>();
auto b = std::make_shared<B>();
a->bptr = b;
b->aptr = a; // ❌ 서로 참조, use_count 절대 0 안 됨
```

➡️ 해결: `B` 안에서 `weak_ptr<A>` 사용.

---

## 🎯 **면접용 정리**

> `unique_ptr`은 **단독 소유**, `shared_ptr`은 **참조 카운트 기반 공유 소유**입니다.
>
> `unique_ptr`은 이동만 가능해 비용이 거의 없고,
> `shared_ptr`은 **Control Block**에서 참조 수를 원자적으로 관리해
> 여러 포인터가 안전하게 공유할 수 있습니다.
>
> 단, `shared_ptr`은 순환 참조 문제를 주의해야 합니다.

---

### 🔥 꼬리질문 예상

1. **Q. shared_ptr의 참조 카운트는 어떻게 관리되나요?**
   → Control Block 내부의 atomic 카운터로 관리됩니다.

2. **Q. unique_ptr은 왜 복사할 수 없나요?**
   → 단독 소유 보장을 위해 복사 생성자가 `delete` 처리되어 있음.

3. **Q. shared_ptr의 순환 참조는 어떻게 해결하나요?**
   → `weak_ptr`를 사용해 소유권 없는 참조를 둡니다.

4. **Q. make_shared를 쓰면 좋은 이유는?**
   → 객체와 Control Block을 한 번에 할당해 **성능 향상**과 **예외 안전성**을 확보합니다.

</details>


## 30. 람다(lambda) 표현식에서 캡처(capture) 방식 [=], [&] 의 차이와 각각의 장단점을 설명해보세요.

<details close>
<summary><strong>💡 자세한 정답</strong></summary>

<div style="background-color:#f6f8fa; padding:15px; border-radius:10px;">

## 💡 **람다 캡처란?**

> 람다(lambda)는 **익명 함수 객체**이며,
> 외부 스코프의 변수를 내부에서 사용할 때 **캡처(capture)** 방식을 지정해야 한다.
>
> 즉, “외부 변수를 어떻게 가져올지”를 정의하는 부분이 **capture list**(`[]`)이다.

---

## ⚙️ 1️⃣ **기본 문법**

```cpp
[캡처 방식](매개변수) -> 반환형 { 함수 본문 };
```

예시:

```cpp
int a = 10, b = 20;

auto f1 = [=]() { return a + b; }; // 값 캡처
auto f2 = [&]() { b += 5; return a + b; }; // 참조 캡처
```

---

## ⚙️ 2️⃣ **[=] (값 캡처, by value)**

> 외부 변수를 **복사해서 람다 내부에 저장**.
> 원본이 바뀌어도 람다 내부에는 영향을 주지 않는다.

```cpp
int x = 10;
auto valCap = [=]() { return x + 5; };
x = 20;
std::cout << valCap(); // 15 (복사된 10 사용)
```

| 특징     | 설명                                    |
| ------ | ------------------------------------- |
| 복사본 생성 | 외부 변수의 현재 값을 복사해 람다 내부에 저장            |
| 수정 불가  | 기본적으로 const 취급 (`mutable` 키워드로 변경 가능) |
| 수명 보장  | 원본이 사라져도 복사본이 남아 안전                   |
| 장점     | 예측 가능, 스레드 안전                         |
| 단점     | 메모리 복사 비용 발생, 변경 반영 불가                |

---

### 📘 mutable 키워드로 수정 가능하게 만들기

```cpp
int cnt = 0;
auto f = [=]() mutable { cnt++; return cnt; };
f(); f(); 
std::cout << cnt; // 여전히 0 (복사본만 증가)
```

➡️ `mutable`은 람다 내부 복사본만 수정 가능, 외부 변수는 그대로.

---

## ⚙️ 3️⃣ **[&] (참조 캡처, by reference)**

> 외부 변수를 **참조로 저장**하여 원본에 직접 접근한다.
> 따라서 변경 시 원본 변수에도 반영된다.

```cpp
int y = 10;
auto refCap = [&]() { y += 5; };
refCap();
std::cout << y; // 15 (원본 수정됨)
```

| 특징    | 설명                             |
| ----- | ------------------------------ |
| 참조 저장 | 외부 변수 주소를 저장, 복사 없음            |
| 변경 가능 | 원본 직접 수정 가능                    |
| 수명 의존 | 원본이 사라지면 dangling reference 위험 |
| 장점    | 복사 비용 없음, 외부 상태 반영 가능          |
| 단점    | 수명 문제 발생 가능, 스레드 안전성 낮음        |

---

## ⚙️ 4️⃣ **혼합 캡처**

| 형태        | 설명                   |
| --------- | -------------------- |
| `[=, &x]` | 기본은 값 캡처, `x`만 참조    |
| `[&, x]`  | 기본은 참조 캡처, `x`만 값 복사 |

```cpp
int a = 10, b = 20;
auto mix = [=, &b]() { b += 5; return a + b; };
mix(); // b는 수정됨, a는 복사본 유지
```

---

## ⚙️ 5️⃣ **캡처와 수명(Lifetime) 주의점**

```cpp
auto CreateLambda() {
    int local = 42;
    return [&]() { return local; }; // ❌ 위험: local은 이미 소멸
}
```

➡️ 참조 캡처는 지역 변수가 사라지면 **dangling reference 발생**
➡️ 값 캡처는 복사본이 남으므로 안전.

---

## ⚙️ 6️⃣ **성능 및 사용 가이드**

| 항목      | [=] 값 캡처    | [&] 참조 캡처        |
| ------- | ----------- | ---------------- |
| 메모리 비용  | 복사 오버헤드     | 낮음               |
| 안전성     | 수명 안전       | 위험 (dangling 가능) |
| 변경 가능성  | 기본 const    | 원본 변경 가능         |
| 스레드 안정성 | 높음 (복사본 사용) | 낮음 (공유 데이터 접근)   |
| 사용 예시   | 병렬/비동기 처리   | 로컬 변수 즉시 수정      |

---

### 📘 실무 예시 — IOCP 서버에서의 사용

```cpp
auto job = [&, id]() { 
    GRoomManager.DoWork(id, this->_player); 
};
```

* `id`는 값 캡처로 안전히 보관
* `this`는 참조 캡처로 실제 세션 객체 접근
  ➡️ 혼합 캡처 패턴은 **성능 + 안정성 균형**을 맞출 때 자주 사용된다.

---

## 🎯 **면접용 정리**

> `[=]`은 외부 변수를 **값으로 복사**, `[&]`는 **참조로 접근**합니다.
>
> 값 캡처는 안전하지만 변경 불가,
> 참조 캡처는 효율적이지만 수명 문제에 주의해야 합니다.
>
> 따라서 “읽기 전용”이면 `[=]`,
> “수정이 필요하거나 외부 상태를 반영해야 할 때”는 `[&]`를 사용합니다.

---

### 🔥 꼬리질문 예상

1. **Q. mutable 키워드의 역할은?**
   → 값 캡처된 변수를 람다 내부에서 수정할 수 있게 한다.

2. **Q. 참조 캡처가 위험한 이유는?**
   → 원본이 소멸되면 람다가 dangling reference를 갖게 된다.

3. **Q. 람다에서 this 캡처 방식은?**
   → `[this]`는 객체 포인터 복사, `[=]`은 this 멤버 자동 복사(C++20부터 `[=*this]` 지원).

4. **Q. 스레드에서 캡처 방식 선택 기준은?**
   → 스레드 내부에서는 [=] 값 캡처가 안전하다. 참조 캡처는 경쟁 조건(race condition) 유발 가능.

</details>

## 31. std::function과 단순 함수 포인터(void(*)())의 차이는 무엇인가요?

<details close>
<summary><strong>💡 자세한 정답</strong></summary>

<div style="background-color:#f6f8fa; padding:15px; border-radius:10px;">

## 💡 **핵심 개념**

> `std::function`은 **C++11에서 도입된 범용 함수 래퍼(General-purpose function wrapper)** 로,
> 단순 함수 포인터보다 **훨씬 유연하게 다양한 호출 대상(callable)** 을 저장하고 실행할 수 있는 템플릿 클래스다.

---

## ⚙️ 1️⃣ **기본 비교**

| 구분            | `std::function`                         | 함수 포인터 (`void(*)()`)          |
| ------------- | --------------------------------------- | ----------------------------- |
| **저장 가능 대상**  | 일반 함수, 멤버 함수, 람다, functor 등 모든 callable | 오직 **전역 함수** 또는 **static 함수** |
| **타입 안정성**    | 템플릿으로 인자/반환 타입 검사                       | 시그니처 일치만 확인                   |
| **상태(state)** | 람다의 캡처 값, functor 상태 등 보관 가능            | 상태 저장 불가능                     |
| **할당 가능성**    | 다양한 callable을 대입 가능                     | 같은 시그니처 함수만 대입 가능             |
| **내부 구현**     | type erasure + 가상 호출 테이블                | 단순한 함수 주소 저장                  |
| **오버헤드**      | 약간 존재 (heap 할당 + 가상 호출)                 | 거의 없음 (정적 호출)                 |

---

## ⚙️ 2️⃣ **기본 사용 예시**

### ✅ 함수 포인터

```cpp
void Hello() { std::cout << "Hello\n"; }

void (*funcPtr)() = &Hello;
funcPtr(); // 호출
```

* 함수 주소만 저장
* 멤버 함수나 캡처된 람다는 저장 불가

---

### ✅ std::function

```cpp
#include <functional>

std::function<void()> func;

func = [] { std::cout << "Lambda!\n"; }; // 캡처 없는 람다
func(); // 호출

func = Hello; // 일반 함수 대입
func(); // 호출
```

* 람다, 일반 함수, functor 모두 저장 가능
* 내부적으로 **type-erased callable**을 `operator()`로 실행

---

## ⚙️ 3️⃣ **캡처된 람다 예시**

```cpp
int x = 10;
std::function<void()> f = [x]() { std::cout << x << '\n'; };
f(); // 출력: 10
```

📌 `std::function`은 내부적으로 **람다의 복사본과 상태값(x)** 을 함께 저장한다.
➡️ 함수 포인터로는 불가능한 “상태 있는 함수” 표현 가능.

---

## ⚙️ 4️⃣ **functor(함수 객체)도 가능**

```cpp
struct Printer {
    void operator()() const { std::cout << "Printer called\n"; }
};

std::function<void()> f = Printer();
f(); // "Printer called"
```

> 즉, **operator()를 가진 모든 객체(callable object)** 를 수용한다.

---

## ⚙️ 5️⃣ **내부 구현 원리**

`std::function`은 내부적으로 **type erasure (타입 소거)** 기법을 사용한다.

```
std::function<void()> f = ...;

f
 ├─ control block (type-erased)
 │   ├─ callable object (람다 / 함수 / functor)
 │   ├─ invoke() 함수 포인터 (virtual dispatch)
 │   └─ destructor 포인터
```

* 어떤 타입의 callable이 와도 공통 인터페이스(`invoke()`)로 실행 가능
* 다양한 타입의 호출 대상을 **하나의 함수형 인터페이스로 통합**

---

## ⚙️ 6️⃣ **성능 차이**

| 항목         | 함수 포인터     | std::function              |
| ---------- | ---------- | -------------------------- |
| **호출 속도**  | 빠름 (직접 호출) | 약간 느림 (간접 호출)              |
| **메모리 사용** | 작음         | 크다 (callable 객체 저장)        |
| **유연성**    | 낮음         | 매우 높음                      |
| **사용 목적**  | 단순한 함수 콜백  | 상태 있는 콜백, 람다 저장, 이벤트 핸들링 등 |

---

### 📘 예시 — IOCP 서버 콜백 구조

```cpp
std::function<void()> onAccept;
onAccept = [this]() { this->ProcessAccept(); };

void* completionKey = nullptr;
PostQueuedCompletionStatus(hIOCP, 0, (ULONG_PTR)completionKey, nullptr);
```

➡️ `std::function`을 이용해
→ **상태(this)** 와 **함수 로직**을 함께 저장할 수 있다.
➡️ 함수 포인터는 단순한 `void(*)()`만 가능하므로 불가능.

---

## ⚙️ 7️⃣ **메모리 오버헤드와 주의점**

| 항목          | 설명                                            |
| ----------- | --------------------------------------------- |
| 내부 힙 할당     | 큰 람다나 functor 저장 시 heap 사용 가능                 |
| 인라인 버퍼      | 소형 객체(SBO, Small Buffer Optimization)는 스택에 저장 |
| empty check | `if (f)` 로 함수 존재 여부 검사 가능                     |
| 멀티스레드 안전성   | 별도 동기화 필요 (copy 시 ref count 없음)               |

---

## 🎯 **면접용 정리**

> `std::function`은 **람다, 멤버 함수, functor 등 모든 호출 가능한 객체**를 저장할 수 있는 **범용 함수 래퍼**입니다.
>
> 단순 함수 포인터(`void(*)()`)는 **정적 함수 주소만 저장** 가능하지만,
> `std::function`은 **캡처된 상태나 클래스 멤버까지 포함한 callable 객체**를 관리할 수 있습니다.
>
> 내부적으로는 **type erasure + 가상 호출 구조**로 동작합니다.

---

### 🔥 꼬리질문 예상

1. **Q. std::function 내부적으로 어떤 방식으로 다양한 타입을 저장하나요?**
   → type erasure 기법을 사용해 callable 객체를 void* 형태로 저장하고, invoke 함수 포인터를 통해 호출합니다.

2. **Q. std::function의 성능이 함수 포인터보다 느린 이유는?**
   → 간접 호출(virtual dispatch)과 메모리 복사 오버헤드가 있기 때문입니다.

3. **Q. 캡처가 있는 람다는 왜 함수 포인터로 저장할 수 없나요?**
   → 캡처된 상태값이 추가 메모리에 저장되기 때문. 함수 포인터는 단순 코드 주소만 저장합니다.

4. **Q. 소형 객체 최적화(SBO)는 무엇인가요?**
   → 작은 람다나 functor는 heap 대신 내부 버퍼(stack)에 저장하여 성능을 개선하는 최적화입니다.

</details>


## 32. constexpr 함수와 const 함수는 어떤 차이를 가지나요?

<details close>
<summary><strong>💡 자세한 정답</strong></summary>

<div style="background-color:#f6f8fa; padding:15px; border-radius:10px;">

## 💡 **핵심 개념**

> `const`와 `constexpr`은 모두 “변하지 않는 값”과 관련이 있지만,
> **시점이 다르다.**
>
> * `const` → **런타임 상수 (run-time constant)**
> * `constexpr` → **컴파일타임 상수 (compile-time constant)**

---

## ⚙️ 1️⃣ **const 함수**

> 객체 상태를 변경하지 않겠다는 의미의 **멤버 함수 한정자**다.

```cpp
class Player {
    int hp = 100;
public:
    int GetHP() const { return hp; } // const 함수
    void SetHP(int v) { hp = v; }
};
```

📌 **특징**

| 항목       | 설명                                               |
| -------- | ------------------------------------------------ |
| 선언 위치    | 멤버 함수 선언 뒤 (`void Foo() const;`)                 |
| 의미       | 함수 내에서 멤버 변수 수정 금지                               |
| this 포인터 | `Player* const this` → `const Player* this` 로 변환 |
| 목적       | “이 함수는 객체 상태를 바꾸지 않는다”는 계약 보장                    |
| 위반 시 에러  | const 함수 내에서 멤버 수정 시 컴파일 에러 발생                   |

---

## ⚙️ 2️⃣ **constexpr 함수**

> **컴파일 시점**에 계산 가능한 함수를 의미한다.
> 함수 결과가 “컴파일 타임 상수”로 평가될 수 있어야 함.

```cpp
constexpr int Add(int a, int b) {
    return a + b;
}

int arr[Add(2, 3)]; // 컴파일 시 계산 → int arr[5];
```

📌 **특징**

| 항목       | 설명                               |
| -------- | -------------------------------- |
| 평가 시점    | **컴파일 타임** (조건 충족 시)             |
| 반환값      | 상수식(constexpr context)에서 사용 가능   |
| 제약 조건    | 함수 내부에 컴파일타임 계산 가능한 문장만 포함       |
| 런타임 호출   | 가능 (`constexpr` 함수는 일반 함수로도 사용됨) |
| C++14 이후 | 지역 변수, 루프 등 제한 완화됨               |

---

## ⚙️ 3️⃣ **비교 요약**

| 구분              | `const` 함수      | `constexpr` 함수  |
| --------------- | --------------- | --------------- |
| **적용 대상**       | 클래스 멤버 함수       | 모든 함수           |
| **평가 시점**       | 런타임             | 컴파일 타임          |
| **목적**          | 객체 상태 불변 보장     | 상수 표현식 계산       |
| **this 포인터**    | `const T* this` | 일반 함수 가능        |
| **컴파일러 검사 항목**  | 멤버 수정 여부        | 컴파일타임 평가 가능 여부  |
| **함수 호출 가능 시점** | 실행 시            | 컴파일 시 (또는 실행 시) |

---

## ⚙️ 4️⃣ **함께 사용 가능**

`constexpr` 멤버 함수는 암묵적으로 `const` 특성을 가진다.

```cpp
class Vec2 {
    int x, y;
public:
    constexpr Vec2(int _x, int _y) : x(_x), y(_y) {}
    constexpr int GetX() const { return x; } // const 포함
};
```

➡️ `constexpr` 멤버 함수는 자동으로 const 함수로 간주됨.
(객체 상태를 변경할 수 없음)

---

## ⚙️ 5️⃣ **예시 비교**

```cpp
class Test {
    int a = 10;
public:
    int GetA() const { return a; }           // const: 객체 수정 불가
    constexpr int Sum(int x, int y) const {  // constexpr: 컴파일 시 계산 가능
        return x + y;
    }
};

constexpr int result = Test().Sum(2, 3); // 컴파일 시 계산됨
```

---

## ⚙️ 6️⃣ **실무 활용**

| 구분          | 예시                                      | 활용 목적                       |
| ----------- | --------------------------------------- | --------------------------- |
| `const`     | `int GetHP() const`                     | 멤버 불변 보장 (클래스 인터페이스 신뢰성 확보) |
| `constexpr` | `constexpr int MaxHP() { return 100; }` | 상수 정의 및 컴파일 최적화             |
| 둘 다 사용      | `constexpr int GetX() const`            | 상수 객체의 컴파일타임 접근 지원          |

---

## ⚙️ 7️⃣ **주의점**

| 항목                      | 설명                                       |
| ----------------------- | ---------------------------------------- |
| **const는 런타임 개념**       | 실행 중 변경 불가이지만, 컴파일러가 계산하진 않음             |
| **constexpr은 컴파일타임 개념** | 반드시 컴파일 중 계산 가능해야 함                      |
| **constexpr 함수라도**      | 인자로 런타임 값이 들어오면 런타임에 실행됨                 |
| **constexpr 함수 제약**     | 반드시 하나의 return 식으로 평가 가능해야 함 (C++14 전까지) |

---

## 🎯 **면접용 정리**

> `const` 함수는 **“객체의 상태를 바꾸지 않겠다”**는 약속이고,
> `constexpr` 함수는 **“컴파일 타임에 계산할 수 있다”**는 의미입니다.
>
> 즉, `const`는 **런타임 제약**, `constexpr`은 **컴파일타임 제약**이며,
> `constexpr` 멤버 함수는 자동으로 `const` 특성을 포함합니다.

---

### 🔥 꼬리질문 예상

1. **Q. constexpr 함수는 무조건 컴파일 시 실행되나요?**
   → 아니요, 인자가 런타임 값이면 런타임에 실행됩니다.

2. **Q. const 함수와 constexpr 함수는 함께 쓸 수 있나요?**
   → 예, `constexpr` 멤버 함수는 자동으로 const 취급됩니다.

3. **Q. constexpr의 장점은?**
   → 런타임 비용 제거, 상수 배열 크기 정의 등 컴파일 최적화 가능.

4. **Q. const 함수가 필요한 이유는?**
   → 멤버 함수가 객체 상태를 변경하지 않음을 보장해, 안정적인 인터페이스를 제공하기 때문입니다.

</details>

## 33. move 생성자와 copy 생성자가 모두 정의되어 있을 때, 컴파일러는 어떤 기준으로 호출을 선택하나요?

<details close>
<summary><strong>💡 자세한 정답</strong></summary>

<div style="background-color:#f6f8fa; padding:15px; border-radius:10px;">

## 💡 **핵심 개념**

> 객체를 초기화할 때 **컴파일러는 전달된 값의 value category (lvalue / rvalue)** 에 따라
> **copy 생성자 또는 move 생성자 중 하나를 선택**한다.
>
> 즉, **“이 객체를 복사해야 하는가, 이동해도 되는가”**를 구분하는 기준은
> **값이 임시(rvalue)인지, 이름(lvalue)이 있는지**에 따라 달라진다.

---

## ⚙️ 1️⃣ **기본 규칙**

| 값의 형태                            | 호출되는 생성자     | 예시                                     |
| -------------------------------- | ------------ | -------------------------------------- |
| **lvalue** (이름 있는 객체)            | **copy 생성자** | `T a; T b = a;`                        |
| **rvalue** (임시 객체, std::move 사용) | **move 생성자** | `T b = std::move(a);` 또는 `return T();` |

---

### 📘 예시 코드

```cpp
class Data {
public:
    Data() {}
    Data(const Data&)  { std::cout << "Copy\n"; }
    Data(Data&&) noexcept { std::cout << "Move\n"; }
};

Data MakeData() {
    Data d;
    return d; // RVO or Move
}

int main() {
    Data a;
    Data b = a;           // Copy (lvalue)
    Data c = std::move(a); // Move (rvalue)
    Data d = MakeData();   // Move or RVO
}
```

출력 (최적화 없을 시):

```
Copy
Move
Move
```

---

## ⚙️ 2️⃣ **선택 우선순위**

| 우선순위               | 조건                                   | 설명           |
| ------------------ | ------------------------------------ | ------------ |
| ① **Move 생성자**     | 인자가 **rvalue** 이고 move 생성자가 존재할 때    | 가장 우선적으로 사용됨 |
| ② **Copy 생성자**     | 인자가 **lvalue** 이거나 move 생성자가 삭제되었을 때 | fallback 선택  |
| ③ **Deleted Move** | move 생성자가 `= delete`되면 **copy로 대체**  |              |
| ④ **Deleted Copy** | copy까지 삭제되면 **컴파일 에러** 발생            |              |

---

## ⚙️ 3️⃣ **std::move의 역할**

```cpp
T b = std::move(a);
```

* `std::move`는 **단순히 lvalue를 rvalue로 캐스팅**한다.
* 실제 이동은 **move 생성자**가 존재할 때 수행된다.

> 즉, `std::move()`는 “이 자원 버려도 돼”라는 **의사 전달 장치**일 뿐,
> 실제 이동은 move 생성자 구현 여부에 달려 있다.

---

## ⚙️ 4️⃣ **noexcept의 영향**

> STL 컨테이너는 move 생성자가 `noexcept`일 때만 이동을 선택한다.

```cpp
class A {
public:
    A(A&&) { ... } // noexcept 아님
    A(const A&) { ... }
};

std::vector<A> v;
v.push_back(A()); // Copy 발생 (예외 안전 보장 위해)
```

📌 즉, move 생성자가 있어도 `noexcept`가 아니면 **copy로 fallback**될 수 있다.

---

## ⚙️ 5️⃣ **컴파일러의 자동 생성 규칙**

| 생성자 정의 상태 | 컴파일러 동작               |
| --------- | --------------------- |
| 둘 다 미정의   | 컴파일러가 copy/move 모두 생성 |
| copy만 정의  | move는 자동 삭제           |
| move만 정의  | copy는 자동 삭제           |
| 둘 다 삭제    | 객체 복사/이동 불가           |

➡️ 직접 정의할 때는 둘 다 고려해야 한다.

---

## ⚙️ 6️⃣ **Move가 Copy보다 선택되지 않는 경우**

| 원인                       | 설명                            |
| ------------------------ | ----------------------------- |
| move 생성자가 삭제(`= delete`) | 강제로 copy로 대체                  |
| move 생성자가 noexcept 아님    | 컨테이너에서 copy fallback          |
| 인자가 const lvalue         | move 불가능 (`const` rvalue는 없음) |

예시 👇

```cpp
const Data d;
Data x = std::move(d); // ❌ const 객체는 이동 불가 → Copy 호출
```

---

## ⚙️ 7️⃣ **정리된 호출 우선순위 흐름**

```text
1️⃣ 인자가 rvalue → move 생성자 존재 → Move 호출
2️⃣ 인자가 rvalue → move 생성자 없음 → Copy 호출
3️⃣ 인자가 lvalue → Copy 호출
4️⃣ move가 noexcept 아님 (컨테이너 상황) → Copy 호출
```

---

## 🎯 **면접용 정리**

> move 생성자와 copy 생성자가 모두 있을 때,
> **컴파일러는 인자가 rvalue이면 move, lvalue이면 copy**를 호출합니다.
>
> 단, move 생성자가 `noexcept`가 아니거나 삭제된 경우에는
> **copy 생성자로 fallback**됩니다.
>
> `std::move()`는 단순 캐스팅이므로, **move 생성자가 없으면 copy가 호출**됩니다.

---

### 🔥 꼬리질문 예상

1. **Q. const 객체는 move 가능한가요?**
   → 불가능합니다. move는 비-const rvalue에만 적용됩니다.

2. **Q. STL 컨테이너에서 copy가 호출되는 이유는?**
   → move 생성자가 `noexcept`가 아닐 경우 예외 안전성 확보를 위해 copy 사용.

3. **Q. move와 copy 둘 다 정의하지 않으면 어떻게 되나요?**
   → 컴파일러가 암시적으로 생성하지만, 조건에 따라 삭제될 수 있습니다.

4. **Q. std::move()는 무조건 이동을 수행하나요?**
   → 아니요. move 생성자가 없으면 단순히 copy로 대체됩니다.

</details>


## 34. RAII(Resource Acquisition Is Initialization) 패턴이 예외 안전성을 보장하는 이유를 설명해보세요.

<details close>
<summary><strong>💡 자세한 정답</strong></summary>

<div style="background-color:#f6f8fa; padding:15px; border-radius:10px;">

## 💡 **RAII란?**

> **Resource Acquisition Is Initialization**
> (자원 획득은 초기화다)
>
> 객체의 **생성자에서 자원을 획득하고**,
> **소멸자에서 자원을 자동 해제**하는 패턴이다.
>
> 즉, **객체의 생명주기(lifetime)** 와 **자원의 소유권**을 일치시켜
> **예외가 발생하더라도 자원이 자동 정리되도록 보장**한다.

---

## ⚙️ 1️⃣ **핵심 원리**

| 시점  | RAII의 동작                      |
| --- | ----------------------------- |
| 생성자 | 자원 획득 (파일 열기, 메모리 할당, 락 획득 등) |
| 소멸자 | 자원 해제 (파일 닫기, 메모리 해제, 락 반환 등) |

📌 **C++은 블록을 벗어날 때 자동으로 소멸자 호출** → 자원 누수 없음.

---

### 📘 예시 — RAII 미사용 (문제 발생)

```cpp
void ReadFile() {
    FILE* f = fopen("data.txt", "r");
    if (!f) throw std::runtime_error("파일 열기 실패");

    // 예외 발생 시 fclose()가 호출되지 않음 → 자원 누수
    throw std::runtime_error("읽기 실패");
    fclose(f);
}
```

---

### ✅ RAII 적용 예시

```cpp
#include <fstream>
void ReadFile() {
    std::ifstream file("data.txt"); // 생성자에서 파일 open
    if (!file.is_open()) throw std::runtime_error("파일 열기 실패");

    // 예외 발생하더라도 소멸자에서 자동 close
    throw std::runtime_error("읽기 실패");
} // file 소멸 → close() 자동 호출
```

➡️ 예외가 나도 `file` 객체의 소멸자가 호출되며 파일 자동 정리.

---

## ⚙️ 2️⃣ **예외 안전성(Exception Safety)**

| 수준                                  | 보장 내용                       |
| ----------------------------------- | --------------------------- |
| **기본 보장 (Basic Guarantee)**         | 예외 발생 시 자원 누수 없음, 객체 일관성 유지 |
| **강력 보장 (Strong Guarantee)**        | 예외 발생 시 프로그램 상태가 예외 전과 동일   |
| **nothrow 보장 (No-throw Guarantee)** | 절대 예외를 던지지 않음               |

RAII는 최소 **기본 보장**을 항상 제공한다.
소멸자는 예외를 던지지 않으므로(기본적으로 `noexcept`),
예외 발생 여부와 관계없이 자원이 정리된다.

---

## ⚙️ 3️⃣ **스택 언와인딩(stack unwinding)과의 관계**

> 예외 발생 시, 스택 언와인딩 과정에서 **스택에 존재하는 모든 객체의 소멸자**가 자동 호출된다.

```cpp
void func() {
    std::lock_guard<std::mutex> lock(mtx); // RAII
    throw std::runtime_error("예외 발생");
} // 여기서 lock 해제 자동 수행
```

➡️ 예외로 인해 함수가 종료되어도
`lock_guard`의 소멸자가 호출 → **락 자동 반환**

---

## ⚙️ 4️⃣ **대표적인 RAII 클래스**

| 클래스               | 관리 자원   | 설명                           |
| ----------------- | ------- | ---------------------------- |
| `std::unique_ptr` | 동적 메모리  | delete 자동 호출                 |
| `std::shared_ptr` | 공유 메모리  | 참조 카운트 기반 관리                 |
| `std::lock_guard` | mutex 락 | 범위 기반 락 해제                   |
| `std::fstream`    | 파일 핸들   | close 자동 수행                  |
| `std::thread`     | 실행 스레드  | join/detach 자동 처리 (C++11 이후) |

---

### 📘 예시 — lock_guard로 예외 안전성 확보

```cpp
std::mutex m;

void CriticalSection() {
    std::lock_guard<std::mutex> lock(m); // 락 획득
    DoWork(); // 예외 발생 가능
} // 예외 발생해도 lock 해제됨 (소멸자 자동 호출)
```

➡️ 락 해제 코드(`unlock()`)를 따로 쓸 필요 없음
➡️ **try/catch가 없어도 안전하게 자원 해제 보장**

---

## ⚙️ 5️⃣ **예외가 발생해도 안전한 이유**

1. **자원은 객체 내부에 캡슐화되어 있음**
   → 외부에서 수동 해제가 필요 없음
2. **스택 기반 생명주기 관리**
   → 함수가 끝나면 자동 소멸
3. **소멸자는 예외를 던지지 않음 (`noexcept`)**
   → 스택 언와인딩 중 추가 예외 방지
4. **C++ 언어 차원의 자동 호출 보장**
   → try-catch 없이도 정리 수행

---

## ⚙️ 6️⃣ **RAII 패턴의 일반 구조**

```cpp
template<typename T>
class ResourceGuard {
    T* _res;
public:
    explicit ResourceGuard(T* res) : _res(res) {}
    ~ResourceGuard() { Release(_res); } // 항상 실행
};
```

> 생성자에서 획득, 소멸자에서 해제.
> 예외가 발생하더라도 소멸자는 반드시 호출됨.

---

## 🎯 **면접용 정리**

> RAII는 **자원 획득을 객체 초기화에 결합시켜**
> 예외가 발생하더라도 **소멸자를 통해 자원을 자동 해제**하는 패턴입니다.
>
> C++의 **스택 기반 객체 파괴 규칙**(스택 언와인딩)에 의해
> try/catch 없이도 **자원 누수를 원천적으로 방지**합니다.

---

### 🔥 꼬리질문 예상

1. **Q. 소멸자가 예외를 던지면 어떻게 되나요?**
   → 스택 언와인딩 중 또 다른 예외 발생 시 `std::terminate()`가 호출됩니다.

2. **Q. RAII는 C 언어에서는 불가능한가요?**
   → 네, C에는 소멸자가 없기 때문에 수동 관리가 필요합니다.

3. **Q. RAII와 스마트 포인터의 관계는?**
   → 스마트 포인터는 RAII를 적용한 대표적인 메모리 관리 클래스입니다.

4. **Q. RAII의 핵심 이점 한 문장으로?**
   → “예외가 발생해도 자원이 자동 해제된다.”

</details>


## 35. vector, list, deque의 구조적 차이와 각각 적합한 상황을 설명해보세요.

<details close>
<summary><strong>💡 자세한 정답</strong></summary>

<div style="background-color:#f6f8fa; padding:15px; border-radius:10px;">

## 💡 **핵심 개념**

> `std::vector`, `std::list`, `std::deque`는 모두 **시퀀스 컨테이너(Sequence Container)** 로,
> 내부 구조와 메모리 관리 방식이 다르기 때문에
> **삽입·삭제·접근 성능이 서로 다르다.**

---

## ⚙️ 1️⃣ **내부 구조 요약**

| 컨테이너       | 내부 구조                          | 메모리 연속성 | 접근 속도       | 삽입/삭제 성능                  |
| ---------- | ------------------------------ | ------- | ----------- | ------------------------- |
| **vector** | 동적 배열 (Dynamic Array)          | O (연속)  | O(1) 랜덤 접근  | 끝: O(1) / 중간: O(n)        |
| **list**   | 이중 연결 리스트 (Doubly Linked List) | X       | O(n) 순차 접근  | 어느 위치든 O(1) (iterator 기준) |
| **deque**  | 블록 배열 (Segmented Array)        | 부분 연속   | O(1) 양방향 접근 | 앞/뒤 삽입 O(1) / 중간 O(n)     |

---

## ⚙️ 2️⃣ **std::vector**

### 📘 구조

* 내부는 **하나의 연속된 동적 배열**
* 메모리 재할당 시 전체 복사 발생

```cpp
std::vector<int> v;
v.push_back(1); // 끝 삽입 빠름
```

### 📌 특징

| 항목      | 설명                            |
| ------- | ----------------------------- |
| 연속된 메모리 | 배열처럼 인덱스로 O(1) 접근 가능          |
| 삽입/삭제   | 끝에서만 효율적 (push_back/pop_back) |
| 중간 삽입   | 전체 복사 필요 → O(n)               |
| 재할당     | capacity 초과 시 2배 확장 + 전체 이동   |
| 캐시 친화성  | 매우 높음 (연속 메모리)                |

### ✅ **적합한 상황**

* 데이터 크기가 점진적으로 커짐
* **랜덤 접근**이 잦음
* **읽기 중심 구조 (lookup-heavy)**

예: 게임 서버의 **세션 리스트**, **버퍼 관리**, **ID 테이블**

---

## ⚙️ 3️⃣ **std::list**

### 📘 구조

* **이중 연결 리스트 (Doubly Linked List)**
* 각 노드가 데이터 + 이전/다음 포인터를 가짐

```cpp
std::list<int> l;
l.push_back(10);
l.push_front(5);
```

### 📌 특징

| 항목      | 설명                    |
| ------- | --------------------- |
| 메모리 비연속 | 각 노드가 heap에 따로 존재     |
| 삽입/삭제   | iterator 위치 기준으로 O(1) |
| 랜덤 접근   | 불가능 (O(n))            |
| 캐시 효율   | 낮음 (불연속 메모리 접근)       |

### ✅ **적합한 상황**

* 삽입/삭제가 매우 빈번한 경우
* iterator 안정성이 중요한 경우
* 요소 주소가 자주 바뀌면 안 되는 경우

예: **JobQueue**, **이벤트 체인**, **linked buffer 시스템**

---

## ⚙️ 4️⃣ **std::deque**

### 📘 구조

* **Segmented Array (분절 배열)**
  → 여러 개의 고정 크기 블록을 관리하는 “이중 버퍼 구조”

```cpp
std::deque<int> dq;
dq.push_front(1);
dq.push_back(2);
```

### 📌 특징

| 항목          | 설명                             |
| ----------- | ------------------------------ |
| 앞뒤 모두 빠른 삽입 | push_front / push_back 모두 O(1) |
| 중간 삽입       | O(n)                           |
| 랜덤 접근       | O(1) (vector처럼 인덱스로 접근 가능)     |
| 메모리         | 여러 블록으로 분산 (완전 연속은 아님)         |
| 재할당         | 필요 없음 (동적 블록 추가)               |

### ✅ **적합한 상황**

* **앞뒤 양방향 삽입/삭제** 모두 필요한 경우
* 크기 변동이 매우 잦은 버퍼 구조

예: **패킷 큐**, **로그 버퍼**, **Task Queue**

---

## ⚙️ 5️⃣ **성능 비교 요약**

| 연산                    | `vector` | `list` | `deque`        |
| --------------------- | -------- | ------ | -------------- |
| push_back             | O(1)*    | O(1)   | O(1)           |
| push_front            | O(n)     | O(1)   | O(1)           |
| insert (중간)           | O(n)     | O(1)   | O(n)           |
| erase (중간)            | O(n)     | O(1)   | O(n)           |
| random access         | O(1)     | O(n)   | O(1)           |
| memory locality       | 매우 좋음    | 매우 나쁨  | 보통             |
| iterator invalidation | 재할당 시 무효 | 안정적    | 블록 재배치 시 일부 무효 |

> *vector의 push_back은 capacity 초과 시 재할당 때문에 일시적 O(n)

---

## ⚙️ 6️⃣ **메모리 구조 예시**

```
vector: [0][1][2][3][4][5][6]
list:   [data | prev | next] -> [data | prev | next]
deque:  [block0][block1][block2]
```

---

## 🎯 **면접용 정리**

> * **vector** : 연속 메모리, 랜덤 접근 빠름, 삽입/삭제 느림
> * **list** : 비연속 메모리, 삽입/삭제 빠름, 접근 느림
> * **deque** : 양방향 삽입 빠름, 부분 연속 메모리, 중간 삽입 느림
>
> 따라서 **읽기 중심 → vector**,
> **삽입 중심 → list**,
> **앞뒤 큐 형태 → deque**가 적합하다.

---

### 🔥 꼬리질문 예상

1. **Q. vector에서 중간 삽입이 느린 이유는?**
   → 연속 메모리를 유지하기 위해 뒤 요소를 모두 복사/이동해야 하기 때문.

2. **Q. list는 캐시 효율이 낮은 이유는?**
   → 메모리가 불연속적이라 CPU 캐시 적중률이 떨어진다.

3. **Q. deque가 vector보다 유리한 상황은?**
   → push_front가 자주 필요한 경우.

4. **Q. vector의 iterator가 무효화되는 시점은?**
   → capacity 증가(재할당) 시, 모든 iterator/pointer/reference가 무효화된다.

</details>

## 36. map과 unordered_map의 내부 구조와 시간 복잡도 차이를 설명해보세요.

<details close>
<summary><strong>💡 자세한 정답</strong></summary>

<div style="background-color:#f6f8fa; padding:15px; border-radius:10px;">

## 💡 **핵심 개념**

> `std::map`과 `std::unordered_map`은 모두
> **Key-Value 기반의 연관 컨테이너(Associative Container)** 이지만,
> 내부 자료구조와 정렬/탐색 방식이 완전히 다르다.
>
> * `map` → **Red-Black Tree (정렬 기반 이진 탐색 트리)**
> * `unordered_map` → **Hash Table (해시 기반 버킷 구조)**

---

## ⚙️ 1️⃣ **std::map 구조**

### 📘 내부 구조

* **Red-Black Tree (균형 이진 탐색 트리)**
* 모든 노드가 **Key 기준으로 정렬 상태 유지**
* 삽입, 삭제 후에도 트리 균형 자동 유지

```cpp
std::map<int, std::string> m;
m[2] = "B";
m[1] = "A";
m[3] = "C";
```

➡️ 내부적으로 `1 < 2 < 3` 순으로 정렬되어 저장됨.

---

### 📌 특징

| 항목       | 설명                                 |
| -------- | ---------------------------------- |
| 정렬 유지    | 항상 key 기준 오름차순 (기본적으로 `operator<`) |
| 탐색/삽입/삭제 | O(log N)                           |
| 반복자 순회   | 정렬된 순서로 O(N)                       |
| 메모리 구조   | 노드 기반 (각 노드에 key/value/포인터 존재)     |
| 키 중복     | 불가 (`std::multimap` 사용 시 허용)       |

---

### ✅ **장점**

* 정렬된 데이터 유지
* 범위 탐색(`lower_bound`, `upper_bound`) 효율적
* 일정한 성능 보장 (O(log N))

### ⚠️ **단점**

* 삽입 시 균형 유지 비용 존재
* 캐시 효율 낮음 (노드 비연속 메모리)

---

## ⚙️ 2️⃣ **std::unordered_map 구조**

### 📘 내부 구조

* **Hash Table (해시 테이블)**
* Key를 해시 함수로 버킷(bucket)에 매핑
* 동일 버킷 내 충돌 시 **체인 구조(Linked List or Open Addressing)** 로 관리

```cpp
std::unordered_map<int, std::string> um;
um[10] = "Ten";
um[20] = "Twenty";
```

➡️ Key는 해시값 순으로 저장되며 **정렬되지 않음**.

---

### 📌 특징

| 항목       | 설명                                 |
| -------- | ---------------------------------- |
| 정렬 여부    | 없음 (해시값 순)                         |
| 탐색/삽입/삭제 | 평균 O(1), 최악 O(N)                   |
| 메모리 구조   | 해시 버킷 배열 + 연결 리스트                  |
| 키 중복     | 불가 (`unordered_multimap` 사용 시 허용)  |
| 해시 함수    | `std::hash<Key>` 기본 제공 (커스터마이즈 가능) |

---

### ✅ **장점**

* 빠른 평균 접근 속도 (O(1))
* 대용량 데이터 탐색에 유리
* key 정렬이 불필요한 경우 효율적

### ⚠️ **단점**

* 해시 충돌 시 성능 급락 (최악 O(N))
* 순회 순서가 예측 불가
* 메모리 사용량 큼 (버킷 + 체인 오버헤드)

---

## ⚙️ 3️⃣ **시간 복잡도 비교**

| 연산            | `std::map` | `std::unordered_map` |
| ------------- | ---------- | -------------------- |
| 탐색(find)      | O(log N)   | 평균 O(1), 최악 O(N)     |
| 삽입(insert)    | O(log N)   | 평균 O(1), 최악 O(N)     |
| 삭제(erase)     | O(log N)   | 평균 O(1), 최악 O(N)     |
| 순회(iteration) | 정렬 순서 O(N) | 임의 순서 O(N)           |

---

## ⚙️ 4️⃣ **메모리 구조 비교**

```
std::map (Red-Black Tree)
          [5]
         /   \
      [3]    [8]
     /  \    / \
   [1] [4] [6] [9]

std::unordered_map (Hash Table)
Bucket[0]: [key=8]
Bucket[1]: [key=1] -> [key=9]
Bucket[2]: [key=5]
Bucket[3]: [key=3] -> [key=4]
```

---

## ⚙️ 5️⃣ **언제 어떤 걸 써야 하나**

| 상황                           | 추천 컨테이너         | 이유            |
| ---------------------------- | --------------- | ------------- |
| 키 정렬이 필요할 때                  | `map`           | 트리 기반 정렬 유지   |
| 빠른 탐색이 중요할 때                 | `unordered_map` | 평균 O(1) 탐색    |
| 범위 기반 탐색(`lower_bound`) 필요 시 | `map`           | 정렬 필요         |
| 메모리보다 속도 우선                  | `unordered_map` | 해시 기반 접근      |
| 예측 가능한 순회 필요                 | `map`           | 오름차순 정렬 순회 가능 |

---

## ⚙️ 6️⃣ **실무 예시**

| 분야             | 사용 예시                                          |
| -------------- | ---------------------------------------------- |
| **게임 서버**      | 세션 ID → 세션 객체 (`unordered_map<int, Session*>`) |
| **DB 캐시 관리**   | Key 정렬, 범위 탐색 (`map`)                          |
| **패킷 라우팅 테이블** | 빠른 lookup (`unordered_map`)                    |

---

### 📘 예시 — unordered_map 활용 (IOCP 세션 관리)

```cpp
std::unordered_map<uint64_t, Session*> sessions;

void Register(Session* s) {
    sessions[s->id] = s; // 평균 O(1)
}

Session* Find(uint64_t id) {
    auto it = sessions.find(id);
    return (it != sessions.end()) ? it->second : nullptr;
}
```

➡️ 연결 수가 수천~수만 개인 서버 환경에서
O(1) 탐색이 가능한 `unordered_map`이 압도적으로 효율적.

---

## 🎯 **면접용 정리**

> `std::map`은 **Red-Black Tree** 기반으로 **정렬된 키 순서 유지** 및 **O(log N)** 탐색을 보장하고,
> `std::unordered_map`은 **Hash Table** 기반으로 **평균 O(1)** 탐색이 가능하지만,
> 해시 충돌 시 **최악 O(N)** 으로 성능이 저하됩니다.
>
> 따라서 **정렬이 필요하면 map**, **빠른 lookup이 필요하면 unordered_map**을 사용합니다.

---

### 🔥 꼬리질문 예상

1. **Q. unordered_map이 느려질 수 있는 경우는?**
   → 해시 충돌이 심할 때 (모든 키가 같은 버킷에 몰릴 경우 O(N)).

2. **Q. map에서 lower_bound()가 빠른 이유는?**
   → 트리 구조라 logN 시간에 “이상/이하” 경계 탐색 가능.

3. **Q. unordered_map에서 key 순회 순서가 일정하지 않은 이유는?**
   → 내부적으로 해시 버킷 배열 순서에 따라 저장되기 때문.

4. **Q. 해시 함수 커스터마이징은 어떻게 하나요?**
   → `unordered_map<Key, Value, CustomHash>` 형태로 세 번째 인자 전달.

</details>

## 37. priority_queue는 내부적으로 어떤 자료구조로 구현되어 있나요?

<details close>
<summary><strong>💡 자세한 정답</strong></summary>

<div style="background-color:#f6f8fa; padding:15px; border-radius:10px;">

## 💡 **priority_queue란?**

> `std::priority_queue`는 **가장 우선순위가 높은 원소를 빠르게 추출하기 위한 컨테이너 어댑터(Container Adapter)** 이다.
>
> 내부적으로는 **힙(Heap)** 자료구조로 구현되어 있으며,
> 기본적으로 **최대 힙(Max-Heap)** 형태로 동작한다.

---

## ⚙️ 1️⃣ **내부 자료구조**

`priority_queue`는 실제로 **힙 구조를 유지하는 `std::vector` 위에 구현된 컨테이너 어댑터**다.

```cpp
template <class T,
          class Container = std::vector<T>,
          class Compare = std::less<typename Container::value_type>>
class priority_queue;
```

| 구성 요소                      | 역할                                                  |
| -------------------------- | --------------------------------------------------- |
| **Container (기본: vector)** | 힙 원소 저장 (동적 배열)                                     |
| **Compare (기본: less)**     | 비교 함수 (정렬 기준 정의)                                    |
| **알고리즘**                   | `std::make_heap`, `std::push_heap`, `std::pop_heap` |

---

## ⚙️ 2️⃣ **힙(Heap) 구조 설명**

> 힙은 **완전이진트리(Complete Binary Tree)** 형태의 자료구조로,
> 부모 노드가 자식보다 항상 크거나(최대 힙), 작거나(최소 힙) 하는 **힙 속성(Heap Property)** 을 유지한다.

📘 예시 — 최대 힙 (Max Heap)

```
        [50]
       /    \
    [30]    [40]
    /  \    /  \
  [10][20][35][25]
```

➡️ `priority_queue`는 내부적으로 **이 트리를 배열(vector) 형태로 저장**

```text
index: 0   1   2   3   4   5   6
value: 50  30  40  10  20  35  25
```

---

## ⚙️ 3️⃣ **주요 연산 동작 원리**

| 연산                  | 내부 동작                                                  | 시간 복잡도   |
| ------------------- | ------------------------------------------------------ | -------- |
| **push()**          | `vector::push_back()` → `std::push_heap()` 호출로 힙 속성 복원 | O(log N) |
| **pop()**           | `std::pop_heap()` → 마지막 원소 제거                          | O(log N) |
| **top()**           | 최상단(루트) 원소 반환                                          | O(1)     |
| **empty(), size()** | 상태 조회                                                  | O(1)     |

---

### 📘 예시 코드

```cpp
#include <queue>
#include <vector>
#include <iostream>

int main() {
    std::priority_queue<int> pq;
    pq.push(5);
    pq.push(10);
    pq.push(3);

    std::cout << pq.top(); // 10 (가장 큰 값)
}
```

➡️ 내부적으로는 다음과 같은 힙 변환이 일어난다.

1️⃣ push(5): [5]
2️⃣ push(10): [10, 5] (힙 재정렬)
3️⃣ push(3): [10, 5, 3]
→ `top()` = 10

---

## ⚙️ 4️⃣ **min-heap (최소 힙)으로 사용하는 방법**

`Compare` 인자로 `std::greater<T>`를 넘기면 **최소 힙(min-heap)** 으로 동작한다.

```cpp
std::priority_queue<int, std::vector<int>, std::greater<int>> minHeap;

minHeap.push(5);
minHeap.push(10);
minHeap.push(3);

std::cout << minHeap.top(); // 3 (가장 작은 값)
```

---

## ⚙️ 5️⃣ **내부 구현 알고리즘 (STL heap 함수)**

| 함수                           | 설명                        |
| ---------------------------- | ------------------------- |
| `std::make_heap(begin, end)` | 주어진 구간을 힙 구조로 변환          |
| `std::push_heap(begin, end)` | 마지막에 추가된 원소를 위로 끌어올려 힙 유지 |
| `std::pop_heap(begin, end)`  | 루트(최대값)를 뒤로 보내고 힙 재구성     |
| `std::sort_heap(begin, end)` | 힙을 이용한 정렬 수행 (힙 정렬)       |

---

### 📘 예시 — 내부에서 실제로 일어나는 동작

```cpp
std::vector<int> v = {10, 5, 8};
std::make_heap(v.begin(), v.end()); // [10, 5, 8]
v.push_back(12);
std::push_heap(v.begin(), v.end()); // [12, 10, 8, 5]
std::pop_heap(v.begin(), v.end());  // [10, 5, 8, 12] (12 맨 뒤로 이동)
v.pop_back();                       // 12 제거
```

---

## ⚙️ 6️⃣ **시간 복잡도 요약**

| 연산            | 시간 복잡도   | 설명               |
| ------------- | -------- | ---------------- |
| `push()`      | O(log N) | 새 원소 삽입 후 위로 재배치 |
| `pop()`       | O(log N) | 루트 제거 후 아래로 재배치  |
| `top()`       | O(1)     | 최상단 원소 접근        |
| `make_heap()` | O(N)     | 초기 힙 구성          |

---

## ⚙️ 7️⃣ **장단점 요약**

| 항목            | 장점                | 단점            |
| ------------- | ----------------- | ------------- |
| **vector 기반** | 캐시 효율 높음 (연속 메모리) | 중간 삽입 불가      |
| **정렬 불필요**    | 항상 최대/최소 접근 O(1)  | 특정 원소 검색 O(N) |
| **일관된 성능**    | 삽입·삭제 logN 유지     | 정렬된 순회 불가능    |

---

## 🎯 **면접용 정리**

> `std::priority_queue`는 내부적으로 **vector 기반의 힙(Heap)** 구조로 구현되어 있으며,
> 기본적으로 **최대 힙(Max-Heap)** 형태로 동작합니다.
>
> 삽입(`push`)과 삭제(`pop`)는 `std::push_heap`, `std::pop_heap` 알고리즘으로
> **O(log N)** 의 시간 복잡도를 가지며,
> `top()`은 **O(1)** 로 최댓값(또는 최솟값)에 즉시 접근할 수 있습니다.

---

### 🔥 꼬리질문 예상

1. **Q. priority_queue가 정렬된 컨테이너와 다른 점은?**
   → 항상 “최대(또는 최소)” 원소만 보장하며 전체 정렬은 유지하지 않는다.

2. **Q. 최소 힙(min-heap)으로 바꾸려면?**
   → `std::greater<T>`를 비교자로 전달한다.

3. **Q. priority_queue의 내부 컨테이너는 바꿀 수 있나요?**
   → 가능하다. 기본은 `std::vector`, `std::deque`로 변경 가능.

4. **Q. push와 pop의 시간 복잡도는 왜 logN인가요?**
   → 완전이진트리에서 부모/자식 관계 비교를 통한 상하 이동이 logN 단계만에 끝나기 때문.

</details>

## 38. iterator invalidation(이터레이터 무효화)은 언제 발생하며, 왜 주의해야 하나요?

<details close>
<summary><strong>💡 자세한 정답</strong></summary>

<div style="background-color:#f6f8fa; padding:15px; border-radius:10px;">

## 💡 **iterator invalidation이란?**

> **이터레이터가 가리키던 원소나 메모리가 더 이상 유효하지 않게 되는 상황**을 말한다.
>
> 즉, 컨테이너의 구조가 변경되어 **기존 이터레이터, 포인터, 참조(reference)가 무효화되는 현상**이다.
>
> 무효화된 이터레이터를 사용하면 **정의되지 않은 동작(UB, Undefined Behavior)** 이 발생한다.

---

## ⚙️ 1️⃣ **왜 발생하나?**

컨테이너 내부 구조가 변할 때,
이터레이터가 가리키는 **메모리 주소가 바뀌거나 해제**되기 때문이다.

```cpp
std::vector<int> v = {1, 2, 3};
auto it = v.begin();
v.push_back(4); // 재할당 발생 가능
std::cout << *it; // ❌ it은 이제 무효화됨 (UB)
```

---

## ⚙️ 2️⃣ **컨테이너별 무효화 규칙 요약**

| 컨테이너              | 삽입 시                                                             | 삭제 시                  | 재할당/크기변경 시                      | 비고         |
| ----------------- | ---------------------------------------------------------------- | --------------------- | ------------------------------- | ---------- |
| **vector**        | **capacity 초과 시 모든 iterator 무효화**<br>capacity 유지 시 삽입 지점 이후만 무효화 | 삭제 지점 이후 iterator 무효화 | 재할당 시 전체 무효화                    | 연속 메모리 구조  |
| **deque**         | 중간 삽입/삭제 시 전체 무효화<br>양 끝 삽입은 일부 유지                               | 비슷하게 중간 변경 시 대부분 무효화  | 블록 단위 이동 시 부분 무효화               | 블록 배열 구조   |
| **list**          | 삽입/삭제 시 기존 iterator **유효 유지**                                    | 삭제된 노드만 무효            | 재할당 없음 (노드 기반)                  | **가장 안전함** |
| **set / map**     | 삽입/삭제 시 **다른 iterator 유지**                                       | 삭제된 원소만 무효            | 정렬 유지 (tree 구조)                 | RB-tree 기반 |
| **unordered_map** | rehash 발생 시 전체 무효화                                               | erase된 원소만 무효         | load factor 초과 시 rehash로 전체 무효화 | 해시 테이블 기반  |

---

## ⚙️ 3️⃣ **vector의 대표적인 사례**

### ✅ 무효화 발생

```cpp
std::vector<int> v = {1, 2, 3};
auto it = v.begin();
v.push_back(4); // capacity 증가 시 재할당 → 모든 iterator 무효화
```

### ✅ 안전한 사용법

```cpp
v.reserve(10);  // capacity 미리 확보
auto it = v.begin();
v.push_back(4); // 재할당 없음 → it 여전히 유효
```

> 💡 해결책: `reserve()`로 미리 메모리 확보 시 무효화 방지 가능

---

## ⚙️ 4️⃣ **list의 경우**

```cpp
std::list<int> lst = {1, 2, 3};
auto it = lst.begin();
lst.insert(it, 0); // ✅ 모든 iterator 유효
lst.erase(it);     // ❌ 해당 it만 무효
```

> 각 노드가 독립적 메모리를 가지므로 삽입/삭제에도 다른 iterator는 유지됨.

---

## ⚙️ 5️⃣ **unordered_map (hash 기반 컨테이너)**

```cpp
std::unordered_map<int, int> um;
for (int i = 0; i < 10; i++) um[i] = i;

auto it = um.begin();
um.insert({20, 20}); // rehash 발생 시 전체 iterator 무효
```

> 버킷(bucket) 재배치로 모든 iterator/pointer/reference 무효화 가능.
>
> ✅ 해결법: `reserve()`로 미리 버킷 수 확보 (`um.reserve(100)`)

---

## ⚙️ 6️⃣ **무효화 이후의 위험성**

| 문제                      | 결과                            |
| ----------------------- | ----------------------------- |
| 무효화된 iterator 역참조       | 프로그램 크래시 or 쓰레기값              |
| 무효화된 iterator로 erase 반복 | UB 발생 (segfault 가능)           |
| 메모리 재활용 시               | 이전 주소가 다른 객체로 덮여 예상치 못한 동작 발생 |

---

### 📘 예시 — 잘못된 erase 루프

```cpp
std::vector<int> v = {1, 2, 3, 4, 5};

for (auto it = v.begin(); it != v.end(); ++it) {
    if (*it % 2 == 0)
        v.erase(it); // ❌ erase 후 it 무효화 → UB
}
```

### ✅ 수정 버전

```cpp
for (auto it = v.begin(); it != v.end();) {
    if (*it % 2 == 0)
        it = v.erase(it); // erase 반환값으로 다음 iterator 갱신
    else
        ++it;
}
```

---

## ⚙️ 7️⃣ **안전하게 다루는 방법**

| 방법                | 설명                           |
| ----------------- | ---------------------------- |
| `erase()` 반환값 활용  | erase 이후 iterator 갱신         |
| `reserve()` 사용    | vector/unordered_map의 재할당 방지 |
| `list` / `map` 사용 | 삽입·삭제 시 iterator 안정성 확보      |
| 범위 기반 for문 주의     | 컨테이너 수정 시 무효화 가능             |

---

## 🎯 **면접용 정리**

> iterator invalidation은 **컨테이너 구조 변경(삽입, 삭제, 재할당)** 으로 인해
> **이터레이터나 참조가 가리키는 메모리가 사라지는 현상**입니다.
>
> 컨테이너마다 무효화 조건이 다르며,
> **vector와 unordered_map은 재할당 시 전체 무효화**,
> **list와 map은 대부분 유지**됩니다.
>
> 무효화된 iterator를 접근하면 **Undefined Behavior(UB)** 가 발생합니다.

---

### 🔥 꼬리질문 예상

1. **Q. vector의 erase 후 iterator를 안전하게 유지하려면?**
   → `erase()`의 반환 iterator를 사용해야 합니다.

2. **Q. unordered_map에서 iterator 무효화를 방지하려면?**
   → `reserve()`로 미리 버킷 수를 확보합니다.

3. **Q. list가 iterator 안전한 이유는?**
   → 노드가 독립적으로 존재하며, 삽입/삭제 시 다른 노드 주소가 바뀌지 않기 때문입니다.

4. **Q. invalid iterator 사용 시 어떤 문제가 생기나요?**
   → 프로그램 크래시, 잘못된 데이터 접근, 미정의 동작(UB).

</details>

## 39. copy-on-write(COW)란 무엇이며, 어떤 상황에서 사용되나요?

<details close>
<summary><strong>💡 자세한 정답</strong></summary>

<div style="background-color:#f6f8fa; padding:15px; border-radius:10px;">

## 💡 **copy-on-write (COW)란?**

> **Copy-On-Write**는 “복사는 필요할 때만 한다”는 의미로,
> **데이터를 여러 객체가 공유하다가 실제 수정이 일어날 때만 복사하는 기법**이다.
>
> 즉, 초기에는 **메모리를 공유**하여 복사 비용을 아끼고,
> **수정 시점에 진짜 복사(deep copy)** 를 수행한다.

---

## ⚙️ 1️⃣ **핵심 아이디어**

| 단계                              | 설명                      |
| ------------------------------- | ----------------------- |
| **① 공유(copy by reference)**     | 새 객체는 기존 데이터의 주소를 참조만 함 |
| **② 참조 카운트 증가**                 | 공유된 데이터의 ref-count += 1 |
| **③ 수정 발생 시 복사(copy-on-write)** | 수정하려는 시점에만 실제 데이터 복제    |
| **④ 참조 카운트 분리**                 | 원본과 복제본이 각각 독립적으로 동작    |

---

### 📘 예시 개념도

```
초기 상태:
  A.data ─┐
           ├─► [Shared Buffer] (refCount = 2)
  B.data ─┘

수정 발생 시:
  B.data ─► [Copied Buffer] (refCount = 1)
  A.data ─► [Original Buffer] (refCount = 1)
```

➡️ 읽기만 할 때는 메모리 공유
➡️ 쓰기 발생 시에만 복사 수행

---

## ⚙️ 2️⃣ **예시 코드**

```cpp
class CowString {
    std::shared_ptr<std::string> data;
public:
    CowString(const std::string& str) : data(std::make_shared<std::string>(str)) {}

    // 쓰기 접근 시 복사
    void modify(char c) {
        if (!data.unique()) // 여러 객체가 공유 중이라면
            data = std::make_shared<std::string>(*data); // 깊은 복사 수행
        data->push_back(c);
    }

    const std::string& get() const { return *data; }
};
```

📌 **핵심**:

* 읽기 시에는 공유 (참조만)
* 수정 시에는 복사 (진짜 쓰기 발생)

---

## ⚙️ 3️⃣ **COW의 장점**

| 항목                   | 설명                         |
| -------------------- | -------------------------- |
| **성능 최적화**           | 불필요한 복사를 피함 (메모리 + CPU 절약) |
| **메모리 효율성**          | 여러 객체가 같은 데이터 공유           |
| **지연 복사(Lazy Copy)** | 실제로 수정할 때까지 복사 안 함         |

---

## ⚙️ 4️⃣ **단점 및 주의점**

| 항목                 | 설명                                 |
| ------------------ | ---------------------------------- |
| **스레드 안전성 문제**     | 여러 스레드가 공유한 객체를 동시에 수정하면 데이터 경합 발생 |
| **복잡한 관리**         | 참조 카운트 + 쓰기 시점 감지 로직 필요            |
| **현대 C++에서 사용 제한** | C++11 이후 move semantics로 대체됨       |

---

## ⚙️ 5️⃣ **사용 사례**

| 사용 위치                                 | 설명                                                 |
| ------------------------------------- | -------------------------------------------------- |
| **문자열 클래스 (과거 std::string)**          | C++03까지는 COW 구현 존재 (읽기 중심 환경에서 유리)                 |
| **이미지/텍스처 캐시 시스템**                    | 이미지 공유 후, 편집 시점에만 복사                               |
| **DB 버퍼 관리 (Copy-On-Write Snapshot)** | 트랜잭션 격리 — 수정 중에도 다른 스냅샷은 안전하게 유지                   |
| **OS 가상 메모리**                         | `fork()` 호출 시 페이지를 공유, 쓰기 발생 시 복사 (page-level COW) |

---

### 📘 OS 레벨 COW 예시 (Linux)

```text
Parent process fork() → Child shares same pages
Write to shared page → Page fault → Copy page for child only
```

➡️ 메모리 복사 비용을 “쓰기 시점”까지 지연시켜 성능 최적화.

---

## ⚙️ 6️⃣ **C++11 이후의 대체 기술**

| 기술                      | 설명                               |
| ----------------------- | -------------------------------- |
| **Move Semantics**      | 자원의 “소유권 이동”으로 COW의 복사 최적화 문제 해결 |
| **std::shared_ptr**     | 내부 참조 카운팅으로 안전한 공유 제공            |
| **Immutable Object 설계** | 불변 객체 기반의 안전한 공유 구조 활용           |

📌 현대 C++에서는 COW보다 **move semantics + smart pointer 조합**이 더 효율적이고 안전함.

---

## ⚙️ 7️⃣ **COW vs Move Semantics 비교**

| 구분             | Copy-On-Write | Move Semantics |
| -------------- | ------------- | -------------- |
| 복사 시점          | 쓰기 발생 시       | 명시적 이동 시       |
| 메모리 공유         | 있음            | 없음             |
| 스레드 안전성        | 낮음            | 높음             |
| 구현 복잡도         | 높음            | 낮음             |
| C++11 이후 권장 여부 | ❌ 비권장         | ✅ 적극 사용        |

---

## 🎯 **면접용 정리**

> **Copy-On-Write(COW)**는 여러 객체가 같은 데이터를 **공유하다가**,
> **수정이 필요할 때만 복사**하는 지연 복사(Lazy Copy) 기법입니다.
>
> 읽기 중심 환경에서는 메모리 절약과 성능 이점이 있지만,
> 스레드 환경에서는 안전하지 않고,
> C++11 이후에는 **move semantics**로 대부분 대체되었습니다.

---

### 🔥 꼬리질문 예상

1. **Q. C++11 이후 std::string에서 COW가 사라진 이유는?**
   → move semantics 도입으로 불필요해졌고, 스레드 안전성 문제 때문입니다.

2. **Q. COW는 주로 어디서 사용되나요?**
   → OS의 가상 메모리(`fork`), DB 스냅샷, 이미지 편집기 등 “읽기 위주” 시스템.

3. **Q. COW와 참조 카운팅(shared_ptr)의 차이점은?**
   → shared_ptr은 단순 공유, COW는 공유 + 쓰기 시점 복사.

4. **Q. 멀티스레드 환경에서 COW의 문제는?**
   → 동시에 수정 시 race condition 발생 가능, deep copy 중에도 충돌 위험.

</details>
