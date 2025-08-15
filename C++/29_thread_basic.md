# 🧵 C++ 쓰레드 기본 개념 및 사용법 정리

## 1. 쓰레드 기본 개념
- `std::thread`는 C++11부터 표준에 포함된 쓰레드 클래스.
- 새로운 쓰레드를 생성하여 동시에 여러 작업을 수행할 수 있음.
- 멀티코어 환경에서 성능을 높이거나, I/O 대기 중인 CPU를 효율적으로 활용 가능.

---

## 2. 기본 환경 설정
```cpp
#include <iostream>
#include <thread>

void worker(int n) {
    std::cout << "Worker " << n << " is running\n";
}

int main() {
    std::thread t(worker, 1); // 새로운 쓰레드 생성
    t.join(); // 메인 쓰레드가 t의 작업을 기다림
}
```
- `std::thread` 객체를 생성할 때 **실행할 함수**와 **인자**를 전달.
- 쓰레드 생성 후 반드시 `join()` 또는 `detach()`로 처리해야 함.  
  (그렇지 않으면 프로그램이 종료될 때 `std::terminate()`가 호출됨)

---

## 3. 주요 멤버 함수

### 3.1 `hardware_concurrency()`
- 시스템에서 **동시에 실행 가능한 하드웨어 쓰레드 개수**를 반환.
- 보통 CPU의 논리 코어 개수를 의미.

```cpp
#include <iostream>
#include <thread>

int main() {
    unsigned int n = std::thread::hardware_concurrency();
    std::cout << "가능한 쓰레드 수: " << n << "\n";
}
```

---

### 3.2 `get_id()`
- 쓰레드 객체의 **고유 ID**를 반환.
- `std::thread::id` 타입으로 반환되며, 디버깅 시 유용.

```cpp
#include <iostream>
#include <thread>

void task() {
    std::cout << "Thread ID: " << std::this_thread::get_id() << "\n";
}

int main() {
    std::thread t(task);
    std::cout << "Main Thread ID: " << std::this_thread::get_id() << "\n";
    t.join();
}
```

---

### 3.3 `joinable()`
- 해당 쓰레드가 `join()` 또는 `detach()` 가능 상태인지 확인.
- 이미 `join()`/`detach()`를 호출했거나, 쓰레드가 생성되지 않았다면 `false` 반환.

```cpp
#include <iostream>
#include <thread>

void work() {}

int main() {
    std::thread t(work);
    if (t.joinable()) { // join 가능 여부 확인
        t.join();
    }
}
```
**왜 필요한가?**  
- `join()`을 두 번 호출하면 예외가 발생하므로, `joinable()`로 체크 후 호출.

---

### 3.4 `join()`
- 해당 쓰레드가 끝날 때까지 현재 쓰레드를 **블로킹**.
- 쓰레드 동기화의 가장 기본적인 방법.

```cpp
#include <iostream>
#include <thread>

void work() {
    std::cout << "작업 시작\n";
}

int main() {
    std::thread t(work);
    t.join(); // t가 끝날 때까지 기다림
    std::cout << "메인 쓰레드 계속 실행\n";
}
```

---

### 3.5 `detach()`
- 쓰레드를 **백그라운드에서 독립 실행**시키고, 쓰레드 객체와 분리.
- 메인 쓰레드가 종료되더라도 백그라운드에서 계속 실행됨.
- **주의**: detach한 쓰레드는 제어 불가능 → 자원 정리가 까다로움.

```cpp
#include <iostream>
#include <thread>
#include <chrono>

void backgroundTask() {
    std::this_thread::sleep_for(std::chrono::seconds(3));
    std::cout << "백그라운드 작업 완료\n";
}

int main() {
    std::thread t(backgroundTask);
    t.detach();
    std::cout << "메인 쓰레드 종료 직전\n";
}
```
