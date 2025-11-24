# 🔔 `std::condition_variable` (조건 변수)

멀티스레드 환경에서 **스레드 간의 효율적인 동기화**를 도와주는 도구.  
특히 **Producer-Consumer 문제** 해결에 자주 쓰인다.  

---

## 📌 Condition Variable의 개념

- C++11에서 도입된 **표준 동기화 도구**.
- 스레드가 어떤 조건이 만족될 때까지 **효율적으로 대기**할 수 있게 한다.
- **Busy-waiting(계속 루프 돌면서 확인)**을 피하고, 운영체제가 스레드를 **sleep 상태로 전환**했다가 조건이 충족되면 깨운다.

### 주요 멤버 함수
- `wait(lock, predicate)`  
  - `predicate`가 `true`가 될 때까지 현재 스레드를 대기 상태로 둔다.
  - 자동으로 `lock.unlock()` → 대기 → 깨어날 때 `lock.lock()` 다시 수행.
- `notify_one()`  
  - 대기 중인 **하나의 스레드**를 깨움.
- `notify_all()`  
  - 대기 중인 **모든 스레드**를 깨움.

---

## 🧩 예제 코드

### Producer-Consumer with Condition Variable
```cpp
#include <iostream>
#include <thread>
#include <mutex>
#include <queue>
#include <condition_variable>

using namespace std;

mutex m;
condition_variable cv;
queue<int> q;
bool ready = false;

void Producer()
{
    while (true)
    {
        {
            unique_lock<mutex> lock(m);
            q.push(100);
            ready = true;
            cout << "Produced: 100" << endl;
        }
        cv.notify_one(); // 하나의 소비자 스레드를 깨움
        this_thread::sleep_for(chrono::milliseconds(1000));
    }
}

void Consumer()
{
    while (true)
    {
        unique_lock<mutex> lock(m);

        // 조건이 만족할 때까지 대기
        cv.wait(lock, [] { return ready && !q.empty(); });

        int data = q.front();
        q.pop();
        cout << "Consumed: " << data << endl;

        if (q.empty())
            ready = false;
    }
}

int main()
{
    thread t1(Producer);
    thread t2(Consumer);

    t1.join();
    t2.join();
}
```

---

## ⚖️ Event vs Condition Variable 비교

| 구분 | Windows Event | Condition Variable |
|------|---------------|---------------------|
| **소속** | OS 커널 오브젝트 (Windows 전용) | C++ 표준 라이브러리 |
| **상태 관리** | 신호(Signal) / 비신호(Non-Signal) | 임의의 조건(predictate) |
| **대기 방식** | `WaitForSingleObject` (커널 전환 비용 큼) | `wait(lock, predicate)` (유저 모드+커널 모드 효율적) |
| **리셋 방식** | 자동/수동 리셋 구분 필요 | 조건 변수 자체는 조건을 저장하지 않음 (조건은 프로그래머가 관리) |
| **사용 범위** | Windows 전용 | 멀티플랫폼 (리눅스/맥/윈도우 모두 가능) |
| **비용** | 무거움 (커널 오브젝트 호출) | 상대적으로 가벼움 |
| **적합한 경우** | Windows API 기반 서버/클라이언트 | 표준 C++ 기반 크로스플랫폼 프로그램 |

---

## 🎮 게임 서버에서의 활용

- **Event**
  - Windows 전용 IOCP 서버나 WinAPI 기반 코드에서 종종 사용.
  - 직관적이지만, **커널 오브젝트 전환 비용**이 크다.
- **Condition Variable**
  - 크로스플랫폼 서버나 최신 C++ 코드에서 더 선호됨.
  - 게임 서버에서 **Job Queue**, **패킷 처리 대기열** 등에서 활용하기 좋음.

---

## ✅ 결론

- `condition_variable`은 **스레드를 효율적으로 sleep/wakeup** 시켜주는 도구.
- **mutex + condition_variable** 조합으로 동기화 구현.  
- Event보다 가볍고, 멀티플랫폼 호환성도 좋아서 C++ 서버 프로그래밍에서는 사실상 표준.

---
