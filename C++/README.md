# ⚙️ C++ 정리

> C++ 핵심 문법, 메모리 구조, 객체지향, 스마트 포인터, 멀티스레딩 등을 정리한 공간입니다.
> 문법 중심의 이론 정리와 실제 프로젝트 기반 실습 내용을 **이원화**하여 구성합니다.

---

## 📚 이론 정리 (C++ Language Concepts)

### 1. 포인터 & 메모리

| 주제              | 설명                                         | 링크                                                                          |
| --------------- | ------------------------------------------ | --------------------------------------------------------------------------- |
| 포인터 기초          | 포인터 선언, 역참조, 주소값 다루기                       | [01\_pointer-basics.md](./01_pointer-basics.md)                             |
| 포인터 연산          | +, -, 비교, 포인터 간 거리 계산                      | [02\_pointer-arithmetic.md](./02_pointer-arithmetic.md)                     |
| 참조 vs 포인터       | 참조 개념과 포인터와의 차이                            | [03\_reference-vs-pointer.md](./03_reference-vs-pointer.md)                 |
| 배열 기초           | 배열 선언, 초기화, 순회 방법                          | [04\_array-basics.md](./04_array-basics.md)                                 |
| 포인터 vs 배열       | 메모리 구조 차이, `sizeof` 비교                     | [05\_pointer-vs-array.md](./05_pointer-vs-array.md)                         |
| 다중 포인터 & 다차원 배열 | 2차원 배열, 포인터 배열, `int**` 차이                 | [06\_multi-pointer-multidim-array.md](./06_multi-pointer-multidim-array.md) |
| 동적 메모리          | new/delete, malloc/free, 메모리 누수 방지         | [12-dynamic-memory-basics.md](./12-dynamic-memory-basics.md)                |
| 타입 변환           | 값 변환 vs 참조 변환, 암시적/명시적 변환                  | [13\_type\_conversion.md](./13_type_conversion.md)                          |
| 포인터 캐스팅         | `static_cast`, `reinterpret_cast` 등 포인터 변환 | [14\_type\_conversion\_pointer.md](./14_type_conversion_pointer.md)         |
| 얕은 복사 vs 깊은 복사  | 복사 생성자, 대입 연산자                             | [15\_cpp-copy-shallow-deep.md](./15_cpp-copy-shallow-deep.md)               |
| 스마트 포인터         | `unique_ptr`, `shared_ptr`, `weak_ptr`     | [41\_SmartPointer.md](./41_SmartPointer.md)                                 |

---

### 2. 객체지향 (OOP)

| 주제         | 설명                                 | 링크                                                                         |
| ---------- | ---------------------------------- | -------------------------------------------------------------------------- |
| OOP 기초     | 클래스, 생성자/소멸자, 접근 제어자               | [07\_cpp\_oop\_basics.md](./07_cpp_oop_basics.md)                          |
| 상속 & 캡슐화   | 클래스 상속, 캡슐화 개념 정리                  | [08\_cpp\_oop\_inheritance\_encap.md](./08_cpp_oop_inheritance_encap.md)   |
| 다형성        | 가상 함수, 오버라이딩, vtable               | [09\_cpp\_oop\_polymorphism.md](./09_cpp_oop_polymorphism.md)              |
| 초기화 리스트    | 멤버 초기화, `explicit` 생성자             | [10\_cpp\_cpplistinitializer-list.md](./10_cpp_cpplistinitializer-list.md) |
| 연산자 오버로딩   | 산술/비교/대입 연산자 정의                    | [11\_cpp\_operator-overloading.md](./11_cpp_operator-overloading.md)       |
| static 키워드 | static 변수, static 함수, 클래스 내 static | [11\_static-struct-class.md](./11_static-struct-class.md)                  |

---

### 3. 함수 & 템플릿

| 주제                 | 설명                          | 링크                                                        |
| ------------------ | --------------------------- | --------------------------------------------------------- |
| 함수 포인터             | 함수 포인터 선언, 콜백 구현            | [17-function-pointer.md](./17-function-pointer.md)        |
| 함수 객체 (Functor)    | functor 정의와 활용, 서버 개발 관점 예제 | [18-function-object.md](./18-function-object.md)          |
| 템플릿 기초             | 함수 템플릿, 클래스 템플릿, 특수화        | [19-templates-basics.md](./19-templates-basics.md)        |
| 콜백 함수              | 함수 포인터 vs 람다 기반 콜백          | [20-callbacks.md](./20-callbacks.md)                      |
| Lambda             | 람다 기본 문법, 캡처 방식             | [39\_cpp-lambda.md](./39_cpp-lambda.md)                   |
| Lambda vs Callback | 콜백 함수와 람다 비교                | [40\_cpp-lambda-callback.md](./40_cpp-lambda-callback.md) |

---

### 4. STL (Standard Template Library)

| 주제                  | 설명                                      | 링크                                                         |
| ------------------- | --------------------------------------- | ---------------------------------------------------------- |
| Vector              | 벡터 원리, 메모리 확장 방식                        | [21-stl-vector.md](./21-stl-vector.md)                     |
| Vector Iterator     | 반복자 개념, 범위 기반 for, iterator 사용          | [22-stl-vector-iterator.md](./22-stl-vector-iterator.md)   |
| List                | 이중 연결 리스트 구현 및 사용                       | [23-stl-list.md](./23-stl-list.md)                         |
| Deque               | 양방향 큐 자료구조, 내부 구조                       | [24-stl-deque.md](./24-stl-deque.md)                       |
| Map                 | key-value 매핑, 정렬 원리                     | [25-stl-map.md](./25-stl-map.md)                           |
| Set                 | 중복 없는 정렬된 집합                            | [26-stl-set.md](./26-stl-set.md)                           |
| MultiSet & MultiMap | 중복 허용 set/map                           | [27-stl-multi-set-map.md](./27-stl-multi-set-map.md)       |
| STL Algorithms      | `find`, `count`, `remove`, `for_each` 등 | [28-stl-algorithms-basic.md](./28-stl-algorithms-basic.md) |

---

### 5. 동시성 (Concurrency)

| 주제                   | 설명                                      | 링크                                                                          |
| -------------------- | --------------------------------------- | --------------------------------------------------------------------------- |
| 스레드 기초               | `std::thread`, join, detach             | [29\_thread\_basic.md](./29_thread_basic.md)                                |
| 원자적 연산               | `std::atomic`, 메모리 모델                   | [30\_atomic.md](./30_atomic.md)                                             |
| 락 (Lock & LockGuard) | 뮤텍스, RAII 기반 락                          | [31\_lock.md](./31_lock.md)                                                 |
| 데드락                  | 교착 상태, 예방 기법                            | [32\_deadlock-basics.md](./32_deadlock-basics.md)                           |
| 스핀락                  | busy-wait 기반 락                          | [33-spinlock.md](./33-spinlock.md)                                          |
| Sleep                | `std::this_thread::sleep_for`, sleep 이슈 | [34-sleep.md](./34-sleep.md)                                                |
| Event                | 조건 충족 시 동기화 이벤트 처리                      | [35-event.md](./35-event.md)                                                |
| Condition Variable   | 조건 변수, `notify_one`, `notify_all`       | [36-condition-variable.md](./36-condition-variable.md)                      |
| Future & Promise     | 비동기 작업, packaged\_task                  | [37-future-promise-packaged\_task.md](./37-future-promise-packaged_task.md) |

---

### 6. 시스템 프로그래밍 (System Level)

| 주제                 | 설명                                                              | 링크                                                                         |
| ------------------ | --------------------------------------------------------------- | -------------------------------------------------------------------------- |
| 캐스팅 연산자            | `static_cast`, `dynamic_cast`, `const_cast`, `reinterpret_cast` | [16\_casting\_operators.md](./16_casting_operators.md)                     |
| CPU 파이프라인 & 메모리 모델 | CPU 동작, 메모리 계층 구조                                               | [38cpu-pipeline-and-memory-model.md](./38cpu-pipeline-and-memory-model.md) |

---

## 🧪 실습 정리 (C++ Projects & Practice)

| 프로젝트             | 설명                        | 링크                                                                                |
| ---------------- | ------------------------- | --------------------------------------------------------------------------------- |
| IOCP Mini Server | Windows IOCP 기반 비동기 서버 구현 | 🔗 [IOCP-GameServer](https://github.com/KwakKyungIn/IOCP-GameServer)              |
| C++ Practice     | STL, 템플릿, 람다 등을 활용한 실습 코드 | 🔗 [CPlusPlus-Practice](https://github.com/KwakKyungIn/CPlusPlus-Practice) *(예정)* |

---


## ✍️ 사용법 및 정리 기준

* 각 문서는 다음 구조로 작성됩니다:

  1. **핵심 문법 설명**
  2. **예제 코드**
  3. **주의할 점 & 요약 정리**

* 실전에서 많이 쓰이는 문법 위주로 작성하며,
  실습 프로젝트와 연계될 수 있도록 구성합니다.

---

> 📌 목표:
> 이론은 확실히 이해하고, 실습은 진짜 서버 코드에서 써먹는 것.
> 포인터, 참조, 메모리, 동기화 등 **하드코어한 시스템 프로그래밍 C++ 실력을 쌓기 위한 기록입니다.**

---

