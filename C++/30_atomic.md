# ⚙️ C++ `std::atomic` 개념 및 사용법 정리

## 1. `std::atomic` 이란?
- **멀티쓰레드 환경에서 안전하게 값을 읽고/쓰기 위해 사용하는 원자적(atomic) 타입**.
- 여러 쓰레드가 **동시에 같은 변수에 접근하더라도** 값이 **깨지지 않도록 보장**.
- 내부적으로 **메모리 배리어(memory barrier)** 와 **CPU의 원자적 명령어**를 사용하여 동작.

---

## 2. 왜 필요한가?
멀티쓰레드 환경에서 일반 변수는 다음과 같은 문제가 발생할 수 있음:

### 문제 상황 (데이터 레이스)
```cpp
#include <iostream>
#include <thread>

int sum = 0; // 힙이나 전역에 존재하는 공유 변수

void add() {
    for (int i = 0; i < 1000000; ++i) {
        sum++; // 여러 쓰레드가 동시에 접근하면 값이 깨짐
    }
}

void sub() {
    for (int i = 0; i < 1000000; ++i) {
        sum--;
    }
}

int main() {
    std::thread t1(add);
    std::thread t2(sub);

    t1.join();
    t2.join();

    std::cout << "결과: " << sum << "\n";
}
```
**문제점:**  
- `sum++` 연산은 내부적으로 **읽기 → 계산 → 쓰기** 3단계를 거침.
- 두 쓰레드가 동시에 접근하면, 값이 꼬여서 **예상치 못한 결과**가 나옴.

---

## 3. `std::atomic` 사용
```cpp
#include <iostream>
#include <thread>
#include <atomic>

std::atomic<int> sum(0); // 원자적 정수 타입

void add() {
    for (int i = 0; i < 1000000; ++i) {
        sum++; // 원자적으로 수행됨
    }
}

void sub() {
    for (int i = 0; i < 1000000; ++i) {
        sum--;
    }
}

int main() {
    std::thread t1(add);
    std::thread t2(sub);

    t1.join();
    t2.join();

    std::cout << "결과: " << sum << "\n"; // 항상 0
}
```
**설명:**  
- `std::atomic<int>`는 `++`, `--`, `+=`, `-=` 등 기본 연산이 **원자적으로** 보장됨.
- CPU 차원에서 락(lock) 없이 동작하는 경우가 많아 빠르지만, 경쟁 상황이 심하면 **성능 저하**가 발생할 수 있음.

---

## 4. 너무 많이 쓰면 안 좋은 이유
- `std::atomic`은 락을 사용하지 않는 대신 **메모리 배리어**를 사용하여 명령어 재정렬을 방지.
- **모든 원자 연산은 CPU 캐시 동기화를 유발**하므로, 쓰레드가 많을수록 성능 저하 가능.
- 특히, **빈번한 atomic 변수 공유**는 **캐시 라인 경합(Cache Line Contention)** 을 초래해 병목이 됨.

**💡 권장 사항**
- 꼭 필요한 공유 데이터에만 사용.
- 가능하면 **쓰레드 간 공유를 줄이고**, 쓰레드 로컬 변수 사용을 우선.
- 여러 변수를 함께 보호하려면 `std::mutex`가 적합.

---

## 5. 추가 예시 — `fetch_add`와 `fetch_sub`
- `fetch_add()` / `fetch_sub()`는 원자적으로 값을 더하고 빼며, **이전 값을 반환**.

```cpp
#include <iostream>
#include <thread>
#include <atomic>

std::atomic<int> sum(0);

void add() {
    for (int i = 0; i < 5; ++i) {
        int old = sum.fetch_add(1); // 기존 값을 반환하고 +1
        std::cout << "ADD: 이전값=" << old << ", 현재값=" << sum.load() << "\n";
    }
}

void sub() {
    for (int i = 0; i < 5; ++i) {
        int old = sum.fetch_sub(1);
        std::cout << "SUB: 이전값=" << old << ", 현재값=" << sum.load() << "\n";
    }
}

int main() {
    std::thread t1(add);
    std::thread t2(sub);

    t1.join();
    t2.join();

    std::cout << "최종 값: " << sum << "\n";
}
```

---

## 6. 결론
- `std::atomic`은 **락 없는 동기화(lock-free synchronization)** 를 제공.
- 데이터 레이스 방지에 유용하지만, **무분별한 사용은 성능 저하**를 야기할 수 있음.
- 게임 서버나 네트워크 프로그램에서 **카운터, 플래그, 상태 값** 관리 등에 자주 사용.
