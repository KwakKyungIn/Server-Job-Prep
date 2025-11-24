# 📦 STL Vector 완전 정리

> `std::vector`는 C++ STL(Standard Template Library)에서 가장 널리 사용되는 **동적 배열 컨테이너**이다.  
> **배열처럼 사용 가능**하면서도, **크기 자동 관리**, **삽입/삭제 유연성**, **템플릿 기반의 타입 일반화** 등의 장점을 가진다.

---

## 📚 STL이란?

- **STL (Standard Template Library)**: C++ 표준 라이브러리의 일부로, **자료구조(Container)** 와 **알고리즘(Algorithm)** 을 **템플릿 기반**으로 제공
- STL의 핵심 구성요소:
  - **컨테이너(Container)**: 데이터를 저장하는 자료구조 (`vector`, `map`, `set`, ...)
  - **반복자(Iterator)**: 컨테이너 순회 도구 (포인터처럼 사용)
  - **알고리즘(Algorithm)**: 정렬, 탐색, 복사 등 (`std::sort`, `std::find`, ...)

---

## 📦 `std::vector`란?

- 동적 배열(Dynamic Array) 자료구조
- 배열과 달리 **크기가 자동 조절됨**
- **템플릿 기반**으로 어떤 타입도 저장 가능 (`std::vector<int>`, `std::vector<std::string>` 등)

---

## ⚙️ vector의 동작 원리

### 🔢 기본 용어

| 용어 | 설명 |
|------|------|
| `size` | 현재 저장된 원소의 개수 |
| `capacity` | 현재 할당된 메모리 공간 (최대 저장 가능 수) |

### 📈 메모리 증설 과정

1. **초기 삽입 시**
   - 여유 공간(capacity)이 없다면, 일정 크기만큼 미리 메모리 할당
2. **capacity 초과 시**
   - 메모리를 **더 큰 블록으로 새로 할당**
   - 기존 데이터를 **새 메모리로 복사**
   - **기존 메모리는 해제**

### ❓ 여유분 설정 기준

- 대부분의 STL 구현은 `capacity * 1.5` 또는 `2배`로 증가시킴 (성능 vs 메모리 균형)
- 이유: **재할당/복사 비용 최소화**를 위한 전략

```cpp
std::vector<int> v;
v.push_back(1);  // capacity가 부족하면 메모리 재할당 발생
```

---

## 🧭 vector 사용 예시

```cpp
std::vector<int> v = {1, 2, 3};
v.push_back(4);          // 뒤에 삽입
v.pop_back();            // 마지막 원소 제거
v[1] = 10;               // 인덱스로 접근 가능
std::cout << v.size();   // 현재 크기 확인
```

---

## 🔁 반복자 (Iterator)

> `Iterator`는 vector 내부 데이터를 가리키는 **포인터 유사 객체**로, 컨테이너 전체를 순회하는 데 사용된다.

### 📌 기본 문법

```cpp
std::vector<int> v = {1, 2, 3, 4};

for (std::vector<int>::iterator it = v.begin(); it != v.end(); ++it) {
    std::cout << *it << " ";
}
```

- `v.begin()` : 첫 원소를 가리킴
- `v.end()` : 마지막 원소 **다음 위치**를 가리킴
- `*it` : 현재 원소
- `++it` : 다음 원소로 이동

### ✅ 범위 기반 for문 (C++11 이상)

```cpp
for (int val : v) {
    std::cout << val << " ";
}
```

---

## 🧠 vector vs 일반 배열

| 항목 | `std::vector` | 일반 배열 (`int arr[]`) |
|------|---------------|--------------------------|
| 크기 변경 | 가능 | 불가능 |
| 크기 확인 | `v.size()` | 불가능 (`sizeof` 활용 불완전) |
| 복사 | 깊은 복사 | 얕은 복사 (포인터 복사) |
| 메모리 관리 | 자동 | 수동 |
| 타입 일반화 | 가능 (`template`) | 고정 |

---

## 💡 vector의 주의점

- `push_back()` 반복 시 **재할당 비용 발생 가능**
  - 빈번한 삽입이 예상된다면 `reserve()`로 capacity 미리 확보
- `[]` 연산자는 **범위 검사를 하지 않음**
  - 안전한 접근은 `at()` 사용 권장 (`예외 처리 있음`)
- iterator는 **vector가 재할당될 경우 무효화됨**

---

## 🎮 게임 서버 예시

- **접속 중인 유저 목록**을 vector로 관리

```cpp
std::vector<Player*> playerList;

void BroadcastMessage(const std::string& msg) {
    for (Player* p : playerList) {
        p->SendMessage(msg);
    }
}
```

- 주의: 유저가 접속 종료되면 vector에서 삭제 → `erase` 이후 반복자 무효화 주의

---

## ✅ 요약

| 항목 | 설명 |
|------|------|
| STL | 템플릿 기반 자료구조 + 알고리즘 집합 |
| vector | 크기 조절 가능한 동적 배열 컨테이너 |
| size / capacity | 실제 원소 수 vs 메모리 할당량 |
| 재할당 전략 | 공간 부족 시 더 큰 메모리 블록으로 복사 |
| 반복자 | 컨테이너 내 원소를 가리키는 포인터 유사 객체 |
| 실전 사용 | 리스트 관리, 큐 역할 등에서 자주 사용됨 |

---

## 🔍 면접 포인트

- `vector`가 메모리를 어떻게 관리하는가?
- `capacity`와 `size`의 차이는?
- `erase()` 또는 `insert()` 후 반복자는 어떻게 되는가?
- `vector`가 재할당 시 어떤 일이 발생하는가?

---

> 🎯 `vector`는 C++에서 가장 실용적이고 강력한 컨테이너이다.  
> 메모리 관리 전략, 반복자 처리, 그리고 동작 방식까지 모두 익혀두면 STL 사용에 자신감이 생긴다.
