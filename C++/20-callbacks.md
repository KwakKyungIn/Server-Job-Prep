# 🔁 C++에서의 콜백 함수 (Callback Function)

> 콜백 함수는 **"특정 이벤트가 발생했을 때 실행될 함수"** 를 **다른 코드에 전달하는 구조**이다.  
> C++에서는 **함수 포인터**, **함수 객체**, **템플릿 기반 함수 객체**, **람다** 등 다양한 방식으로 구현할 수 있다.  
> 게임 서버 개발에서는 **비동기 이벤트 처리**, **네트워크 패킷 핸들러 등록**, **작업 큐 처리** 등에 자주 쓰인다.

---

## ❓ 콜백 함수란?

- 어떤 동작이 끝났을 때 **알림용 함수 또는 후속 작업 함수**를 등록하고 호출하는 방식
- 호출자는 함수를 **직접 호출하지 않고**, 등록된 콜백을 실행시킴
- 함수 자체를 **인자로 전달**하거나, 나중에 **지정한 조건이 충족되면 호출**

---

## 📦 콜백 구현 방법들

### 1. 함수 포인터를 이용한 콜백

```cpp
void OnPlayerConnected() {
    std::cout << "플레이어가 접속했습니다!\n";
}

void RegisterCallback(void (*callback)()) {
    // 이벤트가 발생했다고 가정하고 호출
    callback();
}

int main() {
    RegisterCallback(OnPlayerConnected);
}
```

- ✅ 가장 전통적인 방식
- ❌ 상태 정보 전달 불가, 유연성이 낮음

---

### 2. 함수 객체 (Functor) 기반 콜백

```cpp
class OnPlayerDisconnect {
public:
    void operator()() {
        std::cout << "플레이어가 연결 종료했습니다.\n";
    }
};

template<typename Callback>
void RegisterEvent(Callback cb) {
    cb();  // 함수처럼 호출 가능
}

int main() {
    RegisterEvent(OnPlayerDisconnect());
}
```

- ✅ 상태 저장 가능
- ✅ 템플릿과 결합 시 유연성 극대화
- ✅ STL 스타일과 궁합 좋음

---

### 3. 템플릿 기반 다형 콜백 처리

```cpp
template<typename Callback>
class EventHandler {
private:
    Callback callback;
public:
    EventHandler(Callback cb) : callback(cb) {}
    void Trigger() { callback(); }
};

int main() {
    EventHandler handler([]() {
        std::cout << "게임 시작 처리!\n";
    });

    handler.Trigger();
}
```

- ✅ 다양한 함수형 인터페이스 수용
- ✅ 람다, 함수 객체, 함수 포인터 모두 지원
- ✅ C++의 함수형 프로그래밍 스타일 구현 가능

---

## 🧠 콜백과 함수 포인터, 함수 객체의 연결

| 방식 | 설명 | 장점 | 단점 |
|------|------|------|------|
| 함수 포인터 | `void (*f)()` 형태 | 간단함 | 상태 유지 불가 |
| 함수 객체 | `operator()` 구현 객체 | 상태 저장, 유연성 | 다소 복잡 |
| 람다 | 익명 함수 객체 | 간결함, 캡처 가능 | 복잡한 재사용에는 부적합 |
| 템플릿 | 콜백의 타입 추론 처리 | 유연함 | 에러 메시지 난해할 수 있음 |

---

## 🎮 게임 서버 예시: Job Queue + 콜백

```cpp
class Job {
public:
    virtual void operator()() = 0;
    virtual ~Job() {}
};

class SendWelcomePacket : public Job {
public:
    void operator()() override {
        std::cout << "환영 패킷 전송!\n";
    }
};

std::queue<Job*> jobQueue;
jobQueue.push(new SendWelcomePacket());

(*jobQueue.front())();  // 실행
```

> **콜백 객체를 큐에 등록하고, 나중에 꺼내서 호출하는 방식**  
> → IOCP의 CompletionPort 처리, AI 스크립트 처리 등에서 활용

---

## ✅ 요약

| 개념 | 설명 |
|------|------|
| 콜백 함수 | "나중에 실행할 함수"를 인자로 전달하는 방식 |
| 구현 방식 | 함수 포인터, 함수 객체, 템플릿 + 람다 |
| 장점 | 유연한 이벤트 기반 설계 |
| 서버 활용 | 비동기 네트워크, 이벤트 등록, 커맨드 시스템 등 |

---

## 💡 면접 포인트

- 함수 포인터와 함수 객체의 차이점은?
- 템플릿을 이용해 콜백 인터페이스를 만드는 이유는?
- 게임 서버에서 콜백 구조가 필요한 이유는?
- C++11 이후 `std::function`, `std::bind`, 람다 표현식과의 관계는?

---

> 🧭 C++에서의 콜백은 단순한 호출 이상의 의미를 가진다.  
> **의존성 역전, 느슨한 결합, 유연한 아키텍처 설계**로 이어지는 개념이므로 확실히 익혀둘 것!
