# 🔷 C++ 객체지향 정리 - Part 2  
> 상속(Inheritance) & 은닉성(Encapsulation) 중심

게임 서버 개발과 같은 복잡한 시스템에서는 코드 재사용성과 데이터 보호가 매우 중요합니다.  
**상속과 은닉성**은 이러한 목적을 달성하는 객체지향 핵심 개념입니다.

---

## 🧬 상속(Inheritance)

상속이란 **기존 클래스의 특성을 새로운 클래스에 물려주는 기능**입니다.  
코드를 중복 없이 확장할 수 있어 서버 구조 설계에 자주 활용됩니다.

### 📌 기본 문법

```cpp
class BaseClass {
    // 기본 클래스
};

class DerivedClass : public BaseClass {
    // 파생 클래스
};
```

- `public` 상속 시: `public`, `protected` 멤버가 그대로 유지됨
- `private` 상속 시: 모든 멤버가 파생 클래스에서 `private`으로 바뀜

---

## 🎮 예제: 게임 서버에서의 상속 구조

```cpp
#include <iostream>
using namespace std;

class GameEntity {
protected:
    int id;
public:
    GameEntity(int id) : id(id) {}

    void printID() {
        cout << "Entity ID: " << id << endl;
    }
};

class Player : public GameEntity {
    string name;
public:
    Player(int id, const string& name) : GameEntity(id), name(name) {}

    void introduce() {
        cout << "Player [" << name << "] with ID " << id << " is online." << endl;
    }
};

int main() {
    Player p1(101, "Archer");
    p1.printID();
    p1.introduce();
}
```

### ✅ 실행 결과 예시

```
Entity ID: 101
Player [Archer] with ID 101 is online.
```

> `Player` 클래스는 `GameEntity`의 멤버와 기능을 상속받아 확장합니다.  
> 이는 게임 서버에서 몬스터, 플레이어, NPC 등 공통 요소를 묶을 때 매우 유용합니다.

---

## 🔐 은닉성(Encapsulation & Hiding)

은닉성은 클래스의 내부 데이터를 보호하는 개념입니다.  
직접 접근을 막고, **제공된 인터페이스만 통해 조작하도록 유도**합니다.

### 📌 접근 지정자 요약

| 지정자 | 설명 |
|--------|------|
| `private` | 클래스 내부에서만 접근 가능 |
| `protected` | 파생 클래스에서도 접근 가능 |
| `public` | 어디서든 접근 가능 |

---

## 🧪 예제: DB 커넥션 클래스 은닉 처리

```cpp
class DBConnection {
private:
    string connectionString;
    bool connected = false;

public:
    void connect(const string& conn) {
        connectionString = conn;
        connected = true;
        cout << "DB 연결됨: " << connectionString << endl;
    }

    void disconnect() {
        connected = false;
        cout << "DB 연결 해제됨" << endl;
    }

    bool isConnected() const {
        return connected;
    }
};
```

- 내부 변수인 `connectionString`, `connected`는 외부에서 **직접 접근할 수 없음**
- `connect()`, `disconnect()` 같은 **공식 함수**만 통해 제어 가능

> 게임 서버에서 데이터베이스, 세션, 로그 등 민감한 정보를 외부 접근으로부터 보호할 수 있습니다.

---

## ✅ 정리 요약

| 개념 | 설명 |
|------|------|
| 상속 | 기존 클래스 기능을 재사용 |
| 파생 클래스 | 기본 클래스를 확장 |
| protected | 상속 전용 멤버 접근 허용 |
| 은닉성 | 내부 데이터를 숨기고 인터페이스만 노출 |
| 사용 목적 | 안정성 확보, 유지보수 용이 |

---

📌 다음 학습 예정: **다형성(Polymorphism)**  
→ 게임 내 다양한 행동을 하나의 인터페이스로 통합하는 핵심 개념!
