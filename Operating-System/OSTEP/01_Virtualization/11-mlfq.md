# 🌀 스케줄링: 멀티 레벨 피드백 큐 (OSTEP Chapter 11)

**MLFQ(Multilevel Feedback Queue)**는 현대 운영체제에서 널리 쓰이는 강력한 스케줄링 전략이다. 이 기법은 **대화형(interactive)** 작업에 빠른 응답을 제공하면서도, 긴 작업(long job)도 공정하게 실행되도록 설계되었다. 실행 시간 정보를 모른다는 제약 속에서 **작업의 과거 행동을 관찰하여 미래를 예측**하는 방식으로 우선순위를 조정한다.

---

## ❓ 핵심 질문

> **작업 실행 시간 정보 없이**, 반환 시간과 응답 시간을 동시에 최소화하려면 어떤 스케줄링 전략을 사용해야 할까?

---

## 🔍 상세 개념 정리

### 11.1 MLFQ 기본 규칙

MLFQ는 여러 개의 **우선순위 큐**로 구성되며, 각 큐는 고유한 **타임 슬라이스(time slice)**를 갖는다. 높은 우선순위 큐일수록 더 짧은 타임 슬라이스를 가진다.

#### ✅ 핵심 규칙

1. **규칙 1**: 우선순위(A) > 우선순위(B) → A 실행
2. **규칙 2**: 우선순위(A) = 우선순위(B) → RR 방식으로 실행

#### 🎯 예시
- A와 B: 높은 우선순위
- C: 중간 우선순위
- D: 낮은 우선순위

→ A와 B가 번갈아 실행되고, C와 D는 대기 (기아 발생 가능)

---

### 11.2 시도 1: 우선순위의 동적 변경

MLFQ는 **정적인 우선순위 대신**, **프로세스 행동에 따라 우선순위를 동적으로 조정**한다.

#### ✅ 추가 규칙

3. **규칙 3**: 모든 새 프로세스는 최상위 큐에서 시작
4. **규칙 4a**: 타임 슬라이스를 **모두 사용**하면 → 우선순위 **낮아짐**
5. **규칙 4b**: 타임 슬라이스 **이전에 CPU 반납**하면 → 우선순위 **유지**

#### 📌 예시

- A: 긴 실행 시간 → 매 타임 슬라이스를 다 쓰고 점점 낮은 우선순위로 이동
- B: 짧고 대화형 → 타임 슬라이스를 다 쓰기 전 CPU를 자주 반납 → 높은 우선순위 유지

> MLFQ는 실행 패턴을 기반으로 **SJF와 비슷하게 동작**

---

### 11.3 시도 2: 우선순위 상향 조정 (Priority Boost)

기아(starvation) 문제와 동적 특성 변화 대응을 위해, 주기적으로 모든 프로세스를 **최상위 큐로 리셋**한다.

#### ✅ 규칙 추가

6. **규칙 5**: 일정 주기 S마다 모든 프로세스를 **최상위 큐로 이동**

#### 📌 효과
- 긴 작업도 결국 CPU를 배정받음 → 기아 방지
- CPU 중심 프로세스가 대화형으로 변해도 빠르게 우선순위를 회복 가능

> S 값은 조정이 필요함: 너무 작으면 대화형 성능 ↓, 너무 크면 기아 발생 ↑

---

### 11.4 시도 3: 스케줄러 조작 방지

**타임 슬라이스가 끝나기 전에 I/O 요청**을 하는 방식으로 우선순위를 유지하려는 **속임수**가 가능하다.

#### 📌 해결책

- **누적 CPU 시간 추적**: 단순한 타임 슬라이스 기준이 아니라, **해당 우선순위 큐에서의 총 CPU 사용량** 기준으로 강등 여부를 결정

#### ✅ 새로운 규칙 정의

4. **규칙 4** (개정): 지정된 큐에서의 **총 시간 소진** → 우선순위 강등 (CPU 반납 시점과 무관)

> 이제는 CPU를 쪼개 써도 결국은 낮은 우선순위로 이동하게 됨

---

### 11.5 MLFQ 조정과 현실적 고려사항

#### ⚙️ 조정할 변수들

| 항목 | 설명 |
|------|------|
| 큐 개수 | 몇 개의 우선순위 단계를 둘 것인가? |
| 큐별 타임 슬라이스 | 우선순위가 낮을수록 보통 길어짐 |
| 상향 주기 (S) | 얼마나 자주 우선순위를 리셋할 것인가? |

#### 📌 예시 (Solaris 시스템)

- 큐 개수: 60
- 타임 슬라이스: 20ms → 수백 ms로 증가
- 상향 주기: 약 1초마다

---

### 11.6 다양한 시스템에서의 MLFQ

- **BSD Unix**: decay-usage 기반으로 CPU 사용량이 오래된 값일수록 감쇠 적용
- **Solaris**: 우선순위-시간 테이블 기반으로 관리
- **Windows NT**: MLFQ 구조 채택

#### 🧠 Ousterhout’s Law
> 피할 수 있다면 **부두 상수(voodoo constants)**를 피하라. 그러나 현실에서는 피할 수 없는 경우가 많다. 디폴트 값이 대체로 사용된다.

---

## ✅ 요약

| 요소 | 설명 |
|------|------|
| 멀티 큐 구조 | 각 큐는 다른 우선순위 및 타임 슬라이스 |
| 동적 피드백 | 실행 패턴을 기반으로 우선순위 변경 |
| 기아 방지 | 주기적 우선순위 상향 (규칙 5) |
| 조작 방지 | 누적 CPU 사용 시간 기준 우선순위 하향 |
| 사용자 조정 | nice 명령 등으로 힌트(advice) 제공 가능 |

MLFQ는 다음을 동시에 달성하려는 노력의 산물이다:
- **짧은 대화형 작업에 빠른 응답**
- **긴 작업도 공정하게 실행**
- **미래 예측 없이도 효율적인 정책 구현**

---

> 📚 출처: 『운영체제: 아주 쉬운 세 가지 이야기 (OSTEP)』
