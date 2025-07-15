# 🔷 C++ 객체지향 기초 정리

C++은 강력한 **객체지향(Object-Oriented Programming)** 언어로, 대규모 프로그램이나 게임 서버와 같은 복잡한 구조의 소프트웨어를 설계하는 데 적합합니다.  
객체지향 프로그래밍은 현실 세계의 사물을 객체로 모델링하여 **유지보수성**과 **확장성**을 높입니다.

---

## 🧱 클래스(Class)와 객체(Object)

### ✅ 클래스란?

- 객체를 만들기 위한 **설계도**
- 속성(멤버 변수)과 동작(멤버 함수)으로 구성됨

### ✅ 객체란?

- 클래스를 바탕으로 생성된 **실체(인스턴스)**

```cpp
class Player {
public:
    std::string name;
    int level;

    void introduce() {
        std::cout << "Player: " << name << " / Level: " << level << std::endl;
    }
};

int main() {
    Player p1;
    p1.name = "Knight";
    p1.level = 10;
    p1.introduce();
}
```

---

## 🔧 생성자(Constructor)

객체가 생성될 때 자동으로 호출되는 **특수한 함수**입니다.

### 📌 특징

- 클래스 이름과 같으며 **반환형이 없음**
- **오버로딩 가능**: 여러 개 정의 가능
- 객체의 **초기값 설정**에 사용됨

### 🧩 생성자의 종류

| 종류 | 설명 |
|------|------|
| 기본 생성자 | 인자가 없는 생성자 |
| 매개변수 생성자 | 특정 값을 받아 초기화 |
| 타입 변환 생성자 | 하나의 매개변수를 받아 객체로 자동 변환 가능 |

```cpp
class Session {
    int id;
public:
    Session() {
        id = 0;
        std::cout << "Default Session started\n";
    }

    Session(int sessionId) {
        id = sessionId;
        std::cout << "Session #" << id << " started\n";
    }

    // 타입 변환 생성자
    Session(double sessionId) {
        id = static_cast<int>(sessionId);
        std::cout << "Session from double ID started\n";
    }
};
```

> ⚠️ 암시적 변환을 막고 싶을 경우 `explicit` 키워드 사용

---

## 💥 소멸자(Destructor)

객체가 **수명 종료** 시 자동 호출되는 특수한 함수입니다. 리소스를 정리하거나 로그를 남길 때 사용됩니다.

### 📌 특징

- `~클래스이름()` 형식
- **매개변수와 반환형 없음**
- **단 하나만 정의 가능**

```cpp
class Network {
public:
    Network() {
        std::cout << "Network 연결됨\n";
    }

    ~Network() {
        std::cout << "Network 연결 종료\n";
    }
};

int main() {
    Network n1;
}  // 여기서 소멸자 호출
```

> 서버에서 파일, 소켓, DB 연결을 닫을 때 유용

---

## 🧠 정리 요약

| 개념 | 설명 |
|------|------|
| 클래스 | 객체의 설계도. 멤버 변수 + 함수 포함 |
| 생성자 | 객체 생성 시 자동 호출. 초기화 목적 |
| 소멸자 | 객체 소멸 시 자동 호출. 정리 작업 |
| 오버로딩 | 생성자는 여러 개 가능 (매개변수 다르게) |
| 소멸자 개수 | 단 하나만 정의 가능 |

