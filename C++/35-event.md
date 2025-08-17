# 🔔 Windows Event를 이용한 스레드 동기화

이번에는 **Windows Event 객체**를 이용한 **Producer-Consumer(생산자-소비자) 문제 해결** 예제다.  
`mutex`와 `event`를 조합해서 안전하게 데이터를 주고받는 방법을 다룬다.

---

## 📌 Event란?

- **커널 오브젝트(Kernel Object)** 중 하나.
- 상태를 **신호(Signal)** 또는 **비신호(Non-Signal)** 로 가지며, 스레드가 이 이벤트를 기다릴 수 있다.
- 스레드 동기화를 위해 사용되며, `SetEvent`, `ResetEvent`, `WaitForSingleObject` 같은 API로 제어한다.

### 이벤트 종류
- **수동 리셋(Manual Reset)**  
  - 신호 상태가 되면 모든 대기 중인 스레드가 동시에 깨어남.  
  - 이후 `ResetEvent`를 호출해 수동으로 비신호 상태로 돌려야 한다.
- **자동 리셋(Auto Reset)**  
  - 신호 상태가 되면 **하나의 스레드만 깨어나고**, 자동으로 비신호 상태로 바뀜.  
  - 일반적으로 Producer-Consumer 문제에 많이 사용.

---

## 🧩 예제 코드 설명

### 코드
```cpp
#include "pch.h"
#include <iostream>
#include "CorePch.h"
#include <atomic>
#include <thread>
#include <mutex>
#include <queue>
#include <windows.h> 

using namespace std;

mutex m;
queue<int32_t> q;
HANDLE handle;

void Producer()
{
    while (true)
    {
        {
            unique_lock<mutex> lock(m);
            q.push(100); // 데이터 추가
            cout << "Produced: 100" << endl;
        }

        ::SetEvent(handle); // 이벤트 신호 상태로 전환 → Consumer가 깨어남
        this_thread::sleep_for(chrono::milliseconds(1000));
    }
}

void Consumer()
{
    while (true)
    {
        // 이벤트가 신호 상태가 될 때까지 대기
        ::WaitForSingleObject(handle, INFINITE);

        unique_lock<mutex> lock(m);
        if (!q.empty())
        {
            int32_t data = q.front();
            q.pop();
            cout << "Consumed: " << data << endl;
        }
    }
}

int main()
{
    // 이벤트 생성
    // bManualReset = FALSE → 자동 리셋 이벤트
    // bInitialState = FALSE → 초기 상태는 비신호
    handle = ::CreateEvent(NULL, FALSE, FALSE, NULL);

    thread t1(Producer);
    thread t2(Consumer);

    t1.join();
    t2.join();

    ::CloseHandle(handle);
}
```

---

## 🔍 동작 원리

1. **Producer**
   - 데이터를 큐에 `push()`.
   - `SetEvent()` 호출 → 이벤트 신호 상태.
   - `Consumer` 스레드가 깨어남.

2. **Consumer**
   - `WaitForSingleObject(handle, INFINITE)`에서 대기.
   - 이벤트가 신호 상태가 되면 실행 재개.
   - 큐에서 데이터를 꺼내 처리.
   - (자동 리셋이므로 이벤트는 자동으로 비신호 상태로 돌아감)

3. **자동 리셋(auto reset) 이벤트 덕분에**
   - 한 번 신호 상태가 되면 **오직 하나의 스레드만** 깨어나도록 제어 가능.

---

## ⚠️ 주의할 점

- **이벤트만으로는 큐의 상태를 완벽히 보장하지 못함.**  
  → 반드시 `mutex`로 큐 접근을 보호해야 한다.  

- **Manual Reset 이벤트를 잘못 쓰면**  
  여러 Consumer 스레드가 동시에 깨어나서 같은 데이터를 꺼내려는 경쟁이 발생할 수 있다.  

- 이벤트는 커널 오브젝트라 **커널 모드 전환 비용이 크다.**  
  → 고성능 서버에서는 `condition_variable` 같은 다른 방법도 고려한다.

---

## 🎮 게임 서버에서의 활용

- **Producer-Consumer 패턴** → 네트워크 패킷 처리, 로그 처리, DB 요청 큐 등에서 활용.  
- 이벤트를 통해 스레드가 **불필요한 busy-waiting을 하지 않도록** 제어 가능.  
- 단, 커널 오브젝트 기반 동기화는 무겁기 때문에 실제 서버에서는 **IOCP + lock-free queue**로 대체하는 경우가 많다.

---

## ✅ 결론

- Event = **스레드 간 신호 전달 장치**.  
- `Producer`가 `SetEvent` → `Consumer`가 `WaitForSingleObject`로 깨어남.  
- 자동 리셋 이벤트를 사용하면 **하나의 소비자만 깨어남**.  
- 큐 동기화에는 반드시 **mutex** 병행 필요.  

---
