# 📘 STL Set 정리

`std::set`은 **중복되지 않는 데이터를 자동 정렬 상태로 저장하는 컨테이너**이다.  
내부적으로 **레드-블랙 트리(Red-Black Tree)** 기반으로 구현되어 있으며,  
**자동 정렬**, **O(log n) 탐색**, **중복 방지**가 핵심 특징이다.

---

## 🧠 핵심 특징

| 항목 | 설명 |
|------|------|
| 중복 허용 여부 | ❌ 허용 안 함 (`set`은 중복 불가) |
| 내부 구조 | 레드-블랙 트리 (균형 이진 탐색 트리) |
| 자동 정렬 | 기본은 `<` 기준으로 오름차순 정렬 |
| 시간 복잡도 | 삽입/삭제/탐색 모두 **O(log n)** |
| 순차 접근 | 지원 (정렬된 순서로 begin → end까지 반복자 사용 가능) |
| 키/값 관계 | 키 = 값 (따라서 `set<int>`은 그냥 값의 모음) |

---

## 🧩 주요 메서드 요약

| 메서드 | 설명 |
|--------|------|
| `insert(value)` | 값 삽입 (중복 시 무시됨) |
| `erase(value)` | 해당 값 삭제 |
| `find(value)` | 해당 값의 iterator 반환 (없으면 `end()`) |
| `count(value)` | 존재 여부 반환 (0 또는 1) |
| `clear()` | 모든 요소 삭제 |
| `size()` | 현재 원소 개수 반환 |
| `begin()`, `end()` | 반복자 접근 |
| `lower_bound(value)` | value 이상 첫 원소 iterator |
| `upper_bound(value)` | value 초과 첫 원소 iterator |

---

## 💻 예제: 기본 사용법

```cpp
#include <iostream>
#include <set>

int main() {
    std::set<int> scores;

    scores.insert(100);
    scores.insert(70);
    scores.insert(50);
    scores.insert(70); // 중복 무시

    for (int score : scores) {
        std::cout << score << " ";
    }

    // 출력: 50 70 100 (자동 정렬)
}
```

---

## 🔁 반복자(iterator) 활용

```cpp
std::set<std::string> usernames = {"Alice", "Bob", "Charlie"};

for (auto it = usernames.begin(); it != usernames.end(); ++it) {
    std::cout << *it << "\n";
}
```

---

## 🧪 예제: 게임 서버 활용 예시

### 상황: 유저의 접속 IP를 중복 없이 기록

```cpp
std::set<std::string> connectedIPs;

void OnPlayerConnect(std::string ip) {
    connectedIPs.insert(ip);
}

void PrintIPList() {
    for (const auto& ip : connectedIPs) {
        std::cout << ip << "\n";
    }
}
```

- **중복 접속 방지**, **자동 정렬**, **빠른 탐색**이 가능한 구조
- `std::unordered_set`으로 바꾸면 더 빠르지만, 정렬이 필요하면 `set` 유지

---

## 🔄 사용자 정의 타입 정렬

`set`은 내부에서 `<` 연산자를 사용하여 정렬하기 때문에, 사용자 정의 타입은 **비교 연산자 오버로딩** 또는 **커스텀 비교 함수**가 필요함.

```cpp
struct Player {
    int id;
    std::string name;

    // 정렬 기준 정의 (id 기준)
    bool operator<(const Player& other) const {
        return id < other.id;
    }
};

std::set<Player> playerSet;
```

또는 functor 방식:

```cpp
struct CompareByName {
    bool operator()(const Player& a, const Player& b) const {
        return a.name < b.name;
    }
};

std::set<Player, CompareByName> playerSet;
```

---

## 🧱 `set` vs `vector` vs `map` 비교

| 항목 | `set` | `vector` | `map` |
|------|-------|----------|--------|
| 자동 정렬 | ✅ | ❌ | ✅ (key 기준) |
| 중복 허용 | ❌ | ✅ | ❌ (key 기준) |
| 탐색 속도 | O(log n) | O(n) | O(log n) |
| 정렬 필요 여부 | ❌ (자동 정렬됨) | ⛔ 필요시 수동 정렬 | ❌ |

---

## ⚠️ 주의 사항

- 삽입 시 **중복 값은 무시**되므로, 동일한 값 두 번 넣어도 한 번만 저장됨
- `set`은 키 기반 자료구조이므로 **값 자체를 수정하는 것은 불가능**  
  (값을 수정하려면 삭제 후 다시 삽입해야 함)

---

## 🧪 실무 팁

- **로그인 유저 관리**, **중복된 패킷 필터링**, **이벤트 처리 순서 관리** 등에 사용
- 실시간 게임 서버에선 `unordered_set`과의 성능 차이를 고려할 것 (hash 기반)

---

## ✅ 요약

| 키워드 | 설명 |
|--------|------|
| 정렬된 집합 | `std::set`은 자동 정렬됨 (기본 `<`) |
| 중복 불가 | 같은 값 두 번 삽입하면 무시됨 |
| 레드-블랙 트리 기반 | 삽입/삭제/탐색 모두 O(log n) |
| 반복자 순회 가능 | 정렬된 순서로 순회 가능 |
| 실무 사용 예 | 유저 ID 관리, 로그인 중복 체크 등 |

---

> 📚 STL 내부 구조까지 묻는 질문이 면접에서 나올 수 있음  
> 레드-블랙 트리라는 점과 중복/정렬 정책은 꼭 기억해둘 것!
