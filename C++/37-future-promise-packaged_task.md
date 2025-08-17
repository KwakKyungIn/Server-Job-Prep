# 🔮 `std::future` / `std::promise` / `std::packaged_task` — 완전 가이드

C++ 표준 동시성에서 **결과를 나중에 받는 비동기 계약**을 만드는 핵심 3종 세트:
- 생성자(Producer): **`std::promise`** 또는 **`std::packaged_task`**
- 소비자(Consumer): **`std::future`** (또는 다중 구독용 **`std::shared_future`**)
- 연결고리: **공유 상태(shared state)** — 값/예외/준비 여부를 담는 보이지 않는 객체

> 한 줄 요약: **Promise/Packaged Task → (공유 상태) → Future**  
> Producer가 값을 넣으면(또는 예외를 설정하면) Consumer는 `future.get()`으로 안전하게 꺼내온다.

---

## 1) 개념 흐름과 멘탈 모델

```
Producer thread                      Shared State                       Consumer thread
----------------                      ------------                      ---------------
set_value(x) / set_exception(e) --->  { value | exception | ready } ---> get() / wait()
             (promise, packaged_task)                                  (future, shared_future)
```

- **공유 상태**는 1회성(one-shot)이다. 값/예외가 들어가면 **준비 완료(ready)** 가 되고,
  해당 상태를 관찰하는 `future`는 `get()`으로 **정확히 한 번** 결과를 수령한다.
- `future`는 **이동 전용(move-only)** 이고, `get()`을 호출하면 **더 이상 유효(valid)하지 않다**.
  여러 스레드가 같은 결과를 읽어야 한다면 `future.share()`로 **`std::shared_future`**를 쓰자.

---

## 2) `std::future<T>` — 결과 수령자(Consumer)

### 핵심 특징
- **단 한 번만 `get()` 가능** (결과/예외를 꺼내며 공유 상태 소비)
- `wait()`, `wait_for()`, `wait_until()`로 **블로킹/타임아웃 대기**
- `valid()`로 공유 상태 보유 여부 확인
- 여러 구독자가 필요하면 `auto sf = fut.share();` → `std::shared_future<T>`

### 소비자 측 사용 예
```cpp
#include <future>
#include <thread>
#include <chrono>
#include <iostream>
using namespace std;

int main() {
    promise<int> p;
    future<int>  f = p.get_future(); // 소비자 측 핸들 확보

    thread producer([pp = move(p)]() mutable {
        this_thread::sleep_for(chrono::milliseconds(100));
        pp.set_value(42); // 결과 준비
    });

    if (f.wait_for(chrono::milliseconds(200)) == future_status::ready) {
        cout << "result: " << f.get() << "\n"; // get()은 한 번만
    } else {
        cout << "timeout!\n";
    }

    producer.join();
}
```

---

## 3) `std::promise<T>` — 값을 직접 채워 넣는 Producer

### 핵심 특징
- `p.get_future()`로 **1:1로 대응되는 `future`** 를 만든다(한 번만 호출 가능).
- `set_value(value)` / `set_exception(std::current_exception())`로 결과/예외 설정.
- **파괴 시점에 값/예외를 설정하지 않았다면** 소비자 `future.get()`에서 **`std::future_error(broken_promise)`** 발생.

### 값/예외 전달 예
```cpp
#include <future>
#include <thread>
#include <iostream>
#include <stdexcept>
using namespace std;

void worker(promise<int> p, bool make_error) {
    try {
        if (make_error) throw runtime_error("boom");
        p.set_value(2025);
    } catch (...) {
        p.set_exception(current_exception()); // 예외 전달
    }
}

int main() {
    promise<int> p;
    future<int>  f = p.get_future();

    thread t(worker, move(p), /*make_error=*/false);

    try {
        cout << f.get() << "\n"; // 2025
    } catch (const exception& e) {
        cerr << "error: " << e.what() << "\n";
    }

    t.join();
}
```

> ⚠️ 주의: `promise`가 **값/예외를 설정하지 않고 소멸**하면 `future.get()`에서 `broken_promise`가 던져진다.  
> 즉, **생산자가 반드시 결과를 설정**하도록 생명주기를 관리하자.

---

## 4) `std::packaged_task<Signature>` — 호출 가능한 것을 “미래 결과”와 결합

### 핵심 특징
- 함수/람다/함수객체를 감싸서 **호출 시 결과가 자동으로 공유 상태에 기록**되도록 만든다.
- `task.get_future()`로 소비자 핸들을 얻고, `task(args...)`로 **작업을 실행**(동기 호출)하거나,
  다른 스레드에서 실행되도록 **스케줄**할 수 있다.
- **한 번 `get_future()` 호출 후에는 공유 상태가 결합**된다.  
  결과를 소모한 뒤 **여러 번 재사용하려면** 새 `packaged_task`를 만들거나 **C++14+의 `reset()`**으로 새 공유 상태를 부여한 뒤 다시 `get_future()`를 받아야 한다.

### 태스크 스케줄링 예
```cpp
#include <future>
#include <thread>
#include <iostream>
using namespace std;

int heavy(int x) {
    this_thread::sleep_for(chrono::milliseconds(100));
    return x * x;
}

int main() {
    packaged_task<int(int)> task(heavy);
    future<int> f = task.get_future(); // 소비자 핸들

    thread worker(move(task), 12);     // 다른 스레드에서 task(12) 실행
    cout << "answer: " << f.get() << "\n"; // 144

    worker.join();
}
```

> 팁: **작업을 큐에 넣고 워커 스레드가 `task()`를 호출**하도록 하면 간단한 쓰레드풀/잡큐를 만들 수 있다.

---

## 5) 여러 소비자 필요? → `std::shared_future<T>`

`std::future`는 **단 한 번만 `get()` 가능**하므로, 여러 스레드가 같은 결과를 읽어야 할 때는 **공유형**을 쓰자.

```cpp
#include <future>
#include <thread>
#include <vector>
#include <iostream>
using namespace std;

int main() {
    promise<int> p;
    shared_future<int> sf = p.get_future().share(); // shared_future로 변환

    thread prod([pp = move(p)]() mutable {
        this_thread::sleep_for(chrono::milliseconds(50));
        pp.set_value(7);
    });

    vector<thread> consumers;
    for (int i = 0; i < 3; ++i) {
        consumers.emplace_back([sf]() {
            cout << "recv: " << sf.get() << "\n"; // 여러 번 가능
        });
    }

    for (auto& t : consumers) t.join();
    prod.join();
}
```

---

## 6) 폴링 vs 블로킹: `wait_for()`와 타임아웃

- `wait()` : 준비될 때까지 **무기한 대기**
- `wait_for(dur)` : `dur` 만큼 대기 후 `future_status` 반환
- `wait_until(tp)` : 특정 시각까지 대기

**게임 서버 예시(프레임 루프에서 폴링)**:
```cpp
future<int> f = /* ... */;
while (true) {
    if (f.wait_for(chrono::milliseconds(0)) == future_status::ready) {
        auto v = f.get();
        // 결과 처리
    }
    // 틱 업무 진행...
}
```

---

## 7) 예외 전파(End-to-end)

- Producer가 `set_exception()`으로 예외를 공유 상태에 넣으면,
- Consumer의 `future.get()`에서 **같은 예외가 그대로 다시 던져진다**.

```cpp
packaged_task<int()> task([] {
    throw runtime_error("db disconnected");
    return 1;
});

future<int> f = task.get_future();
thread t([&] { task(); });

try {
    (void)f.get(); // runtime_error rethrow
} catch (const exception& e) {
    cerr << e.what() << "\n";
}
t.join();
```

---

## 8) 실무 팁 & 함정

- **One-shot 규칙**  
  - `future.get()`은 **한 번만**.
  - 같은 결과를 여러 곳에서 읽고 싶으면 **`shared_future`** 사용.
- **락과의 상호작용**  
  - `set_value()` 호출 직전/직후에 **락을 오래 쥐지 말기**.  
    깨워진 소비자 스레드가 같은 락을 필요로 하면 **지연/교착**의 원인이 된다.
- **수명 관리**  
  - `promise`가 파괴되면 `broken_promise`.  
    생산자 스레드에서 반드시 값/예외를 설정하도록 코드 구조를 짜라.
- **재사용**  
  - 같은 함수를 여러 번 태스크로 던질 때는 **새 `packaged_task` 생성**이 가장 단순.  
    (C++14+의 `reset()`도 가능하지만, **새 `future`를 다시 받아야** 한다.)
- **언제 `promise` vs `packaged_task`?**  
  - 결과를 **직접 계산**하는 코드가 이미 있다면 `packaged_task`로 감싸는 게 간결.  
  - 외부 이벤트/콜백에서 **값만 밀어 넣어야** 한다면 `promise`가 적합.

---

## 9) 세 가지를 한 번에 보는 미니 예제

> 시뮬레이션: 작업(패킷 파싱) 함수를 `packaged_task`로 스케줄하고,  
> 외부 I/O 에러는 `promise`로 신호, 결과는 `future`로 수령.

```cpp
#include <future>
#include <thread>
#include <queue>
#include <mutex>
#include <optional>
#include <iostream>
using namespace std;

string parsePacket(string raw) {
    if (raw == "bad") throw runtime_error("malformed");
    return "parsed:" + raw;
}

int main() {
    // 1) 작업용 packaged_task + 결과용 future
    packaged_task<string(string)> task(parsePacket);
    future<string> result = task.get_future();

    // 2) 외부 에러 신호용 promise (void)
    promise<void> cancel_signal;
    shared_future<void> cancel = cancel_signal.get_future().share();

    // 워커 스레드: 작업 실행 + 예외 전파
    thread worker([t = move(task), cancel]() mutable {
        try {
            // cancel이 준비되면(값/예외) wait 종료
            if (cancel.wait_for(chrono::milliseconds(0)) == future_status::ready)
                throw runtime_error("canceled");

            t("hello"); // parsePacket 호출 → 결과는 result로
        } catch (...) {
            // packaged_task 내부에서 던진 예외는 자동으로 future로 전달
            // (별도 set_exception 불필요)
        }
    });

    // 메인 스레드: 결과 수령
    try {
        cout << result.get() << "\n";
    } catch (const exception& e) {
        cerr << "error: " << e.what() << "\n";
    }

    worker.join();
}
```

---

## 📌 기억 포인트 요약

- **`future`**: 결과 1회 소비, `get()/wait()` 제공.
- **`promise`**: 값을 직접 넣어주는 1:1 채널. **값/예외는 정확히 한 번**.
- **`packaged_task`**: 호출 가능한 것을 “미래 결과”와 결합. 스케줄링하기 좋음.
- **`shared_future`**: 같은 결과를 **여러 소비자**가 읽을 때.

> 이 3종만 익숙해져도 **쓰레드 간 결과 전달/동기화** 문제의 80%는 깔끔하게 풀린다.  
> 다음 단계로는 `std::async`(태스크 런처), 간단한 **스레드풀 + 작업 큐** 설계를 추천!
