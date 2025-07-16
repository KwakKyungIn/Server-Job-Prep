# 🎯 C++ 객체지향 정리 Part 3 - **다형성 중심**

이 문서는 C++ 객체지향 프로그래밍의 핵심 주제인 **다형성(polymorphism)**을 중심으로 정리한 내용입니다.  
서버 프로그래머를 지망하는 개발자에게 필수적인 개념인 **가상 함수, 동적 바인딩, 추상 클래스** 등을 실제 코드 예시와 함께 설명합니다.

---

## 🧩 다형성이란?

다형성이란 **동일한 인터페이스를 통해 다양한 동작을 수행**할 수 있도록 하는 객체지향의 핵심 개념입니다.

C++에서 다형성은 크게 두 가지로 나뉩니다.

| 유형 | 설명 | 키워드 |
|------|------|--------|
| 정적 다형성 | 컴파일 타임에 결정 | 함수 오버로딩, 템플릿 |
| 동적 다형성 | 런타임에 결정 | 가상 함수 (`virtual`) |

---

## ✅ 함수 오버로딩 (정적 다형성의 예)

같은 이름의 함수라도 **매개변수의 수나 타입이 다르면** 컴파일러가 구분할 수 있습니다.  
→ 정적 바인딩, 빠르지만 런타임 유연성은 낮음

### 💡 예시: 다양한 로그 출력 함수
```cpp
#include <iostream>
#include <string>

void logMessage(const std::string& msg) {
    std::cout << "[INFO] " << msg << std::endl;
}

void logMessage(const std::string& msg, int code) {
    std::cout << "[INFO] (" << code << ") " << msg << std::endl;
}
```

---

## ✅ 함수 오버라이딩 (동적 다형성의 핵심)

**부모 클래스의 가상 함수를 자식 클래스에서 재정의**하여  
객체의 실제 타입에 따라 실행 결과가 달라지게 만드는 것.

> `virtual` 키워드를 사용하며, 동적 바인딩을 가능하게 함

### 💡 예시: 이벤트 핸들러 다형성
```cpp
class EventHandler {
public:
    virtual void handle() {
        std::cout << "Default event handler" << std::endl;
    }
};

class LoginHandler : public EventHandler {
public:
    void handle() override {
        std::cout << "Handling login event" << std::endl;
    }
};
```

---

## 🧠 이름 재사용 (Name Hiding) - 간단 언급

C++에서 **자식 클래스가 부모 클래스와 같은 이름의 함수를 새로 정의하면**,  
**부모 클래스의 모든 같은 이름 함수가 숨겨짐** (오버라이딩과 비슷해 보이지만 다름)

### ⚠️ 주의: 오버라이딩과는 다름
- **오버라이딩**은 virtual 함수의 재정의 (동적 바인딩 O)
- **이름 재사용**은 그냥 스코프에 따라 가려지는 것 (바인딩과 무관)

```cpp
class Server {
public:
    void setConfig(int port) {
        std::cout << "Set port: " << port << std::endl;
    }
};

class SecureServer : public Server {
public:
    void setConfig(const std::string& cert) {
        std::cout << "Set cert: " << cert << std::endl;
    }
    // Server::setConfig(int)는 숨겨짐
};
```

👉 `using Server::setConfig;` 으로 부모 함수 다시 노출 가능

---

## 🔁 정적 바인딩 vs 동적 바인딩

### ✅ 정적 바인딩 (Static Binding)

- 컴파일 타임에 함수 호출 결정
- **일반 함수**, **비가상 함수** 사용 시

```cpp
class Server {
public:
    void status() {
        std::cout << "Server is running" << std::endl;
    }
};
```

### ✅ 동적 바인딩 (Dynamic Binding)

- 런타임 시점에 실제 객체 타입을 기준으로 호출
- **가상 함수 (`virtual`) + 포인터/참조**로 접근할 때 발생
- 진짜 다형성이 필요한 순간에 사용

### 💡 예시: 다양한 패킷 처리 방식
```cpp
class Packet {
public:
    virtual void process() {
        std::cout << "Processing generic packet" << std::endl;
    }
};

class AuthPacket : public Packet {
public:
    void process() override {
        std::cout << "Processing auth packet" << std::endl;
    }
};

void handlePacket(Packet* p) {
    p->process();  // 동적 바인딩으로 실제 타입의 process() 호출
}
```

---

## 🧰 가상 함수 테이블 (vtable)

가상 함수가 있는 클래스는 **vtable (virtual function table)** 이라는  
**함수 포인터 테이블**을 내부적으로 생성함.

- 각 클래스마다 vtable이 존재
- 객체는 `vptr`이라는 포인터를 통해 자신의 vtable을 참조
- **동적 바인딩 시 실제 호출 함수는 vtable을 통해 결정**

💡 대부분 컴파일러가 자동으로 처리해주므로 사용자는 `virtual`만 잘 사용하면 됨

---

## 🧩 순수 가상 함수 & 추상 클래스

### ✅ 순수 가상 함수 (Pure Virtual Function)
```cpp
virtual void connect() = 0;
```
- **구현이 없는 가상 함수**
- 자식 클래스가 반드시 재정의해야 함

---

### ✅ 추상 클래스 (Abstract Class)

- **순수 가상 함수 1개 이상** 포함
- **직접 객체 생성 불가**
- 인터페이스 용도로 많이 사용

### 💡 예시: 네트워크 모듈 인터페이스
```cpp
class INetworkModule {
public:
    virtual void connect(const std::string& ip) = 0;
    virtual void disconnect() = 0;
    virtual ~INetworkModule() {}
};

class TcpModule : public INetworkModule {
public:
    void connect(const std::string& ip) override {
        std::cout << "Connecting to " << ip << " via TCP" << std::endl;
    }

    void disconnect() override {
        std::cout << "TCP disconnected" << std::endl;
    }
};
```

---

## ✅ 다형성 요약표

| 개념 | 설명 | 바인딩 시점 | 키워드 | 특징 |
|------|------|--------------|--------|-------|
| 오버로딩 | 같은 이름, 다른 인자 | 컴파일 시 | 없음 | 빠르고 편리함 |
| 오버라이딩 | 부모 함수 재정의 | 런타임 | `virtual` | 다형성 구현 |
| 이름 재사용 | 같은 이름 새 정의 | 컴파일 시 | 없음 | 오버라이딩 아님 |
| 정적 바인딩 | 함수 고정 호출 | 컴파일 시 | 없음 | 빠름 |
| 동적 바인딩 | 실제 타입 따라 호출 | 런타임 | `virtual` | 유연함 |
| 가상 함수 | 동적 바인딩 대상 | 런타임 | `virtual` | vtable 사용 |
| 순수 가상 함수 | 구현 없는 virtual | 런타임 | `= 0` | 추상 클래스 구성 |
| 추상 클래스 | 객체 생성 불가 | 런타임 | 없음 | 인터페이스용 |

---

## 🔚 마무리

- C++에서 다형성은 코드의 유연성과 확장성에 핵심입니다.
- 서버 개발에서는 **패킷 처리**, **이벤트 핸들러**, **모듈 구조 설계** 등에서 매우 유용하게 쓰입니다.
- `virtual`, `override`, 추상 클래스 등을 자연스럽게 사용할 수 있어야 **객체지향적인 설계**가 가능합니다.

> 다음으로는 템플릿 기반 다형성과 인터페이스 분리 원칙(SOLID 원칙)도 정리해볼 예정입니다.

