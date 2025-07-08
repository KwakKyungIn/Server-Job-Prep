# ⚙️ C++ 정리

> C++ 핵심 문법, 메모리 구조, 객체지향, 스마트 포인터, 멀티스레딩 등을 정리한 공간입니다.  
> 문법 중심의 이론 정리와 실제 프로젝트 기반 실습 내용을 **이원화**하여 구성합니다.

---

## 📚 이론 정리 (C++ Language Concepts)

| 구분 | 주제 | 설명 | 링크 |
|------|------|------|------|
| ✅ | 포인터 기초 | 포인터 선언, 역참조, 연산 | [01_pointer-basics.md](./01_pointer-basics.md) |
| ✅ | 포인터 연산 | +, -, 비교, 거리 계산 | [02_pointer-arithmetic.md](./02_pointer-arithmetic.md) |
| ✅ | 참조 vs 포인터 | 참조의 개념과 포인터와의 작동 비교 | [03_reference-vs-pointer.md](./03_reference-vs-pointer.md) |
| ✅ | 배열 기초 | 배열 선언, 초기화, 순회 | [04_array-basics.md](./04_array-basics.md) |
| ✅ | 포인터 vs 배열 | 메모리 구조 차이, sizeof 비교 | [05_pointer-vs-array.md](./05_pointer-vs-array.md) |
| ✅ | 다중 포인터 & 다차원 배열 | 2차원 배열, 포인터 배열, `int**` 차이 | [06_multi-pointer-multidim-array.md](./06_multi-pointer-multidim-array.md) |
| ⏳ | const 키워드 | const 변수, 포인터, 참조에서의 활용 | *(작성 예정)* |
| ⏳ | 함수 인자 전달 | 값/참조/포인터 전달 방식의 차이 | *(작성 예정)* |
| ⏳ | 동적 메모리 | new/delete, 메모리 누수 방지 | *(작성 예정)* |
| ⏳ | 스마트 포인터 | unique_ptr, shared_ptr, weak_ptr | *(작성 예정)* |
| ⏳ | 클래스 & 객체 | 생성자, 소멸자, 캡슐화, 접근 제어자 | *(작성 예정)* |
| ⏳ | 연산자 오버로딩 | 연산자 함수 정의, 멤버 vs 전역 함수 | *(작성 예정)* |
| ⏳ | 템플릿 | 함수 템플릿, 클래스 템플릿, 특수화 | *(작성 예정)* |
| ⏳ | STL | vector, map, set, algorithm 기본기 | *(작성 예정)* |
| ⏳ | 예외 처리 | try-catch, noexcept, throw 예제 | *(작성 예정)* |
| ⏳ | 멀티스레딩 | std::thread, mutex, lock, atomic | *(작성 예정)* |

---

## 🧪 실습 정리 (C++ Projects & Practice)

> 실습은 **별도 레포지토리**에 구성되어 있습니다.  
> IOCP 서버, 동기/비동기 네트워크 모델, 알고리즘 문제풀이 등으로 확장 예정입니다.

| 프로젝트 | 설명 | 링크 |
|----------|------|------|
| IOCP Mini Server | Windows IOCP 기반 비동기 서버 구현 | 🔗 [IOCP-GameServer](https://github.com/KwakKyungIn/IOCP-GameServer) |
| C++ Practice | STL, 템플릿, 람다 등을 활용한 실습 코드 | 🔗 [CPlusPlus-Practice](https://github.com/KwakKyungIn/CPlusPlus-Practice) *(예정)* |

---

## ✍️ 사용법 및 정리 기준

- 각 문서는 다음 구조로 작성됩니다:
  1. **핵심 문법 설명**
  2. **예제 코드**
  3. **주의할 점 & 요약 정리**

- 실전에서 많이 쓰이는 문법 위주로 작성하며,  
  실습 프로젝트와 연계될 수 있도록 구성합니다.

---

> 📌 목표:  
> 이론은 확실히 이해하고, 실습은 진짜 서버 코드에서 써먹는 것.  
> 포인터, 참조, 메모리, 동기화 등 **하드코어한 시스템 프로그래밍 C++ 실력을 쌓기 위한 기록입니다.**
