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