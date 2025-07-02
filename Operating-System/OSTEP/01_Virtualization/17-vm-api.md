# 📦 메모리 관리 API (OSTEP Chapter 17)

C 언어에서는 프로그램이 실행 중 필요한 메모리를 명시적으로 할당하고 해제해야 한다. 이 장에서는 Unix 시스템에서 제공하는 메모리 관리 인터페이스들, 특히 `malloc()`, `free()`, `calloc()`, `realloc()` 함수와 그 사용상의 주의점들을 학습한다. 또한 흔히 발생하는 **메모리 오류 유형**과 디버깅 도구도 소개한다.

---

## ❓ 핵심 질문

> **어떻게 사용자 프로그램이 메모리를 동적으로 안전하고 효율적으로 할당/해제할 수 있을까?**

---

## 🔍 상세 개념 정리

### 17.1 메모리 공간의 두 종류: 스택과 힙

#### 📌 스택 메모리 (Stack)
- 지역 변수용
- 함수 호출 시 자동으로 할당되고, 리턴 시 자동 해제
- 컴파일러가 처리하므로 **프로그래머가 직접 관리할 필요 없음**

```c
void func() {
    int x = 10;  // 스택에 자동 할당
}
```

> 자동 메모리(Automatic memory)라고도 불림

#### 📌 힙 메모리 (Heap)
- 명시적으로 `malloc()` 등을 통해 할당
- **프로그래머가 해제까지 직접 책임져야 함** → 메모리 누수 위험 존재

```c
void func() {
    int *x = (int *) malloc(sizeof(int));  // 힙에 동적 할당
    *x = 42;
    free(x);  // 반드시 해제
}
```

> ❗ 할당 후 해제하지 않으면 **메모리 누수(memory leak)** 발생

---

### 17.2 malloc() 함수

#### 🧱 기본 사용법
```c
#include <stdlib.h>

int *x = (int *) malloc(10 * sizeof(int));
```

- 인자: 바이트 단위 크기 (`size_t`)
- 반환: 성공 시 할당된 메모리 주소, 실패 시 `NULL`
- 반환 타입은 `void *` → 필요시 캐스팅

#### 🧠 팁
- 항상 `sizeof` 연산자를 활용해 타입 크기 기반 할당
```c
double *d = (double *) malloc(sizeof(double));
```
- 문자열 할당 시는 `strlen(s) + 1` (널 문자 포함)
```c
char *dst = (char *) malloc(strlen(src) + 1);
```

> **주의**: `sizeof(x)`는 x의 타입이 포인터이면 **메모리 크기가 아니라 포인터 크기**를 반환함

---

### 17.3 free() 함수

```c
int *x = malloc(10 * sizeof(int));
free(x);  // 할당 해제
```

- 인자: `malloc()` 또는 `calloc()` 등이 반환한 포인터
- 크기는 전달하지 않음 — **메모리 라이브러리가 내부적으로 기억함**

---

### 17.4 흔한 오류

#### 🚨 1. 메모리 할당을 깜빡함
```c
char *dst;
strcpy(dst, src);  // Segmentation fault 가능
```

#### ✅ 해결
```c
char *dst = (char *) malloc(strlen(src) + 1);
strcpy(dst, src);
```

---

#### 🚨 2. 메모리를 **부족하게** 할당함
```c
char *dst = (char *) malloc(strlen(src));  // +1 누락
strcpy(dst, src);  // 버퍼 오버플로우 가능
```

→ 심각한 경우 보안 취약점으로 이어짐 (예: 코드 인젝션)

---

#### 🚨 3. 초기화 없이 사용
```c
int *arr = (int *) malloc(10 * sizeof(int));
printf("%d\n", arr[0]);  // 초기화되지 않은 값
```

→ 쓰레기 값 읽기 발생 가능

---

#### 🚨 4. 메모리 해제를 잊음 (Memory Leak)
```c
int *arr = malloc(100 * sizeof(int));
// free(arr);  ← 누락!
```

→ 짧은 실행 시간 프로그램에선 괜찮지만, **서버/OS에서는 치명적**

---

#### 🚨 5. 해제한 메모리를 다시 사용 (Dangling Pointer)
```c
int *x = malloc(sizeof(int));
free(x);
*x = 42;  // 위험! use-after-free
```

---

#### 🚨 6. 이중 해제 (Double Free)
```c
int *x = malloc(sizeof(int));
free(x);
free(x);  // Undefined Behavior
```

---

#### 🚨 7. 잘못된 포인터 해제
```c
int *x = malloc(10 * sizeof(int));
free(x + 5);  // 유효하지 않은 해제
```

---

> ✅ 해결책: **습관적으로 malloc 후 free를 쓸 것**. 할당 후 NULL로 초기화하면 오류 방지에 도움.

---

### 17.5 운영체제와의 관계

- `malloc()` / `free()`는 시스템 콜이 **아님**
- 메모리 라이브러리는 내부적으로 `brk()` 또는 `mmap()` 같은 시스템 콜 사용

#### 📌 brk / sbrk
- 힙 영역의 끝 지점을 조정
- 현재는 거의 사용되지 않음

#### 📌 mmap()
- 커널로부터 **익명(anonymous) 메모리 페이지**를 요청
- 대형 할당 요청 시 활용됨
- malloc 구현체는 내부적으로 둘 다 혼합 사용

> 직접 `brk`/`mmap`을 호출하는 건 매우 위험! 반드시 `malloc()` 계열 함수만 사용할 것

---

### 17.6 기타 메모리 함수

| 함수 | 설명 |
|------|------|
| `calloc(n, size)` | `n * size`만큼 공간을 할당하고 **0으로 초기화** |
| `realloc(ptr, new_size)` | 기존 포인터 `ptr`의 크기를 `new_size`로 재조정 |
| `strdup(str)` | 문자열 복사와 메모리 할당을 동시에 수행 |

> calloc은 **초기화를 잊는 오류 방지**, realloc은 **동적 배열 구현**에 유용

---

## ✅ 요약

운영체제는 사용자 프로그램이 메모리를 직접 다루지 않고, **간접적으로 요청/해제**하도록 설계했다. 이를 위한 대표적인 라이브러리 함수들이 다음과 같다:

- `malloc(size)`: 크기만큼의 메모리를 힙에서 할당
- `free(ptr)`: 할당된 메모리를 해제
- `calloc(n, size)`: 0으로 초기화된 메모리 할당
- `realloc(ptr, size)`: 기존 할당 크기를 조정

이 함수들은 단순해 보이지만, **잘못 사용할 경우 매우 위험하다.** 대표적인 오류는 다음과 같다:

- 메모리 해제 누락 → 누수
- 중복 해제 → 크래시
- 초기화 없이 사용 → 예측 불가 동작
- 잘못된 포인터 해제 → 시스템 오류

📌 좋은 프로그래머는 `malloc()`보다 `free()`를 더 많이 신경 쓴다.  
그리고 반드시 `valgrind`, `gdb` 같은 도구를 활용해 **누수/오류를 감지하고 수정**하는 습관을 들여야 한다.

---

> 📚 출처: 『운영체제: 아주 쉬운 세 가지 이야기 (OSTEP)』
