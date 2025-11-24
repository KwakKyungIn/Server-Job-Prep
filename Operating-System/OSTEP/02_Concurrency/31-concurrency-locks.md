# 🔒 병행성: 락 (OSTEP Chapter 31)

이 장에서는 병행 프로그램에서 가장 기본적인 동기화 기법인 **락(lock)** 에 대해 설명한다.  
락은 공유 자원에 동시에 접근하는 여러 쓰레드 간의 충돌을 막기 위해 사용된다.  
하드웨어 지원 없이 단순히 인터럽트를 끄거나, 하드웨어 명령어(Test-And-Set, Compare-And-Swap 등)를 사용하여 어떻게 락을 구현할 수 있는지 구체적으로 다룬다.

---

## ❓ 핵심 질문

> 병행 프로그램에서 **여러 명령어 블록을 원자적으로 실행**하려면 어떻게 해야 할까?  
> 하드웨어나 운영체제의 지원 없이 프로그래머가 스스로 이 문제를 어떻게 해결할 수 있을까?

---

## 🔍 상세 개념 정리

### 31.1 락의 기본 개념

#### ✅ 락으로 보호하는 코드 예시
```c
lock(&mutex);
balance = balance + 1;
unlock(&mutex);
```

- `lock()` 함수 호출: 락 획득 시도
- 성공하면 임계 영역 진입
- `unlock()` 함수 호출로 락 해제

#### 락 상태
| 상태 | 의미 |
|------|------|
| 해제(Unlocked) | 사용 가능한 상태 |
| 사용 중(Acquired) | 어떤 쓰레드가 사용 중 |

#### 락의 역할
- **임계 영역(Critical Section)** 에 동시에 두 쓰레드가 진입하는 것을 막는다
- 락을 보유한 쓰레드를 **락 소유자(owner)** 라고 한다

---

### 31.2 Pthread 락 예시

#### POSIX 락 코드
```c
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_lock(&lock);
balance = balance + 1;
pthread_mutex_unlock(&lock);
```

- `pthread_mutex_lock()`: 락 획득 시도
- `pthread_mutex_unlock()`: 락 해제
- 여러 락을 만들어 서로 다른 데이터 보호 가능 → 세밀한 락 전략(fine-grained locking)

---

### 31.3 락 구현의 필요 요소

- 하드웨어 명령어의 도움 필요 (단순한 변수만으로는 안전하지 않음)
- 운영체제가 제공하는 기본 지원도 필요
- 효율적 락을 만들기 위한 평가 기준:
    - 상호 배제(Mutual Exclusion)
    - 공정성(Fairness)
    - 성능(Performance)

---

### 31.4 인터럽트 차단 방식

#### ✅ 단일 프로세서용 기본 락
```c
void lock() {
    DisableInterrupts();
}
void unlock() {
    EnableInterrupts();
}
```

#### ❌ 단점
- 사용자 코드가 시스템 전체 인터럽트를 막으면 심각한 문제 발생
- 멀티 프로세서 환경에선 쓸모 없음
- 장시간 인터럽트 차단 시 시스템 응답성 저하

---

### 31.5 Test-And-Set (TAS)

#### 🛡️ 하드웨어 명령어 기반 원자 연산
```c
typedef struct { int flag; } lock_t;

void init(lock_t *mutex) { mutex->flag = 0; }

void lock(lock_t *mutex) {
    while (TestAndSet(&mutex->flag, 1) == 1) ;
}

void unlock(lock_t *mutex) { mutex->flag = 0; }
```

- TestAndSet(): 값 읽기 + 새로운 값 설정을 원자적으로 수행
- 임계 영역 내에 **하나의 쓰레드만** 존재하게 보장

---

### 31.6 스핀 락의 한계

- 단일 CPU에선 매우 비효율적
- 멀티 CPU에선 어느 정도 효율적
- 공정성 부족 → 일부 쓰레드가 굶주림(starvation) 발생 가능

---

### 31.7 Compare-And-Swap (CAS)

#### ✅ 코드 예시
```c
int CompareAndSwap(int *ptr, int expected, int new) {
    int actual = *ptr;
    if (actual == expected) *ptr = new;
    return actual;
}
```

- 값이 예상한 값이면 교체
- 실패 시 재시도 필요
- Test-And-Set보다 더 강력한 동기화 지원

---

### 31.8 Load-Linked / Store-Conditional (LL/SC)

#### ✅ C 의사 코드
```c
int LoadLinked(int *ptr) { return *ptr; }

int StoreConditional(int *ptr, int value) {
    if (no one has updated *ptr since LoadLinked)
        *ptr = value;
        return 1; // 성공
    return 0; // 실패
}
```

- 두 명령어 사이에 값이 변경되지 않았으면 저장 성공
- 일부 ARM, PowerPC, MIPS 시스템에서 지원

---

### 31.9 티켓 락 (Ticket Lock)

#### ✅ Fetch-And-Add 기반 락
```c
typedef struct { int ticket, turn; } lock_t;

void lock(lock_t *lock) {
    int myturn = FetchAndAdd(&lock->ticket);
    while (lock->turn != myturn) ;
}

void unlock(lock_t *lock) {
    lock->turn++;
}
```

- 순서대로 락을 획득 → **공정성 보장**
- 대기 쓰레드는 자신의 순서가 될 때까지 기다림

---

### 31.10 스핀 락의 성능 문제

| 환경 | 효율성 |
|------|--------|
| 단일 CPU | 매우 비효율적 |
| 멀티 CPU | 임계 영역 짧으면 효율적 |

단일 CPU에서 락 보유 쓰레드가 선점되면, 나머지 쓰레드가 **계속 spin-wait** → 성능 저하

---

### 31.11 스핀 줄이기 전략

#### 🔄 양보(yield)
```c
while (TestAndSet(&flag, 1) == 1)
    yield();
```
- 락이 사용 중이면 CPU 양보
- 단일 CPU 환경에서 효율적이지만 멀티 쓰레드 상황에선 한계

#### 💤 큐 + 대기/깨우기
- Solaris: `park()` / `unpark()` 시스템 콜 사용
- 대기 중인 쓰레드 목록을 관리하고 깨울 쓰레드 지정

---

### 31.12 Linux의 futex

| 시스템 콜 | 동작 |
|-----------|------|
| futex_wait(addr, expected) | 값이 일치하면 대기 |
| futex_wake(addr) | 대기 중인 쓰레드 깨움 |

- Linux는 **futex** 로 커널이 직접 대기 관리
- 스핀 락과 대기 락을 상황에 따라 적절히 조합

---

### 31.13 2단계 락 (Two-phase lock)

- 락이 곧 해제될 것 같으면 스핀으로 기다림
- 일정 시간 안에 못 얻으면 커널이 관리하는 대기 큐로 전환

---

### 31.14 요약: 락 구현의 핵심

| 요소 | 설명 |
|------|------|
| 하드웨어 지원 | Test-And-Set, CAS, LL/SC 등 |
| 운영체제 지원 | park/unpark, futex 등 |
| 공정성 | 티켓 락 등으로 개선 |
| 성능 | 상황에 따라 스핀 락, 대기 락 혼합 |

---

## ✅ 요약

병행성 문제를 해결하기 위해 **락** 을 사용한다.  
하드웨어 명령어(Test-And-Set, CAS, LL/SC 등)나 운영체제의 대기/깨우기 기능을 통해 안정적이고 성능 좋은 락을 만들 수 있다.

| 락 종류 | 특징 |
|--------|------|
| 스핀 락 | 단순하지만 단일 CPU에선 비효율적 |
| 티켓 락 | 공정성 보장 |
| 큐 기반 락 | 굶주림 방지 |
| 2단계 락 | 짧게 스핀 후 커널 대기 |

이 장에서 배운 락들은 다음 장들에서 락을 어떻게 사용하고 최적화하는지의 기초가 된다.

---

> 📚 출처: 『운영체제: 아주 쉬운 세 가지 이야기 (OSTEP)』
