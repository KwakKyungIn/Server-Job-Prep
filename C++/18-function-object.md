# ⚙️ C++ 함수 객체 (Function Object / Functor)

> 함수 객체(Function Object)는 **함수처럼 동작하지만, 객체의 특성을 가지는 클래스**를 의미한다.  
> 연산자 오버로딩을 통해 `()` 호출 연산자를 정의하면, 객체를 함수처럼 호출할 수 있다.  
> STL, 커스텀 비교 함수, 커맨드 패턴 등 **유연하고 확장성 높은 함수 호출 구조**에 자주 사용된다.

---

## ❓ 함수 객체란?

```cpp
class MyFunctor {
public:
    void operator()() {
        std::cout << "함수 객체 호출됨!\n";
    }
};

MyFunctor f;
f();  // 함수처럼 호출 가능
```

- `operator()` 를 오버로딩한 클래스
- 객체지만 함수처럼 `()`로 호출 가능
- 내부 상태를 가질 수 있음 → **람다나 함수 포인터보다 표현력 우수**

---

## ✅ 함수 포인터 vs 함수 객체

| 항목 | 함수 포인터 | 함수 객체 (Functor) |
|------|--------------|---------------------|
| 구조 | 함수 주소 저장 | 객체 내부에 `()` 오버로딩 |
| 상태 저장 | 불가능 | 가능 (멤버 변수 사용 가능) |
| 커스터마이징 | 어려움 | 상속, 템플릿 등으로 다양화 |
| 표현력 | 제한적 | 높음 |
| 성능 | 빠름 | 약간 느릴 수 있음 (인라인 최적화로 무시됨) |

---

## 📦 함수 객체의 활용

### 1. 정렬 기준으로 사용 (STL)
```cpp
struct Player {
    std::string name;
    int level;
};

struct CompareByLevel {
    bool operator()(const Player& a, const Player& b) {
        return a.level > b.level; // 높은 레벨 우선
    }
};

std::vector<Player> players = { {"Alice",10}, {"Bob",20} };
std::sort(players.begin(), players.end(), CompareByLevel());
```

### 2. 커맨드 패턴 구현
```cpp
class ICommand {
public:
    virtual void operator()() = 0;
};

class AttackCommand : public ICommand {
public:
    void operator()() override {
        std::cout << "플레이어가 공격합니다!\n";
    }
};

std::unordered_map<std::string, ICommand*> commandMap;
commandMap["attack"] = new AttackCommand();

(*commandMap["attack"])();  // 플레이어가 공격합니다!
```

---

## 🎮 게임 서버 개발에서의 활용

| 사용 사례 | 설명 |
|-----------|------|
| 패킷 핸들러 객체화 | 상태를 가지는 핸들러 구현 가능 |
| 커맨드 테이블 | 명령 → Functor 매핑 가능 |
| 비동기 작업 큐 | Job Queue에 함수 객체 등록 |
| AI 행동 정의 | 객체 상태 기반 행동 함수화 가능 |

```cpp
class Job {
public:
    virtual void operator()() = 0;
};

class MoveJob : public Job {
    int x, y;
public:
    MoveJob(int _x, int _y) : x(_x), y(_y) {}
    void operator()() override {
        std::cout << "캐릭터가 이동합니다: " << x << ", " << y << "\n";
    }
};

std::queue<Job*> jobQueue;
jobQueue.push(new MoveJob(100, 200));

// 처리
(*jobQueue.front())();  // 캐릭터가 이동합니다: 100, 200
```

---

## 🧠 함수 객체의 장점

- ✅ **상태 유지** 가능 (내부 멤버 활용)
- ✅ **템플릿과 결합** 시 코드 재사용성 극대화
- ✅ **성능 최적화 가능** (인라인화)
- ✅ STL에서 **커스텀 비교, 필터링** 등에 최적
- ✅ **캡슐화**로 설계를 깔끔하게 분리 가능

---

## 🧪 함수 객체 + 람다

```cpp
auto filter = [](const Player& p) {
    return p.level > 50;
};
```

람다도 사실 내부적으로는 **함수 객체로 컴파일**된다.  
→ 함수 객체 = 람다 = `std::function` 의 뿌리 구조

---

## 🧠 마무리 요약

| 개념 | 설명 |
|------|------|
| 함수 객체 | `operator()`를 오버로딩한 객체 |
| 장점 | 상태 유지, 표현력 높음, STL과 연계 용이 |
| 게임 서버 사용처 | 커맨드 테이블, 비동기 작업, AI, 패킷 핸들러 |
| 대안 | 람다, std::function |

---

## 📋 면접 대비 포인트

- 함수 포인터와 함수 객체의 차이점?
- 함수 객체는 언제 쓰고, 왜 더 표현력이 높은가?
- 게임 서버에서 명령 처리, 비동기 작업 시 어떤 방식이 적합한가?
- 함수 객체에 상태를 담는다면, 어떤 장점이 있을까?

---

> 🎯 함수 객체는 게임 서버 아키텍처의 **핸들링, 디커플링, 추상화**에 큰 장점을 준다.  
> 특히 상태 기반 처리나 객체 지향 설계에서 함수 포인터보다 더 강력한 표현력을 발휘한다.
