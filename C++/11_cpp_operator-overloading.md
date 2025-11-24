# 🧮 C++ 연산자 오버로딩 (Operator Overloading)

> 이 문서는 C++의 연산자 오버로딩(operator overloading) 개념을 정리한 자료입니다.  
> 연산자 오버로딩을 통해 **사용자 정의 타입에도 연산자 문법을 적용**할 수 있으며,  
> 특히 벡터, 포지션, 시간, 패킷 등 게임 서버 프로그래밍에 자주 활용됩니다.

---

## ❓ 연산자 오버로딩이란?

클래스에 대해 `+`, `-`, `==` 같은 연산자를 정의함으로써,  
**직관적인 코드 표현**과 **내부 로직 은닉**이 가능하게 만드는 기능.

```cpp
Vector3 a(1, 2, 3);
Vector3 b(4, 5, 6);
Vector3 result = a + b;
```

- `a + b`는 결국 내부에서 `operator+()` 함수가 호출됨.
- 객체지향적인 방식으로 계산 로직을 숨길 수 있음.

---

## 🧱 멤버 함수 vs 전역 함수

| 구분 | 예시 | 특징 |
|------|------|------|
| 멤버 함수 | `A operator+(const A& rhs);` | 왼쪽 피연산자가 해당 클래스여야 가능 |
| 전역 함수 | `operator+(const A& lhs, const A& rhs);` | 양쪽 피연산자 모두 자유롭게 설정 가능 |

---

### ✅ 멤버 연산자 함수

- `a + b` 형태일 때, `a`가 클래스여야만 작동 가능
- `a.operator+(b)`로 해석됨

```cpp
class Vector3 {
    float x, y, z;
public:
    Vector3(float x, float y, float z) : x(x), y(y), z(z) {}

    Vector3 operator+(const Vector3& other) const {
        return Vector3(x + other.x, y + other.y, z + other.z);
    }
};
```

---

### ✅ 전역 연산자 함수

- `a`가 클래스가 아니어도 작동
- 왼쪽/오른쪽 모두 피연산자로 전달받음

```cpp
class Time {
    int seconds;
public:
    explicit Time(int sec) : seconds(sec) {}

    int GetSeconds() const { return seconds; }

    friend Time operator+(const Time& lhs, const Time& rhs);
};

Time operator+(const Time& lhs, const Time& rhs) {
    return Time(lhs.GetSeconds() + rhs.GetSeconds());
}
```

---

## ❗ 대입 연산자(=)는 반드시 멤버 함수로만

```cpp
class Player {
    std::string name;
public:
    Player& operator=(const Player& other) {
        name = other.name;
        return *this;
    }
};
```

- `operator=` 는 **전역 함수로는 정의할 수 없음**
- 반드시 멤버 함수로만 정의해야 작동함

---

## ⛔ 오버로딩 불가능한 연산자

다음 연산자들은 C++에서 오버로딩할 수 **없음**:

- `::` (범위 지정 연산자)
- `.` (멤버 접근 연산자)
- `.*` (멤버 포인터 연산자)
- `sizeof`, `typeid`, `alignof`
- `? :` (삼항 연산자)

---

## 🎮 게임 서버 예제: 벡터 연산

게임 서버에서 자주 쓰이는 `Vector2D` 클래스 예시:

```cpp
class Vector2D {
    float x, y;
public:
    Vector2D(float x, float y) : x(x), y(y) {}

    Vector2D operator+(const Vector2D& rhs) const {
        return Vector2D(x + rhs.x, y + rhs.y);
    }

    Vector2D operator-(const Vector2D& rhs) const {
        return Vector2D(x - rhs.x, y - rhs.y);
    }

    bool operator==(const Vector2D& rhs) const {
        return (x == rhs.x && y == rhs.y);
    }

    void Print() const {
        std::cout << "(" << x << ", " << y << ")\n";
    }
};
```

```cpp
int main() {
    Vector2D p1(10, 20);
    Vector2D p2(5, 7);
    Vector2D result = p1 + p2;

    result.Print(); // (15, 27)

    if (result == Vector2D(15, 27)) {
        std::cout << "Position matched!\n";
    }
}
```

---

## 🧪 테스트 및 디버깅 팁

- 출력 연산자 `<<` 도 오버로딩 가능: `std::ostream& operator<<(std::ostream&, const Class&)`
- 단항/이항 연산자 모두 오버로딩 가능
- 연산자 함수는 `const` 를 붙이는 습관을 들일 것
- `explicit` 생성자 + `friend` 함수로 불필요한 암시적 변환 방지

---

## 🧠 요약

| 구분 | 설명 |
|------|------|
| 멤버 연산자 함수 | 좌측 피연산자가 해당 클래스일 때 사용 |
| 전역 연산자 함수 | 양쪽 피연산자 모두에 대해 유연하게 적용 가능 |
| 대입 연산자 | 멤버 함수로만 정의 가능 |
| 주의 | 모든 연산자가 오버로딩 가능한 것은 아님 |

---

> ✨ 연산자 오버로딩은 **사용자 정의 자료형을 마치 기본 자료형처럼** 다룰 수 있게 해주며,  
> 게임 개발/서버 개발에서도 좌표 계산, 벡터 연산, 시간 처리 등에서 자주 활용됩니다.
