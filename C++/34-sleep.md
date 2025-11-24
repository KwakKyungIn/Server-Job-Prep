# 😴 Sleep 정리 (Thread Sleep)

> **Sleep**은 스레드를 일정 시간 동안 **CPU에서 강제로 쉬게(일시 중단)** 만드는 함수.  
> 스레드는 그 시간 동안 실행되지 않고, CPU는 다른 작업을 처리할 수 있다.

---

## 🛠️ Sleep 기본 동작

- C++에서 쓰레드를 멈출 때 보통 `std::this_thread::sleep_for()` 또는 `std::this_thread::sleep_until()` 사용.
- 운영체제가 스레드를 **Ready Queue → Waiting Queue**로 옮김.
- 설정한 시간이 지나면 다시 Ready Queue에 넣고 스케줄링 대상이 됨.

```cpp
#include <iostream>
#include <thread>
#include <chrono>

void worker() {
    std::cout << "작업 시작\n";
    std::this_thread::sleep_for(std::chrono::seconds(2)); // 2초간 대기
    std::cout << "작업 재개\n";
}

int main() {
    std::thread t(worker);
    t.join();
}
```

### 출력
```
작업 시작
(2초 대기)
작업 재개
```

---

## 📌 Sleep의 특징

1. **CPU 점유 없음**  
   - 스레드가 sleep 중이면 CPU를 사용하지 않는다.  
   - 운영체제는 다른 스레드에 CPU를 배정할 수 있음.

2. **정확도 제한**  
   - 실제로는 설정한 시간보다 조금 더 걸릴 수 있음 (OS 스케줄러 지연).  
   - 실시간성이 중요한 환경에서는 적합하지 않을 수 있음.

3. **스레드 협력**  
   - busy-wait(계속 도는 것) 대신 sleep을 활용하면 CPU 자원 절약.  

---

## 🧩 스핀락과 비교

| 구분              | **Sleep**                          | **Spinlock**                          |
|-------------------|------------------------------------|---------------------------------------|
| CPU 사용 여부     | Sleep 중 CPU 사용 안함              | 계속 루프 돌며 CPU 100% 사용           |
| 응답 속도         | 깨어나기까지 OS 스케줄링 지연 가능  | 락 풀리면 즉시 실행 가능 (빠름)        |
| 사용 적합한 상황  | 오래 기다려야 하는 경우            | 매우 짧게 기다리는 경우                |
| 멀티코어 필요성   | 상관 없음                           | 멀티코어 환경에서만 의미 있음           |

---

## 🚦 스핀락 + Sleep 혼합 (Backoff 전략)

때로는 스핀락만 쓰지 않고 **잠깐 스핀 → 실패하면 Sleep**으로 넘어가는 방식도 쓴다.  
즉, 잠깐은 CPU를 써서 빠르게 락을 얻으려 시도하고, 계속 실패하면 CPU 낭비를 막기 위해 잠시 쉰다.

```cpp
#include <atomic>
#include <thread>
#include <chrono>

class HybridLock {
    std::atomic_flag locked = ATOMIC_FLAG_INIT;

public:
    void lock() {
        int spin_count = 0;
        while (locked.test_and_set(std::memory_order_acquire)) {
            if (++spin_count > 1000) {
                // 너무 오래 돌면 잠시 휴식
                std::this_thread::sleep_for(std::chrono::microseconds(10));
                spin_count = 0;
            }
        }
    }

    void unlock() {
        locked.clear(std::memory_order_release);
    }
};
```

---
---
# ⚖️ `yield()` vs `sleep()` 비교 정리

멀티스레드 프로그래밍에서 스레드의 실행 흐름을 제어할 때 자주 등장하는 두 함수.  
둘 다 **현재 스레드를 잠시 멈추고 CPU를 다른 스레드에게 양보**한다는 공통점이 있지만,  
**양보의 방식과 목적이 다르다.**

---

## 🧩 `std::this_thread::yield()`

### 동작 원리
- 현재 스레드가 CPU를 쓰는 것을 잠시 양보하고, **같은 우선순위를 가진 다른 스레드**가 실행될 수 있게 한다.
- 하지만 **즉시 다시 스케줄링될 수도 있음** (보장 없음).
- OS 스케줄러 정책에 따라 다름.

### 특징
- **아주 짧은 대기** 상황에서 사용.
- CPU를 계속 쓰되 다른 스레드도 기회를 주고 싶을 때.
- "나 좀 쉬었다가 다시 불러줘도 돼" 수준.

### 예제
```cpp
#include <iostream>
#include <thread>
#include <chrono>

void worker(const char* name) {
    for (int i = 0; i < 5; i++) {
        std::cout << name << " 작업 중...\n";
        std::this_thread::yield(); // 다른 스레드에 양보
    }
}

int main() {
    std::thread t1(worker, "스레드1");
    std::thread t2(worker, "스레드2");
    t1.join();
    t2.join();
}
```

---

## 🛌 `std::this_thread::sleep_for()` / `sleep_until()`

### 동작 원리
- 스레드를 **정해진 시간 동안 완전히 멈춤**.
- 그 시간 동안은 CPU를 절대 사용하지 않음.
- 시간이 끝나면 다시 Ready Queue에 올라가 스케줄링을 기다림.

### 특징
- **긴 대기** 상황에서 사용 (ms 단위 이상).
- CPU 자원 절약에 초점.
- "나는 확실히 OO초 동안 쉴게"라는 명령.

### 예제
```cpp
#include <iostream>
#include <thread>
#include <chrono>

void worker(const char* name) {
    std::cout << name << " : 시작\n";
    std::this_thread::sleep_for(std::chrono::seconds(2)); // 2초 휴식
    std::cout << name << " : 재개\n";
}

int main() {
    std::thread t(worker, "스레드1");
    t.join();
}
```

---

## 🔍 Yield vs Sleep 비교

| 구분             | `yield()`                                       | `sleep_for()` / `sleep_until()`                |
|------------------|------------------------------------------------|------------------------------------------------|
| CPU 점유         | 즉시 양보, 바로 다시 잡을 수도 있음             | 아예 CPU에서 빠짐, 대기 시간 끝날 때까지 사용 불가 |
| 대기 시간        | **0초 (즉시)**                                  | **지정한 시간(ms, s 등)**                     |
| 목적             | 다른 스레드에게 실행 기회 주기                  | CPU 자원 절약 + 명확한 시간 제어               |
| 활용 상황        | 짧은 대기 / 락 경합에서 잠깐 양보               | 긴 대기 / 이벤트 기다림                        |
| 예시             | busy-wait 최적화, 게임 루프 안에서 잠깐 양보     | 패킷 대기, 일정 시간마다 실행하는 작업          |

---

## 🎮 게임 서버에서의 활용

- **yield**  
  - JobQueue, SpinLock 등에서 잠깐 CPU를 양보할 때 활용.  
  - 예: 짧게 다른 스레드가 끝내주길 기다리는 상황.  

- **sleep**  
  - DB 쿼리 응답, 클라이언트 패킷 대기처럼 **실제 대기 시간이 긴 작업**에서 활용.  
  - 예: 일정 주기마다 리소스를 갱신하는 타이머 스레드.  

---

## ✅ 결론

- `yield()` → **짧은 양보 (즉시 재실행될 수도 있음)**  
- `sleep()` → **명시적인 대기 (CPU 자원 절약)**  
- 상황에 따라 적절히 선택해야 하며, 실무에서는 **spin → yield → sleep 백오프 전략**을 조합하기도 한다.



## ✅ 요약

- `sleep`은 스레드를 멈추고 CPU를 다른 작업에 할당 → **자원 효율적**.  
- 스핀락은 CPU를 계속 쓰면서 기다림 → **짧은 락 구간에서만 효율적**.  
- 실전에서는 **혼합 전략**(spin + sleep)이 종종 쓰인다.  
- 게임 서버에서도 **짧은 대기 → 스핀**, **긴 대기 → sleep** 같은 패턴 자주 활용됨.



