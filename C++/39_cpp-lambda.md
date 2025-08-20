# 🌱 C++ 람다(lambda) 정리

> **람다(lambda)** 는 C++11부터 도입된 기능으로, **익명 함수(이름 없는 함수)** 를 만들 때 사용한다.  
> 서버 프로그래밍에서 콜백, 스레드 함수 전달, STL 알고리즘에서 간단한 조건 지정 등 **간결한 코드**를 작성할 때 유용하다.

---

## 1. 기본 문법

```cpp
[capture](parameters) -> return_type {
    // 함수 본문
}
````

* **`[]` (캡처 리스트)** : 외부 변수를 람다 안으로 끌어오는 방법 정의
* **`(parameters)`** : 함수의 매개변수
* **`-> return_type`** : 반환 타입(생략 가능, 자동 추론됨)
* **본문 `{}`** : 함수 내용

예시:

```cpp
auto f = []() {
    std::cout << "Hello Lambda" << std::endl;
};
f(); // 실행
```

---

## 2. 캡처 방식

람다에서 **외부 변수 사용 방법**을 지정할 수 있다.

```cpp
int x = 10, y = 20;

// 값 캡처 (복사)
auto f1 = [x]() { std::cout << x << std::endl; };

// 참조 캡처 (레퍼런스)
auto f2 = [&y]() { std::cout << y << std::endl; };

// 모든 변수 값 캡처
auto f3 = [=]() { std::cout << x + y << std::endl; };

// 모든 변수 참조 캡처
auto f4 = [&]() { x = 100; y = 200; };

// 혼합 캡처
auto f5 = [=, &y]() { std::cout << x << ", " << y << std::endl; };
```

---

## 3. 반환 타입 지정

C++은 보통 **반환 타입을 추론**하지만, 필요하다면 `->` 로 직접 지정할 수 있다.

```cpp
auto add = [](int a, int b) -> int {
    return a + b;
};
```

---

## 4. STL 알고리즘과 함께 사용

람다는 특히 **STL 알고리즘**에서 조건을 간결하게 표현할 때 자주 사용된다.

```cpp
std::vector<int> v = {1, 2, 3, 4, 5};

// 특정 조건 찾기
auto it = std::find_if(v.begin(), v.end(), [](int x) {
    return x > 3;
});
if (it != v.end()) std::cout << "Found: " << *it << std::endl;

// for_each 사용
std::for_each(v.begin(), v.end(), [](int& x) {
    x *= 2;
});
```

---

## 5. 함수 객체/함수 포인터와 비교

* **함수 포인터**: 단순히 함수 주소만 저장 가능 → 상태를 가질 수 없음
* **함수 객체(Functor)**: 연산자 오버로딩으로 상태를 가질 수 있음
* **람다**: **상태 캡처 가능 + 익명성** 덕분에 더 간단하고 강력

예시 (콜백에서 유용):

```cpp
void RunTask(std::function<void()> f) {
    f();
}

int counter = 0;
RunTask([&]() { counter++; }); // 람다로 콜백 전달
```

---

## 6. 서버 프로그래밍에서의 활용

* **스레드 생성 시**:

  ```cpp
  std::thread t([]() {
      std::cout << "Thread running" << std::endl;
  });
  t.join();
  ```

* **비동기 작업 처리 (future, async)**:

  ```cpp
  auto fut = std::async(std::launch::async, []() {
      return 42;
  });
  std::cout << fut.get() << std::endl;
  ```

* **네트워크 이벤트 핸들러/콜백 등록**에 활용 → 간결하고 가독성 높음

---

## ✅ 정리

* 람다는 **익명 함수**를 만드는 도구.
* 캡처 리스트(`[]`) 로 외부 변수 활용 가능 (값/참조/혼합).
* 반환 타입은 보통 추론되지만, 필요 시 `->` 로 명시 가능.
* STL 알고리즘, 멀티스레드, 콜백 등에서 매우 자주 쓰임.
* 함수 포인터, 함수 객체와 비교했을 때 **간결성 + 유연성**이 장점.

