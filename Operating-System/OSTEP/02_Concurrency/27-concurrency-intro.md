# 🔀 병행성: 개요 (OSTEP Chapter 27)

지금까지 운영체제는 하나의 CPU를 여러 프로그램이 사용하는 것처럼 보이게 하거나, 메모리를 가상화하는 등 **자원 가상화**에 집중했다.  
이번 장부터는 **병행성(Concurrency)** 개념을 다룬다.  
병행성은 **여러 흐름의 실행이 겹치는 상황**을 의미하며, 이 흐름은 **쓰레드(thread)** 로 표현된다.

---

## ❓ 핵심 질문

> 하나의 프로세스 안에서 여러 실행 흐름이 병행할 때, 이 흐름들이 안전하게 동작하려면 어떻게 관리해야 할까?

---

## 🔍 상세 개념 정리

### 27.1 쓰레드란 무엇인가?

#### 🧠 쓰레드의 기본 개념
- **프로세스(Process):** 독립적인 주소 공간을 가진 실행 단위
- **쓰레드(Thread):** 주소 공간을 공유하지만 독립적으로 실행되는 흐름

#### 📦 쓰레드의 구성 요소
| 구성 요소 | 설명 |
|-----------|------|
| 프로그램 카운터(PC) | 현재 실행 중인 명령어 주소 |
| 레지스터 | 계산을 위한 저장소 |
| 스택 | 함수 호출 시 사용하는 메모리 (쓰레드마다 별도 존재) |
| 주소 공간 | 코드, 데이터, 힙, 파일 등 (프로세스 전체가 공유) |

#### 📂 단일 쓰레드 vs 멀티 쓰레드
| 구분 | 구조 |
|------|------|
| 단일 쓰레드 | 하나의 스택만 존재 |
| 멀티 쓰레드 | 각 쓰레드마다 별도의 스택 보유, 나머지는 공유 |

---

### 27.2 쓰레드 생성 예제

#### ✅ 코드 예시 (`t0.c`)
```c
#include <stdio.h>
#include <pthread.h>
#include <assert.h>

void *mythread(void *arg) {
    printf("%s\n", (char *) arg);
    return NULL;
}

int main(int argc, char *argv[]) {
    pthread_t p1, p2;
    printf("main: begin\n");
    assert(pthread_create(&p1, NULL, mythread, "A") == 0);
    assert(pthread_create(&p2, NULL, mythread, "B") == 0);
    assert(pthread_join(p1, NULL) == 0);
    assert(pthread_join(p2, NULL) == 0);
    printf("main: end\n");
    return 0;
}
```

#### 🔍 실행 순서 예시
| Main | 쓰레드 1 | 쓰레드 2 |
|------|----------|----------|
| main: begin 출력 | | |
| 쓰레드 1 생성 | A 출력 | |
| 쓰레드 2 생성 | | B 출력 |
| main: end 출력 | | |

→ 실행 순서는 매번 다르게 나올 수 있음 (병행성 때문)

---

### 27.3 더 어려운 문제: 공유 데이터

#### ⚠️ 공유 데이터 문제 예제 (`t1.c`)
```c
#include <stdio.h>
#include <pthread.h>
#include "mythreads.h"

static volatile int counter = 0;

void *mythread(void *arg) {
    printf("%s: begin\n", (char *) arg);
    for (int i = 0; i < 1e7; i++) {
        counter = counter + 1;
    }
    printf("%s: done\n", (char *) arg);
    return NULL;
}

int main(int argc, char *argv[]) {
    pthread_t p1, p2;
    printf("main: begin (counter = %d)\n", counter);
    Pthread_create(&p1, NULL, mythread, "A");
    Pthread_create(&p2, NULL, mythread, "B");
    Pthread_join(p1, NULL);
    Pthread_join(p2, NULL);
    printf("main: done with both (counter = %d)\n", counter);
    return 0;
}
```

#### 🛑 기대 결과
- counter = 20,000,000

#### ❌ 실제 결과
- 실행할 때마다 다름 (예: 19,345,221)

#### 📉 이유
- 두 쓰레드가 동시에 `counter = counter + 1`을 실행하는 과정에서 충돌 발생

---

### 27.4 CPU 명령어 관점에서 본 문제

#### 🔬 counter 증가 연산의 실제 동작
```asm
mov 0x8049a1c, %eax   // counter 값 로드
add $0x1, %eax        // 1 증가
mov %eax, 0x8049a1c   // 다시 저장
```

#### ⚙️ 경쟁 조건 (Race Condition)
- 쓰레드 1이 값을 읽고 증가시킨 사이에 쓰레드 2가 값을 덮어쓰기 때문에 결과가 잘못됨
- 이런 코드를 **임계 영역(Critical Section)** 이라고 함

---

### 27.5 병행성 문제 핵심 용어 정리

| 용어 | 정의 |
|------|------|
| 임계 영역 (Critical Section) | 동시에 실행되면 안 되는 코드 영역 |
| 경쟁 조건 (Race Condition) | 여러 쓰레드가 동시에 임계 영역에 들어가 결과가 비결정적이 되는 상황 |
| 비결정성 (Indeterminacy) | 실행할 때마다 결과가 달라지는 상태 |
| 상호 배제 (Mutual Exclusion) | 한 번에 하나의 쓰레드만 임계 영역 실행 가능하게 함 |

---

### 27.6 원자성(Atomicity)에 대한 필요성

#### ✅ 이상적인 상황
- 단일 명령어로 메모리 값을 증가시키면 경쟁 조건 없음
```asm
memory-add 0x8049a1c, $0x1  // 상상 속의 원자적 명령어
```

#### ❌ 현실
- 대부분의 복잡한 연산은 여러 명령어로 이루어짐
- 운영체제/하드웨어/라이브러리가 제공하는 **동기화 함수(synchronization primitives)** 로 해결해야 함

---

### 27.7 또 다른 문제: 동기화 대기

- 단순한 공유 변수 문제뿐만 아니라, **다른 쓰레드가 끝날 때까지 기다리는 문제(waiting)** 도 발생
- 예: I/O 완료 대기, 다른 쓰레드가 초기화 완료할 때까지 대기

#### 해결책
- 조건 변수(Condition Variable)
- 세마포어(Semaphore)
- 락(Lock)

---

### 27.8 운영체제에서 다루는 이유

#### 역사적 이유
- 운영체제는 최초의 병행 프로그램
- 커널 내부에서도 인터럽트, 파일 시스템, 페이지 테이블 등 **공유 자료구조 보호** 필요

#### 현대적 이유
- 사용자 프로그램도 멀티쓰레드 사용
- 운영체제가 제공하는 병행성 지원을 통해 안정성 보장

---

## ✅ 요약

병행성은 단순히 쓰레드 여러 개를 만들고 동시에 실행하는 것을 넘어서,  
**어떻게 하면 이 쓰레드들이 안전하게, 올바르게 실행되는지** 를 고민하는 것이다.

| 개념 | 설명 |
|------|------|
| 쓰레드 | 주소 공간 공유, 독립적 실행 흐름 |
| 병행성 | 여러 쓰레드가 동시에 실행되는 것처럼 보이는 현상 |
| 임계 영역 | 동시에 실행되면 안 되는 코드 |
| 경쟁 조건 | 실행 순서에 따라 결과가 달라지는 문제 |
| 상호 배제 | 경쟁 조건을 방지하기 위한 핵심 개념 |

→ 병행성은 컴퓨터 시스템의 복잡도를 급격히 높이지만, 필수적인 기능이다.

---

> 📚 출처: 『운영체제: 아주 쉬운 세 가지 이야기 (OSTEP)』
