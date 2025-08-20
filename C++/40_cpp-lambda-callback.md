
# 🌱 C++ 람다(lambda) + std::function + 콜백 정리

람다 자체는 **익명 함수**지만, 보통 단독으로 쓰기보다는  
**`std::function`**, **콜백(callback)** 구조와 함께 서버 프로그래밍에서 많이 쓰인다.  
이 세 가지를 비교하고, 면접에서도 자주 나오는 포인트를 정리한다.

---

## 1. 람다 (lambda)

### 특징
- 간단하게 **이름 없는 함수** 정의 가능
- 외부 변수 캡처 가능
- 코드가 짧아져서 콜백 함수 전달할 때 매우 유용

### 예시
```cpp
int x = 10;
auto printX = [=]() { std::cout << "x = " << x << std::endl; };
printX();
````

---

## 2. std::function

### 특징

* **람다, 함수 포인터, 함수 객체** 등 "호출 가능한 것(callable)" 을 **하나의 타입**으로 감싸는 도구
* 즉, 다양한 형태의 함수를 같은 방식으로 관리 가능
* 특히 **콜백 함수 등록/관리**에 쓰임

### 예시

```cpp
#include <functional>

void Hello() { std::cout << "Hello\n"; }

int main() {
    std::function<void()> f1 = Hello;               // 함수 포인터
    std::function<void()> f2 = []() { std::cout << "Lambda\n"; }; // 람다
    f1();
    f2();
}
```

---

## 3. 콜백 함수 (Callback)

### 특징

* **특정 이벤트가 발생했을 때 호출되는 함수**
* "내가 실행할 코드를 미리 등록해놓고, 나중에 호출" → 서버, 네트워크 이벤트에서 필수적
* C++에서는 **함수 포인터, 함수 객체, 람다, std::function** 전부 콜백으로 사용 가능

### 예시 (게임 서버 느낌)

```cpp
#include <iostream>
#include <functional>
#include <string>

class Button {
public:
    void SetOnClick(std::function<void()> callback) {
        onClick = callback;
    }
    void Click() {
        if (onClick) onClick(); // 등록된 콜백 실행
    }
private:
    std::function<void()> onClick;
};

int main() {
    Button btn;

    // 람다로 콜백 등록
    btn.SetOnClick([]() { std::cout << "Button Clicked!\n"; });
    btn.Click(); // 이벤트 발생 시 콜백 실행
}
```

---

## 4. 세 가지 비교

| 개념                | 설명                              | 장점                      | 단점               |
| ----------------- | ------------------------------- | ----------------------- | ---------------- |
| **람다**            | 이름 없는 함수 정의                     | 짧고 간결, 캡처 가능            | 타입이 복잡, 저장 어려움   |
| **std::function** | 함수, 람다, 함수 객체를 감쌀 수 있는 범용 함수 래퍼 | 통일된 인터페이스 제공, 콜백 관리에 용이 | 약간의 성능 오버헤드      |
| **콜백**            | 특정 시점/이벤트에 실행할 함수 개념            | 이벤트 기반 프로그래밍 가능         | 관리 안 하면 콜백 지옥 발생 |

---

## 5. 서버 프로그래밍에서 활용

* **IOCP 서버, 이벤트 루프**에서 많이 사용

  * "데이터 수신 → 처리 함수 호출" 구조
* **멀티스레드 환경**에서 태스크 실행시 람다를 콜백으로 넘겨줌
* **게임 서버**에서 스킬 사용, 버튼 클릭, 네트워크 이벤트 등 이벤트 처리 로직 작성 시 사용

예시 (스레드 풀 콜백):

```cpp
#include <iostream>
#include <thread>
#include <queue>
#include <functional>
#include <mutex>
#include <condition_variable>

std::queue<std::function<void()>> jobs;
std::mutex m;
std::condition_variable cv;

void Worker() {
    while (true) {
        std::function<void()> job;
        {
            std::unique_lock<std::mutex> lock(m);
            cv.wait(lock, [] { return !jobs.empty(); });
            job = jobs.front();
            jobs.pop();
        }
        job(); // 등록된 작업(콜백) 실행
    }
}

int main() {
    std::thread t(Worker);

    {
        std::lock_guard<std::mutex> lock(m);
        jobs.push([]() { std::cout << "Job 1 executed\n"; });
        jobs.push([]() { std::cout << "Job 2 executed\n"; });
    }
    cv.notify_one();

    t.join();
}
```

---

## ✅ 정리

* **람다**: 이름 없는 간단한 함수, 콜백 전달할 때 유용
* **std::function**: 모든 호출 가능 객체(callable)를 담을 수 있는 범용 컨테이너
* **콜백**: 특정 이벤트 시 실행할 함수 개념 (람다/함수포인터/함수객체 모두 가능)
* 서버 프로그래밍에서는 **람다 + std::function 기반 콜백 구조**가 필수적

> 면접 포인트:
>
> * "람다와 std::function 차이"
> * "왜 서버에서 콜백 구조가 필요한가?"
> * "콜백 지옥 문제를 어떻게 해결할 수 있는가? (async/await, future, coroutine 등)"

