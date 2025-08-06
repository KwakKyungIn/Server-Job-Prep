# 🌱 STL multiset & multimap 정리

`std::multiset`과 `std::multimap`은 각각 `set`, `map`의 **변형 컨테이너**로  
**중복된 값을 허용**한다는 차이만 있을 뿐, 기본적인 자료구조와 동작 방식은 동일하다.

---

## 📌 공통 핵심 특징

| 항목 | multiset | multimap |
|------|----------|----------|
| 중복 허용 | ✅ 허용함 | ✅ key 중복 허용 |
| 내부 구조 | 레드-블랙 트리 기반 | 레드-블랙 트리 기반 |
| 정렬 여부 | 자동 정렬 (기본 `<`) | key 기준으로 자동 정렬 |
| 시간 복잡도 | O(log n) (삽입, 삭제, 탐색) | O(log n) |
| 순차 접근 | 가능 (`begin()` ~ `end()`) | 가능 (key 순) |

---

## 🔹 `std::multiset`

### ✅ 정의 및 기본 사용법

```cpp
#include <iostream>
#include <set>

int main() {
    std::multiset<int> ms;
    ms.insert(10);
    ms.insert(10); // 중복 허용
    ms.insert(5);

    for (int val : ms)
        std::cout << val << " ";
    // 출력: 5 10 10
}
```

---

### 🧩 주요 메서드

| 함수 | 설명 |
|------|------|
| `insert(val)` | 값을 삽입 (중복 허용) |
| `erase(val)` | 해당 값 모두 삭제 |
| `find(val)` | 첫 번째 등장 위치 반환 |
| `count(val)` | 해당 값의 개수 반환 |
| `equal_range(val)` | 해당 값의 `[시작, 끝)` 반복자 쌍 반환 |

---

### 🔍 예제: equal_range 사용

```cpp
std::multiset<int> scores = {100, 90, 90, 70};

auto range = scores.equal_range(90);
for (auto it = range.first; it != range.second; ++it) {
    std::cout << *it << " ";
}
// 출력: 90 90
```

---

## 🔹 `std::multimap`

### ✅ 정의 및 기본 사용법

```cpp
#include <iostream>
#include <map>

int main() {
    std::multimap<std::string, int> mm;

    mm.insert({"Alice", 100});
    mm.insert({"Bob", 80});
    mm.insert({"Alice", 95}); // key 중복 허용

    for (auto& [name, score] : mm)
        std::cout << name << ": " << score << "\n";
}
/*
출력:
Alice: 95
Alice: 100
Bob: 80
(정렬 기준에 따라 Alice가 먼저 나옴)
*/
```

---

### 🧩 주요 메서드

| 함수 | 설명 |
|------|------|
| `insert({key, value})` | 중복 key-value 쌍 삽입 |
| `find(key)` | 첫 번째 key의 반복자 반환 |
| `equal_range(key)` | 해당 key의 `[시작, 끝)` 반복자 쌍 반환 |
| `count(key)` | 해당 key의 개수 반환 |
| `erase(key)` | 해당 key 전부 삭제 |

---

### 🔍 예제: equal_range

```cpp
auto range = mm.equal_range("Alice");
for (auto it = range.first; it != range.second; ++it)
    std::cout << it->first << ": " << it->second << "\n";
```

---

## 🧪 게임 서버 실전 예제

### ✔ multiset 활용: 몬스터 드랍 테이블 (중복 확률 존재)

```cpp
std::multiset<std::string> dropTable = {
    "Potion", "Potion", "Gold", "Gold", "Gold", "RareItem"
};

// 특정 드랍 횟수 체크
std::cout << "Gold count: " << dropTable.count("Gold") << "\n";
```

---

### ✔ multimap 활용: 퀘스트 → 다중 보상

```cpp
std::multimap<int, std::string> questRewards;

questRewards.insert({101, "Gold"});
questRewards.insert({101, "XP"});
questRewards.insert({101, "Item"});

// 퀘스트 101의 보상 모두 출력
auto range = questRewards.equal_range(101);
for (auto it = range.first; it != range.second; ++it) {
    std::cout << it->second << "\n";
}
```

---

## ❗ `set/map` vs `multiset/multimap` 비교 요약

| 항목 | set/map | multiset/multimap |
|------|---------|-------------------|
| 중복 허용 | ❌ 불가 | ✅ 가능 |
| `insert()` | 중복 무시 | 모두 삽입 |
| 탐색 방식 | 한 개 반환 | 여러 개 반환 (equal_range) |
| 용도 | 유일한 key/value 보장 필요 | 중복된 데이터 허용 시 |

---

## 🧠 면접 대비 포인트

- STL에서 중복 허용이 필요한 경우 `multi` 계열 사용
- **multimap은 `unordered_multimap`과도 비교 가능** (성능 위주 질문 가능)
- `equal_range()`는 **반복자 범위 반환**이라는 점 명확히 알아두기

---

## ✅ 요약

| 컨테이너 | 핵심 포인트 |
|----------|--------------|
| `std::multiset` | 중복 허용 정렬된 집합 |
| `std::multimap` | 중복 key 허용 정렬된 매핑 |
| 내부 구현 | 레드-블랙 트리 |
| 시간 복잡도 | O(log n) |
| 주요 메서드 | `insert()`, `find()`, `count()`, `equal_range()` |

---

> 📚 참고: 실무에서는 중복 key를 처리할 때 multimap보다 `unordered_multimap`을 사용할 수도 있음 (속도 우선)
