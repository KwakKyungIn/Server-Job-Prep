# 🔄 스핀락(Spinlock) 정리

> **스핀락(Spinlock)**은 스레드가 락을 얻을 때까지 **잠들지 않고(CPU 양보 없이)** 계속 루프를 돌며 기다리는(lock을 “스핀한다”) 방식의 동기화 도구.  
> 일반적인 `mutex`와는 달리 **컨텍스트 스위치 비용을 피하는 대신 CPU를 소모**하는 특성이 있다.

---

## ❓ 왜 스핀락을 쓰는가?

- `std::mutex` → 락이 안 잡히면 스레드를 **커널로 넘겨 잠재움** → 깨우는 데 컨텍스트 스위치 비용 발생.
- **스핀락** → 락이 풀릴 때까지 **사용자 모드에서 바쁘게 돌며 확인**.  
  - 락이 **매우 짧은 시간만** 잡힐 때는 스핀락이 유리하다.  
  - 락이 **길게 유지되면** CPU 자원 낭비 → 성능 악화.

### 📌 사용 상황
- **멀티코어 환경**에서 잠금 시간이 짧은 경우.  
- 커널/드라이버 레벨에서 자주 사용.  
- 게임 서버 엔진에서 **짧은 데이터 보호 구간**(예: job queue, freelist)에도 종종 활용.

---

## 🛠️ 기본 구현 (C++11)

스핀락은 보통 **atomic 플래그**로 구현한다.

```cpp
#include <atomic>

class SpinLock {
    std::atomic_flag locked = ATOMIC_FLAG_INIT;

public:
    void lock() {
        while (locked.test_and_set(std::memory_order_acquire)) {
            // 다른 스레드가 락을 잡고 있다면 계속 "스핀"
        }
    }

    void unlock() {
        locked.clear(std::memory_order_release);
    }
};
```

### 사용 예시
```cpp
#include <thread>
#include <iostream>

SpinLock spin;
int counter = 0;

void worker() {
    for (int i = 0; i < 100000; i++) {
        spin.lock();
        counter++;
        spin.unlock();
    }
}

int main() {
    std::thread t1(worker), t2(worker);
    t1.join(); t2.join();
    std::cout << "Final counter = " << counter << "\n";
}
```

- 두 스레드가 동시에 `counter`를 증가시키지만, 스핀락으로 보호되어 **데이터 경합 없음**.

---

## 📉 단점과 주의점

1. **CPU 낭비**  
   - 잠금 대기 동안 CPU를 100% 사용.  
   - 장시간 락 구간에서는 `std::mutex`가 더 효율적.

2. **단일 코어 환경**  
   - 다른 스레드가 락을 잡고 있어도, **현재 스레드가 계속 바쁘게 돌아서 CPU를 안 내줌** → 다른 스레드가 실행 기회조차 못 얻어 **영원히 풀리지 않는 경우 발생**.

3. **공정성(Fairness) 보장 없음**  
   - 스핀락은 단순히 **먼저 도는 놈이 얻는다**라서, 특정 스레드가 락을 계속 놓치면 기아(Starvation) 가능.

4. **캐시 트래픽 증가**  
   - atomic 연산으로 인한 **캐시 라인 invalidation**이 계속 발생 → 성능 악화 요인.

---

## 🧩 개선 기법

- **Pause 명령어 사용 (x86: `_mm_pause()`)**  
  - 바쁜 루프 내에서 잠깐 대기 → 캐시 충돌 완화, 전력 소모 감소.  

- **Backoff 전략**  
  - 실패할 때마다 기다리는 시간을 **점진적으로 늘려서** 경쟁 완화.  

- **Hybrid Lock (Adaptive Mutex)**  
  - 먼저 잠깐 스핀 → 실패하면 일반 뮤텍스로 폴백.  
  - POSIX pthread, Windows SRWLock 등에 이런 최적화가 포함됨.

---

## ✅ 요약

- 스핀락은 **짧은 임계 구역에서 효율적**, 하지만 긴 대기 상황에서는 **CPU를 낭비**.  
- 구현은 간단히 `std::atomic_flag`로 가능.  
- 멀티코어, 짧은 락 유지 시간, 커널/서버 성능 최적화 영역에서 사용 가치.  
- 단일 코어나 장기 잠금에는 쓰지 말 것.  

---
