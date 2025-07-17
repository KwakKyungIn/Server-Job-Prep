# 📦 동적 메모리 할당 기초

> C++에서 메모리 관리는 매우 중요한 요소이며, 서버 프로그래밍처럼 안정성과 효율성이 중요한 분야에서는 더욱 중요합니다.  
> 이 문서에서는 메모리 구조와 함께 C/C++에서 사용하는 동적 메모리 할당 기법들을 자세히 정리합니다.

---

## 🧠 메모리 구조

프로그램이 실행되면 운영체제는 아래와 같은 **메모리 공간**을 제공합니다:

| 영역 | 설명 |
|------|------|
| 코드(Code) 영역 | 실제 실행할 프로그램의 **기계어 코드**가 저장됩니다. |
| 데이터(Data) 영역 | 초기화된 **전역변수**와 **정적 변수**가 저장됩니다. |
| BSS 영역 | 초기화되지 않은 **전역변수/정적 변수**가 저장됩니다. |
| 힙(Heap) 영역 | **동적 할당**으로 관리되는 영역입니다. malloc/new 사용 시 이 영역 사용 |
| 스택(Stack) 영역 | **지역 변수**, **매개변수**, **함수 호출 스택 프레임** 등이 저장됩니다. 함수 호출 시마다 쌓이고 반환 시 제거됩니다. |

---

## 📌 스택과 힙의 차이

| 구분 | 스택 | 힙 |
|------|------|----|
| 할당 방식 | 컴파일 타임 | 런타임 |
| 관리 주체 | 컴파일러 | 프로그래머 (혹은 스마트 포인터) |
| 생명 주기 | 함수 호출 시 생성, 종료 시 소멸 | 명시적으로 해제해야 함 |
| 장점 | 빠름, 자동 관리 | 크기 유연, 자유로운 생명주기 |
| 단점 | 크기 제한, 생명주기 짧음 | 속도 느림, 메모리 누수 위험 |

---

## ✨ 동적 메모리 할당 함수 및 연산자

### 🔹 `malloc()`

- 힙에서 메모리 블록을 **바이트 단위로 요청**하여 할당합니다.
- 반환값은 `void*` → 적절한 포인터 타입으로 **형변환** 필요
- 할당 실패 시 `NULL` 반환

```cpp
int* arr = (int*)malloc(sizeof(int) * 10); // int 10개 할당
```

### 🔹 `free()`

- `malloc()`/`calloc()`/`realloc()`으로 할당한 메모리를 **해제**합니다.
- 해제 후 포인터는 **사용 금지** (dangling pointer 가능성)

```cpp
free(arr);
arr = nullptr; // dangling 방지
```

### 🔹 `new` / `delete` (C++ 전용)

```cpp
int* a = new int(5);      // 정수 1개 동적할당
int* arr = new int[100];  // 배열 할당

delete a;
delete[] arr;
```

- `new`는 타입 명시 가능, 생성자 호출 가능, 실패 시 예외 throw
- `delete`는 `delete[]`와 구분해서 사용해야 함

---

## ⚠️ 메모리 관련 주요 버그

### ❗ Memory Leak (메모리 누수)
- `malloc()`이나 `new`로 할당한 메모리를 `free()` 또는 `delete` 하지 않음
- 반복적으로 발생 시 **힙 고갈 → 프로그램 종료**

### ❗ Double Free
- 같은 메모리를 두 번 해제 → 대부분 **크래시 발생**

```cpp
int* p = (int*)malloc(4);
free(p);
free(p); // 위험
```

### ❗ Use After Free
- 이미 해제한 메모리를 다시 접근 → **정의되지 않은 동작**, **보안 취약점** 발생

```cpp
int* p = new int(42);
delete p;
cout << *p; // 위험!
```

---

## 📝 참고

- malloc/free는 C 스타일, new/delete는 C++ 스타일
- 스마트 포인터 (unique_ptr, shared_ptr)를 사용하면 자동으로 메모리 관리 가능

```cpp
#include <memory>
std::unique_ptr<int> ptr = std::make_unique<int>(10);
```

# 🔍 힙 메모리 관리 방식과 CRT 동작

---

## 🧰 C 런타임(CRT)과 힙 관리자

C++ 표준 라이브러리는 힙 할당을 직접 운영체제가 아닌 **CRT(C Runtime Library)**를 통해 관리합니다.

- CRT는 내부적으로 운영체제의 `HeapAlloc`, `HeapFree` 또는 `VirtualAlloc` 등을 사용하여 메모리를 관리
- 동적 메모리의 상태를 추적하여 **중복 해제, 미해제 등을 감지**하기도 함 (Debug 모드에서)

---

## 📌 void 포인터 사용 이유

### `malloc()`의 반환형은 `void*`

- **타입이 지정되지 않은 메모리 주소**이기 때문
- 사용 시에는 적절한 타입으로 **캐스팅 필요**

```cpp
int* data = (int*)malloc(sizeof(int) * 50);
```

### C++에서는 타입 안정성 위해 new 사용 권장

---

## 💥 흔한 실수 예시

### ❌ delete로 malloc 할당 해제

```cpp
int* p = (int*)malloc(4);
delete p;  // ❌ 절대 안됨
```

### ❌ malloc과 delete[] 혼용

```cpp
int* p = new int[10];
free(p);  // ❌ undefined behavior
```

- `malloc`은 `free`로 해제해야 하고  
- `new`는 `delete`, `new[]`는 `delete[]`로 해제해야 함

---

## 🧪 메모리 디버깅 팁

- Visual Studio에서는 `_CrtDumpMemoryLeaks()` 를 사용해 **누수 추적**
- Valgrind(리눅스) 등의 툴로 힙 상태 추적 가능

---

## ✅ 요약

| 구분 | 설명 |
|------|------|
| `malloc/free` | C 스타일 동적 할당 |
| `new/delete` | C++ 스타일 동적 할당 (생성자/소멸자 호출됨) |
| void 포인터 | 타입 미지정 → 캐스팅 필요 |
| 스마트 포인터 | 메모리 자동 해제 (RAII 패턴) |
| 주요 버그 | 누수, double free, use-after-free 등 |

---
