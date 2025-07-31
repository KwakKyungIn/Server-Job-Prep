# 📘 STL Deque: 양방향 동적 배열 (std::deque)

C++ STL의 `std::deque`(Double-Ended Queue)는 **양쪽 끝에서 삽입/삭제가 가능한 동적 배열**입니다.  
`vector`보다 **앞쪽 삽입/삭제에 유리**하고, `list`보다 **임의 접근이 빠른 구조**를 가지고 있어  
**vector와 list의 중간적 특성을 갖는 컨테이너**로 자주 쓰입니다.

---

## ✅ 기본 개념

| 항목 | 설명 |
|------|------|
| 구조 | **블록 기반의 동적 배열** |
| 메모리 | 여러 블록으로 나뉘어 관리됨 (비연속적 구조) |
| 삽입/삭제 | **앞/뒤 모두 빠름 (O(1))** |
| 접근 | **임의 접근 가능 (O(1))** |
| 반복자 | `random access iterator` (vector와 동일한 등급) |

---

## 🏗 내부 구조

- `deque`는 내부적으로 **고정 크기 블록(예: 512B씩)**을 사용하여 메모리를 할당하고
- **앞뒤로 블록을 추가하거나 제거**할 수 있게 구성됨
- 따라서 vector처럼 **중간 삽입은 느리지만**, 앞뒤 삽입/삭제는 빠름

```
[   ][   ][ a b c d e ][   ][   ]
             ↑
          중심 블록
```

---

## ⚙ 주요 특징

| 항목 | 설명 |
|------|------|
| `push_front()` / `pop_front()` | ✅ 매우 빠름 (vector는 느림) |
| `push_back()` / `pop_back()` | ✅ vector만큼 빠름 |
| `[]` 연산자 지원 | ✅ 임의 접근 가능 |
| 중간 삽입/삭제 | ❌ 느림 (`list`보다 느릴 수 있음) |

---

## 📌 Vector vs Deque vs List 비교

| 특성 | `vector` | `deque` | `list` |
|------|----------|---------|--------|
| 앞쪽 삽입/삭제 | ❌ 느림 | ✅ 빠름 | ✅ 빠름 |
| 뒤쪽 삽입/삭제 | ✅ 빠름 | ✅ 빠름 | ✅ 빠름 |
| 임의 접근 | ✅ 빠름 | ✅ 빠름 | ❌ 느림 |
| 중간 삽입/삭제 | ❌ 느림 | ❌ 느림 | ✅ 빠름 |
| 메모리 구조 | 연속 | 블록 기반 비연속 | 노드 기반 비연속 |
| 반복자 타입 | Random Access | Random Access | Bidirectional |

---

## 🔁 주요 메서드 예시

```cpp
#include <deque>
#include <iostream>

int main() {
    std::deque<int> dq;

    dq.push_back(10);     // 뒤에 삽입
    dq.push_front(5);     // 앞에 삽입
    dq.push_back(20);     // 다시 뒤에 삽입

    // 출력: 5 10 20
    for (int n : dq)
        std::cout << n << " ";

    std::cout << "\n";

    dq.pop_front();       // 앞에서 제거
    dq.pop_back();        // 뒤에서 제거

    // 출력: 10
    std::cout << dq[0] << "\n"; // 임의 접근도 가능!
}
```

---

## 🧠 STL Deque가 적합한 상황

- **양방향 삽입/삭제가 모두 필요한 큐 구현**
- **자료가 양 끝에서 자주 추가/삭제되는 상황**
- **임의 접근도 필요한 경우**
- 게임 서버에서 예를 들면:
  - **메시지 버퍼 큐**
  - **실시간 입력 처리 큐**
  - **비동기 패킷 처리용 버퍼**

---

## ⚠️ 주의할 점

- `deque`는 `vector`보다 캐시 효율이 낮다 (비연속 메모리)
- 중간 삽입/삭제 성능은 `list`에 비해 떨어진다
- 반복자 무효화(invalidation) 발생 가능성 있음 → 특히 삽입/삭제 후 주의

---

## 🎮 게임 서버 예시: 명령 큐

```cpp
#include <deque>
#include <string>

struct Command {
    int playerId;
    std::string action;
};

std::deque<Command> commandQueue;

// 클라이언트 입력 수신 시
commandQueue.push_back({101, "move_left"});
commandQueue.push_back({102, "attack"});

// 처리 루프
while (!commandQueue.empty()) {
    Command cmd = commandQueue.front();
    commandQueue.pop_front();

    ProcessCommand(cmd);
}
```

> 실시간 명령 큐에서 앞에서 꺼내고, 뒤에서 넣는 구조에 적합

---

## ✅ 요약 정리

| 특성 | 설명 |
|------|------|
| 삽입/삭제 | 앞뒤 모두 빠름 |
| 접근 성능 | 임의 접근 가능 (O(1)) |
| 메모리 구조 | 고정 블록 기반 비연속 |
| 적합한 사용처 | **큐/버퍼** 구조, 패킷 처리 등 |
| 주의사항 | 캐시 효율 낮음, 중간 삽입은 비효율적 |

---

> STL `deque`는 vector와 list의 중간 지점을 잘 채우는 컨테이너입니다.  
> 상황에 따라 vector, deque, list 중에서 가장 적합한 컨테이너를 고르는 판단력이 중요합니다.
