# 🧬 얕은 복사(Shallow Copy) vs 깊은 복사(Deep Copy)

이 문서는 C++에서 객체 복사 시 자주 언급되는 **얕은 복사**와 **깊은 복사**의 개념, 차이점, 그리고 복사 생성자 및 대입 연산자의 재정의 필요성에 대해 정리한 문서입니다.  
게임 서버 프로그래머 면접에서 **자원 관리**, **메모리 복사**, **클래스 복사 시 안전성**을 다룰 때 거의 반드시 나오는 주제입니다.

---

## 📌 복사의 종류 요약

| 구분 | 설명 | 특징 | 예시 상황 |
|------|------|------|----------|
| 얕은 복사 (Shallow Copy) | 객체의 멤버 데이터를 **비트열 그대로 복사** | 포인터 멤버가 있다면 **같은 메모리 주소를 공유** | 기본 복사 생성자, 대입 연산자 사용 시 |
| 깊은 복사 (Deep Copy) | 객체의 멤버 데이터를 **새 메모리에 복사** | 포인터 멤버가 있다면 **별도 메모리 할당 후 복사** | 리소스 독립성을 유지해야 할 때 필수 |

---

## 🧪 얕은 복사 (Shallow Copy)

### 🔸 개념
- C++에서 객체 복사는 기본적으로 **비트 단위 복사**로 수행됩니다.
- 이는 모든 멤버 변수(포인터 포함)의 **주소값 그대로 복사**한다는 뜻입니다.
- 두 객체가 **동일한 힙 자원**을 공유하게 되어, 한 객체가 해당 리소스를 삭제하면 다른 객체도 영향을 받습니다.

### ⚠️ 위험 예시

```cpp
class Pet {
public:
    char* name;

    Pet(const char* initName) {
        name = new char[strlen(initName) + 1];
        strcpy(name, initName);
    }

    ~Pet() {
        delete[] name;
    }
};

class Player {
public:
    Pet* pet;

    Player(const char* petName) {
        pet = new Pet(petName);
    }

    ~Player() {
        delete pet;
    }
};

int main() {
    Player player1("Dragon");
    Player player2 = player1; // 얕은 복사 (컴파일러가 생성한 기본 복사 생성자 사용)

    delete player1.pet; // player2.pet도 같은 주소를 참조 중 → 이중 해제 위험 (Double Free)
}
```

### 🔥 문제 요약
- `player2.pet`은 `player1.pet`과 **같은 주소를 가리킴**
- `player1` 소멸 시 `pet` 삭제 → `player2`도 같은 주소를 참조 → **이중 해제(double free)** 또는 **use-after-free** 발생 가능

---

## 💎 깊은 복사 (Deep Copy)

### 🔸 개념
- 객체가 참조하는 **포인터 멤버의 실제 데이터까지** 새로 생성해 복사합니다.
- 복사 생성자와 복사 대입 연산자를 **사용자 정의**하여 구현합니다.
- 객체 간 리소스 독립성을 보장합니다.

### ✅ 안전한 예시

```cpp
class Pet {
public:
    char* name;

    Pet(const char* initName) {
        name = new char[strlen(initName) + 1];
        strcpy(name, initName);
    }

    Pet(const Pet& other) { // 복사 생성자 (깊은 복사)
        name = new char[strlen(other.name) + 1];
        strcpy(name, other.name);
    }

    Pet& operator=(const Pet& other) { // 복사 대입 연산자 (깊은 복사)
        if (this == &other) return *this;

        delete[] name;
        name = new char[strlen(other.name) + 1];
        strcpy(name, other.name);
        return *this;
    }

    ~Pet() {
        delete[] name;
    }
};

class Player {
public:
    Pet* pet;

    Player(const char* petName) {
        pet = new Pet(petName);
    }

    Player(const Player& other) { // 복사 생성자
        pet = new Pet(*other.pet);
    }

    Player& operator=(const Player& other) { // 복사 대입 연산자
        if (this == &other) return *this;

        delete pet;
        pet = new Pet(*other.pet);
        return *this;
    }

    ~Player() {
        delete pet;
    }
};

int main() {
    Player player1("Phoenix");
    Player player2 = player1; // 안전한 복사
}
```

---

## 👁️ 면접 포인트

- **Q. C++에서 복사 생성자와 복사 대입 연산자를 왜 재정의하나요?**  
  👉 기본 제공되는 복사는 얕은 복사이며, 포인터 멤버를 안전하게 복사하려면 깊은 복사가 필요합니다.

- **Q. 어떤 상황에서 얕은 복사가 문제가 될 수 있나요?**  
  👉 두 객체가 같은 리소스를 참조할 때 한 객체가 해당 자원을 해제하면, 다른 객체는 **dangling pointer**를 갖게 됩니다.

- **Q. 깊은 복사를 구현할 때 주의할 점은?**  
  👉 자기 자신에 대한 대입 (`if (this == &other)`) 체크, 기존 리소스 해제 후 새로 할당 필요

---

## 📌 요약 정리

| 항목 | 얕은 복사 | 깊은 복사 |
|------|-----------|-----------|
| 복사 대상 | 멤버 값 그대로 복사 (주소 공유) | 멤버가 가리키는 데이터까지 새로 복사 |
| 리소스 충돌 가능성 | 매우 높음 (double free, use-after-free) | 없음 |
| 복사 생성자 필요 | ❌ 기본 생성자 사용 | ✅ 직접 정의 |
| 대입 연산자 필요 | ❌ 기본 연산자 사용 | ✅ 직접 정의 |
| 예시 | `Player player2 = player1;` 시 crash | 안전하게 복사 및 해제 |

---

## 🧠 실전에서의 활용

- STL 컨테이너는 내부적으로 깊은 복사를 지원하도록 구현되어 있습니다.
- 하지만 사용자 정의 클래스에서 포인터를 사용한다면, **직접 깊은 복사를 구현하지 않으면 반드시 문제**가 발생합니다.
- 게임 서버에서는 자주 객체를 복제하거나, 상태 스냅샷을 저장해야 하는 상황이 많기 때문에 **깊은 복사의 이해는 필수입니다.**

---

> 📚 관련 키워드: 얕은 복사, 깊은 복사, 복사 생성자, 복사 대입 연산자, 포인터, 힙 메모리, 자원 해제, 게임 객체 복사

