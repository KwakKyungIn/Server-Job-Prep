# 📘 STL Map (std::map) — 균형 이진 탐색 트리 기반 연관 컨테이너

`std::map`은 **(key, value) 쌍**을 저장하는 연관 컨테이너이며, **AVL 트리(또는 Red-Black Tree)** 기반의 **정렬된 이진 탐색 트리**로 동작합니다.  
`vector`나 `list`처럼 순차적인 저장이 아니라, **자동 정렬**과 **로그 시간 탐색**, **key 기반 접근**이 핵심입니다.

---

## ✅ 핵심 특징

| 항목 | 설명 |
|------|------|
| 내부 구조 | **균형 이진 탐색 트리 (Red-Black Tree)** |
| 정렬 기준 | 기본적으로 **key 오름차순**, `std::less<Key>` |
| 중복 허용 | ❌ key는 중복 불가 (중복 허용은 `multimap`) |
| 접근 속도 | 탐색/삽입/삭제 모두 **O(log N)** |
| 자동 정렬 | 삽입 시 자동 정렬, `vector`처럼 인덱스 접근 불가 |
| 반복자 | `bidirectional iterator` 지원 |

---

## 🏗 내부 자료구조: 균형 이진 탐색 트리

- `std::map`은 AVL 트리 또는 레드블랙 트리를 기반으로 구현되어 있으며,
- 삽입/삭제 시에도 **트리 균형 유지** 알고리즘이 동작함
- 그 결과, 모든 연산의 시간 복잡도는 `O(log N)` 수준으로 유지됨

---

## 🔧 기본 사용법

```cpp
#include <map>
#include <iostream>

int main() {
    std::map<int, std::string> players;

    // 삽입 (방법 1)
    players[1] = "Knight";

    // 삽입 (방법 2)
    players.insert(std::make_pair(2, "Archer"));

    // 삽입 (방법 3)
    players.emplace(3, "Wizard");

    // 출력
    for (const auto& p : players) {
        std::cout << "ID: " << p.first << ", Name: " << p.second << "\n";
    }
}
```

### 📌 결과 (정렬된 상태)
```
ID: 1, Name: Knight  
ID: 2, Name: Archer  
ID: 3, Name: Wizard  
```

---

## 🔍 주요 메서드 요약

| 함수 | 설명 |
|------|------|
| `operator[]` | key가 존재하지 않으면 default 생성 |
| `.at(key)` | key 존재 시 value 반환, 없으면 예외 발생 |
| `.insert()` | 중복 key 무시 |
| `.emplace()` | in-place 삽입 (더 빠름) |
| `.find(key)` | key 위치 반복자 반환 (없으면 `.end()`) |
| `.erase(key)` | key 삭제 |
| `.clear()` | 전체 삭제 |
| `.count(key)` | key 존재 여부 (0 또는 1 반환) |

---

## 🧪 상세 예시 모음

### ✅ 1. `operator[]` 와 `.at()` 비교

```cpp
std::map<int, std::string> m;
m[5] = "Hunter";     // OK
std::cout << m[5];   // "Hunter"

std::cout << m.at(5); // OK
std::cout << m.at(10); // ❌ 예외 발생! std::out_of_range
```

> `operator[]`는 **없는 key도 삽입 후 default value로 초기화**됨  
> `.at()`는 안전하게 접근할 때 유용 (예외 처리 필요)

---

### ✅ 2. `insert()` vs `emplace()`

```cpp
std::map<int, std::string> m;

m.insert({1, "A"});         // pair 전달
m.emplace(2, "B");          // 인자 직접 전달 → 더 빠름
```

> `emplace()`는 **불필요한 복사/이동을 줄여서** insert보다 성능 유리

---

### ✅ 3. 반복자 사용과 순회

```cpp
for (auto it = m.begin(); it != m.end(); ++it) {
    std::cout << it->first << ": " << it->second << "\n";
}
```

---

### ✅ 4. 조건 검색 (find / count)

```cpp
if (m.count(2)) {
    std::cout << "Key 2 exists!\n";
}

auto it = m.find(2);
if (it != m.end()) {
    std::cout << it->second << "\n";
}
```

---

## 🧠 map vs unordered_map vs vector

| 항목 | `map` | `unordered_map` | `vector` |
|------|-------|------------------|----------|
| 내부 구조 | Red-Black Tree | Hash Table | 배열 |
| 정렬 | ✅ 자동 오름차순 | ❌ 없음 | ❌ 없음 |
| key 탐색 | O(log N) | O(1) 평균 | O(N) |
| 중복 key | ❌ | ❌ | 가능 |
| 메모리 효율 | 보통 | 더 높음 | 가장 높음 |

> 📌 주의: `unordered_map`은 정렬이 필요 없을 때만 사용하는 것이 좋음 (key 순회가 안됨)

---

## 🎮 게임 서버에서의 활용 예시

### 1. 플레이어 ID → 세션 포인터 매핑

```cpp
std::map<int, PlayerSession*> sessionMap;

sessionMap[101] = new PlayerSession();
sessionMap[205] = new PlayerSession();

auto it = sessionMap.find(101);
if (it != sessionMap.end()) {
    it->second->SendMessage("Hello Player!");
}
```

### 2. 오브젝트 타입별 처리 함수 등록

```cpp
std::map<std::string, std::function<void()>> handlerMap;

handlerMap["MONSTER"] = []() { SpawnMonster(); };
handlerMap["NPC"] = []() { TalkToNpc(); };

// 실행
handlerMap["MONSTER"]();
```

---

## 🧹 삭제와 메모리 관리

```cpp
std::map<int, Player*> m;

// 삽입
m[1] = new Player();
m[2] = new Player();

// 삭제 전 delete 필수
for (auto& pair : m)
    delete pair.second;

m.clear();
```

> `map`은 포인터를 저장할 수 있지만, **메모리 해제는 직접 해야 함!**

---

## ✅ 요약 정리

| 항목 | 요약 |
|------|------|
| 핵심 개념 | key-value 쌍을 정렬된 상태로 저장 |
| 내부 구조 | Red-Black Tree 기반 |
| 시간 복잡도 | 탐색/삽입/삭제 O(log N) |
| key 중복 | 허용 ❌ (중복은 multimap) |
| 반복자 | 정렬 순서대로 순회 가능 |
| 실무 활용 | 플레이어 매핑, 오브젝트 핸들러 등록 등 |

---

> `std::map`은 탐색 + 정렬이 동시에 필요한 경우 가장 적합한 자료구조입니다.  
> 게임 서버처럼 **수많은 객체를 ID 또는 키 기반으로 관리**하는 구조에 매우 자주 사용됩니다.
