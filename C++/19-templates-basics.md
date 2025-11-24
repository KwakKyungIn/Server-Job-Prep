# 🧩 C++ 템플릿 기초 정리

> 템플릿은 **컴파일 타임에 코드 생성을 유도**하는 C++의 제네릭 프로그래밍 기능이다.  
> 타입, 값, 동작을 추상화하여 **코드 중복 제거**, **유연성 향상**, **성능 유지**를 동시에 만족시킨다.

---

## ❓ 템플릿이란?

- **템플릿**은 함수나 클래스 정의에서 **타입을 일반화**할 수 있는 문법이다.
- 컴파일 시점에 **타입에 맞는 코드가 자동 생성**됨.
- 대표적으로 STL의 `vector<T>`, `map<K,V>` 등이 템플릿 기반이다.

---

## 🛠️ 함수 템플릿

### 📌 기본 문법

```cpp
template <typename T>
T Add(T a, T b) {
    return a + b;
}
```

### 📎 사용 예
```cpp
std::cout << Add<int>(3, 5);        // 명시적 타입 지정
std::cout << Add(3.0, 2.5);         // 타입 추론 (double)
```

> `typename` 대신 `class`도 가능. 둘은 기능상 동일.

---

## 🧱 클래스 템플릿

### 📌 기본 문법

```cpp
template <typename T>
class Box {
private:
    T value;
public:
    void Set(T val) { value = val; }
    T Get() { return value; }
};
```

### 📎 사용 예
```cpp
Box<int> intBox;
intBox.Set(42);
std::cout << intBox.Get();
```

> 컴파일 시점에 타입별 클래스를 생성함.

---

## 🧠 템플릿 특수화

특정 타입에 대해 **다르게 구현**하고 싶을 때 사용한다.

### 🎯 함수 템플릿 특수화 예시

```cpp
template <typename T>
void PrintType(T val) {
    std::cout << "일반 타입: " << val << "\n";
}

template <>
void PrintType<bool>(bool val) {
    std::cout << "불리언 타입: " << (val ? "true" : "false") << "\n";
}
```

```cpp
PrintType(123);    // 일반 타입
PrintType(true);   // 특수화된 버전 실행
```

### 🎯 클래스 템플릿 특수화 예시

```cpp
template <typename T>
class Printer {
public:
    void Print(T val) {
        std::cout << "값: " << val << "\n";
    }
};

template <>
class Printer<char> {
public:
    void Print(char val) {
        std::cout << "문자: " << val << "\n";
    }
};
```

---

## 🔢 비타입 템플릿 파라미터

> `template<typename T, int SIZE>` 형태로, 타입 외에 **상수 값**도 템플릿 인자로 전달 가능하다.

### 📌 예시: 배열 템플릿 클래스
```cpp
template <typename T, int SIZE>
class FixedArray {
private:
    T data[SIZE];
public:
    int GetSize() const { return SIZE; }
};
```

```cpp
FixedArray<int, 10> arr;
std::cout << arr.GetSize();   // 10
```

### ⚠️ 주의
- 값은 반드시 **컴파일 타임 상수**여야 함 (literal or constexpr).
- 타입은 `int`, `char`, 포인터 등 단순 값만 가능 (`float` 같은 실수형 불가).

---

## 🔍 템플릿 인스턴스화 동작

| 시점 | 동작 |
|------|------|
| 정의 시 | 템플릿 원형만 존재 |
| 사용 시 | **인스턴스화** (타입에 따라 코드 생성) |
| 장점 | 코드 중복 제거, 타입에 따른 재사용 |
| 단점 | 에러가 복잡하게 출력될 수 있음 (컴파일 시 생성되므로) |

---

## 🧠 마무리 요약

| 개념 | 설명 |
|------|------|
| 함수 템플릿 | 다양한 타입의 함수를 제네릭하게 작성 |
| 클래스 템플릿 | 다양한 타입의 클래스를 생성 |
| 특수화 | 특정 타입에 대한 예외적 정의 |
| 비타입 템플릿 | 상수 값(정수 등)을 템플릿 인자로 전달 |
| STL | 거의 모든 컨테이너가 템플릿 기반 (`vector<T>`, `map<K,V>` 등) |

---

## 💡 면접 포인트

- 함수 템플릿과 클래스 템플릿의 차이점?
- 템플릿 특수화를 왜 사용하는가?
- 비타입(non-type) 템플릿 파라미터의 제약 사항은?
- STL에서 템플릿이 왜 중요한가?

---

> 🎯 C++의 템플릿은 **컴파일 타임 다형성**을 가능케 하며, **성능과 유연성**을 모두 확보할 수 있는 고급 기능이다.  
> 게임 서버 개발에서도 **데이터 컨테이너, 알고리즘 커스터마이징, 유틸리티 구현** 등에 광범위하게 활용된다.
