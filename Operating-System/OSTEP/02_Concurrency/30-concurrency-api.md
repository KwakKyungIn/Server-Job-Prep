# 🔧 병행성: 쓰레드 API (OSTEP Chapter 30)

이 장에서는 POSIX 쓰레드 API를 통해 **쓰레드 생성, 종료, 동기화** 같은 기본적인 기능을 어떻게 사용하는지 소개한다.  
각 함수의 역할과 사용법만 가볍게 다루며, 자세한 내용은 이후의 장들에서 심화 학습한다.

---

## ❓ 핵심 질문

> 운영체제가 쓰레드를 관리할 수 있도록 어떤 인터페이스(API)를 제공해야 할까?  
> 그 인터페이스는 어떻게 설계되어야 쉬우면서도 강력할까?

---

## 🔍 상세 개념 정리

### 30.1 쓰레드 생성 (`pthread_create`)

#### ✅ 기본 함수
```c
int pthread_create(pthread_t *thread,
                   const pthread_attr_t *attr,
                   void *(*start_routine)(void*),
                   void *arg);
```

#### 🔍 주요 인자 설명
| 인자 | 설명 |
|-----|------|
| `thread` | 생성된 쓰레드를 식별하기 위한 포인터 |
| `attr` | 쓰레드 속성 (NULL로 대부분 대체 가능) |
| `start_routine` | 쓰레드가 실행할 함수 (함수 포인터) |
| `arg` | 함수에 전달할 인자 (void* 타입) |

→ **함수 포인터 형태가 복잡해 보이지만**, void* 인자를 받아 void*를 반환하는 일반적인 형태.

#### 🧑‍💻 예제
```c
typedef struct {
    int a;
    int b;
} myarg_t;

void *mythread(void *arg) {
    myarg_t *m = (myarg_t *) arg;
    printf("%d %d\n", m->a, m->b);
    return NULL;
}

int main() {
    pthread_t p;
    myarg_t args = {10, 20};
    pthread_create(&p, NULL, mythread, &args);
    ...
}
```

---

### 30.2 쓰레드 종료 및 기다리기 (`pthread_join`)

#### ✅ 기본 함수
```c
int pthread_join(pthread_t thread, void **retval);
```

#### 🔍 주요 동작
| 항목 | 설명 |
|-----|------|
| `thread` | 기다릴 쓰레드 ID |
| `retval` | 쓰레드의 반환 값 저장 포인터 (필요 없으면 NULL) |

#### 🧑‍💻 반환 값 사용 예시
```c
typedef struct {
    int x;
    int y;
} myret_t;

void *mythread(void *arg) {
    myret_t *r = malloc(sizeof(myret_t));
    r->x = 1;
    r->y = 2;
    return (void *)r;
}

int main() {
    pthread_t p;
    myret_t *result;
    pthread_create(&p, NULL, mythread, NULL);
    pthread_join(p, (void **) &result);
    printf("returned %d %d\n", result->x, result->y);
}
```

#### ⚠️ 주의
- 쓰레드의 **스택 변수 주소를 반환하면 안 됨.**
    - 리턴 시 스택이 사라지므로, 반환 값은 **heap에 할당(malloc 등)** 해야 함.

---

### 30.3 락 (`pthread_mutex_lock`, `pthread_mutex_unlock`)

#### ✅ 기본 함수
```c
pthread_mutex_t lock;
pthread_mutex_lock(&lock);
critical_section();
pthread_mutex_unlock(&lock);
```

#### 🔍 초기화 방법
| 방법 | 설명 |
|------|------|
| 정적 초기화 | `pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;` |
| 동적 초기화 | `pthread_mutex_init(&lock, NULL);` |

#### 🛡️ 올바른 사용법
- 반드시 초기화 후 사용
- 에러 코드를 확인해야 함
- 락을 획득한 쓰레드만이 언락해야 함

#### ✅ 에러 체크 예시 (래퍼 함수)
```c
void Pthread_mutex_lock(pthread_mutex_t *mutex) {
    int rc = pthread_mutex_lock(mutex);
    assert(rc == 0);
}
```

#### 기타 락 함수
| 함수 | 설명 |
|------|------|
| pthread_mutex_trylock() | 락이 없으면 획득, 있으면 실패 반환 |
| pthread_mutex_timedlock() | 일정 시간 동안만 시도 |

---

### 30.4 컨디션 변수 (`pthread_cond_wait`, `pthread_cond_signal`)

#### ✅ 기본 함수
```c
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

pthread_mutex_lock(&lock);
while (ready == 0)
    pthread_cond_wait(&cond, &lock);
pthread_mutex_unlock(&lock);
```

#### 🔍 사용법
| 항목 | 설명 |
|-----|------|
| `pthread_cond_wait()` | 락을 잠금 상태로 호출, 내부적으로 대기 + unlock |
| `pthread_cond_signal()` | 대기 중인 쓰레드 하나를 깨움 |

#### 🚨 주의할 점
- 조건 변수를 사용할 때는 **반드시 락을 보유하고 있어야 한다.**
- 조건 체크 시 반드시 **while문** 사용 (신호만 받고 조건이 안 바뀌었을 수도 있음)

#### ❌ 잘못된 예시
```c
while (ready == 0)
    ; // 플래그만 확인 → 성능 저하 + 버그 발생 가능
```

---

### 30.5 컴파일과 실행

#### ✅ 컴파일 명령
```bash
gcc -o main main.c -Wall -pthread
```

- `pthread.h` 헤더 포함
- `-pthread` 옵션으로 pthread 라이브러리 링크

---

## ✅ 요약

이 장에서는 멀티 쓰레드 프로그램을 작성할 때 필수적인 pthread API를 소개했다.

| 기능 | API |
|------|--------------------------------------|
| 쓰레드 생성 | `pthread_create()` |
| 쓰레드 종료 대기 | `pthread_join()` |
| 락 | `pthread_mutex_lock()`, `pthread_mutex_unlock()` |
| 컨디션 변수 | `pthread_cond_wait()`, `pthread_cond_signal()` |

이 API를 활용해 멀티 쓰레드 프로그램을 작성할 수 있지만, 병행성 문제를 완전히 해결하려면 **적절한 동기화 설계와 주의 깊은 사용법** 이 필요하다.

---

> 📚 출처: 『운영체제: 아주 쉬운 세 가지 이야기 (OSTEP)』
