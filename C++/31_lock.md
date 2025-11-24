# C++ 멀티스레드 동기화 - Lock과 lock_guard 기초

## 1. Lock의 개념
멀티스레드 환경에서는 여러 쓰레드가 **동시에 동일한 자원**(변수, 메모리, 파일 등)에 접근하면 **데이터 경합(Race Condition)**이 발생할 수 있다.  
이를 방지하기 위해 **동기화(Synchronization)** 기법을 사용한다.  
그 중 하나가 **Lock(뮤텍스, Mutex)** 이다.

### 기본 원리
- **lock**: 특정 자원을 점유(잠금) 상태로 만들어 다른 쓰레드가 접근하지 못하게 함.
- **unlock**: 점유를 해제(잠금 해제)하여 다른 쓰레드가 접근 가능하게 함.

즉, **lock을 획득한 쓰레드만** 해당 자원에 접근 가능하다.

---

## 2. 기본 사용 예시

```cpp
#include <iostream>
#include <thread>
#include <mutex>

std::mutex mtx; // 뮤텍스 객체
int shared_data = 0;

void add()
{
    mtx.lock(); // 🔒 잠금 시작
    shared_data++;
    std::cout << "Add: " << shared_data << "\n";
    mtx.unlock(); // 🔓 잠금 해제
}

void sub()
{
    mtx.lock(); // 🔒 잠금 시작
    shared_data--;
    std::cout << "Sub: " << shared_data << "\n";
    mtx.unlock(); // 🔓 잠금 해제
}

int main()
{
    std::thread t1(add);
    std::thread t2(sub);

    t1.join();
    t2.join();
}
```

---

## 3. lock / unlock의 문제점
- **unlock 호출 누락**: 예외가 발생하거나, 함수가 중간에 return되면 unlock 호출이 누락될 수 있음.
- 누락되면 다른 쓰레드가 **영원히 대기 상태(Deadlock)**에 빠질 수 있음.
- 이 문제를 해결하기 위해 C++에서는 **`std::lock_guard`**를 사용한다.

---

## 4. lock_guard로 안전하게 잠금 관리
`lock_guard`는 **생성자에서 자동으로 lock**을 걸고, **소멸자에서 자동으로 unlock**을 호출한다.  
RAII(Resource Acquisition Is Initialization) 원리를 이용하기 때문에, 예외나 조기 반환 상황에서도 **자동으로 unlock**이 보장된다.

```cpp
#include <iostream>
#include <thread>
#include <mutex>

std::mutex mtx;
int shared_data = 0;

void add()
{
    std::lock_guard<std::mutex> lock(mtx); // 🔒 생성 시 lock
    shared_data++;
    std::cout << "Add: " << shared_data << "\n";
} // 🔓 함수 끝나면 lock_guard 소멸 -> 자동 unlock

void sub()
{
    std::lock_guard<std::mutex> lock(mtx);
    shared_data--;
    std::cout << "Sub: " << shared_data << "\n";
}

int main()
{
    std::thread t1(add);
    std::thread t2(sub);

    t1.join();
    t2.join();
}
```

---

## 5. 게임 서버 개발에서의 활용
- **공유 데이터 보호**: 예) MMORPG에서 플레이어 목록, 몬스터 상태, 채팅 로그 같은 전역 자료 구조
- **DB 연동 시 안전성 보장**: 여러 쓰레드가 동시에 DB 쿼리를 날리지 않도록 보호
- **패킷 처리 큐**: 여러 세션에서 동시에 push/pop하는 작업에서 데이터 무결성 유지

---

## 6. 주의사항
1. **잠금 범위 최소화**  
   - 너무 넓게 잠그면 성능 저하 (병렬성 감소)
2. **락 순서 일관성 유지**  
   - 여러 뮤텍스를 사용할 때 순서를 지키지 않으면 교착 상태(deadlock) 발생 가능
3. **lock_guard와 std::unique_lock 구분**  
   - lock_guard: 단순 잠금/해제
   - unique_lock: lock/unlock을 코드 중간에 조절 가능, 조건 변수와 함께 사용 가능

---

## 최종 요약 표

| 구분         | 설명 |
|--------------|------|
| **lock()**   | 뮤텍스를 잠그고 다른 쓰레드 접근 차단 |
| **unlock()** | 뮤텍스 잠금 해제 |
| **lock_guard** | 생성 시 lock, 소멸 시 unlock (RAII 원칙) |
| **장점**     | 예외나 조기 반환 시에도 안전하게 unlock 보장 |
| **단점**     | 잠금 범위를 세밀하게 조절할 수 없음 (필요시 unique_lock 사용) |
