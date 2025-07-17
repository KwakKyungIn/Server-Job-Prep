# 🧷 C++의 static 키워드 & struct vs class

> 이 문서는 C++에서 자주 사용되는 `static` 키워드의 동작 방식과  
> `struct`와 `class` 키워드의 차이에 대해 정리한 문서입니다.  
> 특히 메모리 구조, 생명 주기, 객체 지향 설계의 관점에서 이 개념들이 어떤 의미를 가지는지를 설명합니다.

---

## 📌 static 키워드의 동작 방식

### 🔧 static이 의미하는 것

`static` 키워드는 **메모리와 생명 주기**, **링크 가능성**에 영향을 주는 키워드다.  
사용 위치에 따라 의미가 다르며, 크게 3가지 용도로 사용됨:

| 위치 | 의미 |
|------|------|
| 함수 내부 | **정적 지역 변수**로, 호출이 끝나도 값이 유지됨 |
| 클래스 내부 | **정적 멤버 변수/함수**로, 클래스 단위로 존재 |
| 전역 or 파일 스코프 | 외부 링크를 막아 **내부 연결**로 제한 |

---

### 📁 정적 지역 변수

```cpp
void Increment() {
    static int counter = 0;
    counter++;
    std::cout << "Counter: " << counter << std::endl;
}
```

- `counter`는 함수가 여러 번 호출되어도 **값이 유지됨**
- 한 번만 초기화되고, 종료 시까지 살아있음
- 메모리 상 `.data` 또는 `.bss` 영역에 위치

---

### 🏗️ 클래스의 static 멤버

```cpp
class Player {
public:
    static int totalPlayers;

    Player() { totalPlayers++; }
};

int Player::totalPlayers = 0; // 반드시 클래스 외부에서 정의 필요
```

- 객체가 아닌 **클래스 단위**로 존재
- 모든 인스턴스가 **같은 static 변수**를 공유
- 객체가 없어도 `Player::totalPlayers`처럼 접근 가능
- 메모리는 `.bss` 또는 `.data` 영역에 존재

---

### 📌 메모리 영역: .data vs .bss

| 구분 | 의미 | 위치 |
|------|------|------|
| 초기화된 static 변수 | 명시적으로 값을 가짐 | `.data` 영역 |
| 초기화되지 않은 static 변수 | 암묵적으로 0 | `.bss` 영역 |

```cpp
static int x = 10; // -> .data
static int y;       // -> .bss (자동으로 0)
```

---

### 🧬 생명 주기와 가시 범위

| 구분 | 생명 주기 | 가시 범위 |
|------|-----------|------------|
| static 지역 변수 | 프로그램 시작 ~ 종료 | 함수 내부 |
| static 전역 변수 | 프로그램 시작 ~ 종료 | 파일 내부 |
| static 멤버 변수 | 프로그램 시작 ~ 종료 | 클래스 내부 (접근은 클래스 또는 객체로) |

---

## 🧱 struct vs class

### 🔍 기본 차이: 접근 지정자

| 키워드 | 기본 접근 지정자 |
|--------|------------------|
| struct | `public` |
| class  | `private` |

이 차이만 제외하면 **기능상 차이는 없음**.  
둘 다 생성자, 멤버 함수, 상속 등 **완전한 객체지향 기능**을 제공함.

```cpp
struct Item {
    int id;
    void PrintID() { std::cout << id << std::endl; }
};

class Weapon {
    int damage;
public:
    Weapon(int d) : damage(d) {}
    void Print() { std::cout << damage << std::endl; }
};
```

---

### 🧭 관습적 용도 차이

| 구분 | 용도 | 특징 |
|------|------|------|
| struct | 데이터 묶음 표현 | POD(Plain Old Data)로 자주 사용 |
| class | 객체 지향적 설계 | 정보 은닉, 캡슐화 중심 |

즉, `struct`는 단순 데이터를 담기 위한 **경량 구조체** 용도로,  
`class`는 캡슐화와 다형성을 가진 **로직 중심 구조**로 자주 쓰임.

---

### 🎮 게임 서버 예시: 상태 패킷

```cpp
// struct로 간단한 데이터 묶음 표현
struct PositionPacket {
    int playerId;
    float x, y, z;
};

// class로 복잡한 동작 포함
class GameObject {
    int id;
    float x, y, z;
public:
    GameObject(int id) : id(id), x(0), y(0), z(0) {}
    void Move(float dx, float dy, float dz) {
        x += dx; y += dy; z += dz;
    }
    void Print() {
        std::cout << "ID: " << id << " Pos: " << x << ", " << y << ", " << z << std::endl;
    }
};
```

---

## ✅ 요약

| 주제 | 핵심 내용 |
|------|----------|
| `static` | 클래스 단위 변수, 정적 지역 변수, 내부 링크 등을 위한 키워드 |
| 메모리 영역 | 초기화 → `.data`, 비초기화 → `.bss` |
| 생명 주기 | 프로그램 전체에 걸쳐 존재 |
| struct vs class | 기본 접근 지정자 차이, 용도상 관습 차이 존재 |

---

> ✨ `static`과 `struct/class`의 개념은 **메모리 구조**, **OOP 설계**, **캡슐화 전략**에 영향을 주며,  
> 서버 개발 시 전역 설정, 싱글톤 구현, 패킷 구조 등 다양한 영역에서 활용됩니다.
