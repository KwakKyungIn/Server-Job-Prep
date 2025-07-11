# 🔑 주소 변환의 원리 (OSTEP Chapter 18)

이 장에서는 운영체제가 메모리 가상화를 **어떤 하드웨어와 소프트웨어 기술로 구현하는지**, 그리고 프로그램이 바라보는 가상 주소와 실제 물리 주소가 어떻게 연결되는지 구체적으로 살펴본다.  
**Base-and-Bound 방식의 동적 주소 변환이 핵심 내용이며**, 이는 이후 다룰 세그멘테이션과 페이징의 기초가 된다.

---

## ❓ 핵심 질문

> 어떻게 하면 프로그램이 물리 메모리의 정확한 위치를 몰라도, **자신만의 연속적 주소 공간을 안전하게 사용할 수 있을까?**

---

## 🔍 상세 개념 정리

### 18.1 가상 메모리의 첫 단계: 동적 재배치

가장 단순한 가상 메모리 구현은 **각 프로세스를 물리 메모리의 다른 위치에 재배치**하는 것이다. 이를 위해 사용한 방식이 바로 **동적 재배치(Dynamic Relocation)** 또는 **Base-and-Bound** 기법이다.

#### 🛡️ 핵심 하드웨어: Base와 Bound 레지스터
| 레지스터 | 역할 |
|----------|------|
| Base 레지스터 | 프로세스의 시작 물리 주소 |
| Bound 레지스터 | 프로세스의 주소 공간 크기 (보호용) |

#### 📈 주소 변환 공식
```
물리 주소 = 가상 주소 + Base
```

#### 🔍 보호 기능
- 가상 주소가 Bound 범위를 초과하면 **CPU가 예외(Exception)** 발생
- 다른 프로세스 메모리 침범 방지

---

### 18.2 주소 변환의 예시

#### 📂 가상 주소 공간 예시
| 주소 | 내용 |
|-----|------|
| 0 ~ 4KB | 코드 |
| 4KB ~ 8KB | 데이터 |
| 8KB ~ 16KB | 스택 등 |

#### 예제 코드 (x86)
```asm
movl 0x0(%ebx), %eax   ; EBX의 주소에서 값 로드
addl $0x3, %eax        ; 3 더하기
movl %eax, 0x0(%ebx)   ; 다시 저장
```

#### 주소 변환 예시
- EBX = 15KB, Base = 32KB
- 물리 주소 = 15KB + 32KB = **47KB**
- 실제 메모리에서는 47KB 위치의 값을 읽고 쓰는 것

---

### 18.3 동작 방식

#### ✅ 동작 흐름
1. 명령어 주소: PC → Base 더해 물리 주소 생성
2. 데이터 주소: 가상 주소 + Base → 물리 주소 변환
3. 가상 주소 > Bound → 예외 발생 (Segmentation Fault)

#### ✅ 하드웨어 개입
- 변환과 보호 검사는 CPU 내의 **MMU(Memory Management Unit)** 에서 자동 처리
- 프로세스 관점에서는 전혀 보이지 않음 (**투명성 유지**)

---

### 18.4 하드웨어 구성 요약

| 하드웨어 구성 | 기능 |
|----------------|--------------------------|
| 사용자/커널 모드 | 특권 명령어 보호 |
| Base 레지스터 | 시작 물리 주소 설정 |
| Bound 레지스터 | 접근 범위 제한 |
| 예외 처리 | 범위 초과, 특권 명령어 실행 시 인터럽트 발생 |

#### 🎯 커널이 하는 일
- Base/Bound 설정
- 예외 핸들러 등록
- 특권 명령어 실행 허용

---

### 18.5 운영체제의 역할

#### 📌 프로세스 생성 시
- 빈 메모리 공간 검색 (free list 사용)
- Base/Bound 레지스터 설정
- 메모리 영역을 프로세스에 할당 표시

#### 📌 프로세스 종료 시
- 메모리 해제 → free list에 추가
- PCB(Process Control Block) 정리

#### 📌 문맥 교환(Context Switch)
- Base/Bound 값을 PCB에 저장
- 새 프로세스 스케줄 시 레지스터 값 복원

#### ✅ 메모리 이동도 가능
프로세스를 일시 중단 → 메모리 복사 → Base 값 변경 → 다른 곳에서 재개

---

### 18.6 문제점 및 한계

#### ⚠️ 내부 단편화 발생
- 주소 공간이 작아도 **고정된 슬롯 크기로 할당** → 메모리 낭비

#### ⚠️ 확장성 한계
- 주소 공간 확장 시, 전체 공간 이동 필요
- 여러 프로세스를 작은 메모리 영역에 효율적으로 배치 불가

#### 👉 해결책
→ **세그멘테이션(Segmentation)**, **페이징(Paging)** 으로 발전

---

### 🛡️ 보안적 중요성

- Base/Bound 없이는 프로세스가 **임의 주소에 접근 가능**
- 예를 들어, 트랩 테이블(trap table)을 덮어써서 커널 제어 탈취 가능
- Base/Bound가 **가장 기본적인 보호 기법**

---

### 🔍 참고: 소프트웨어 재배치의 한계

하드웨어 없이 소프트웨어만으로도 재배치는 가능하지만:
- 보호 불가: 잘못된 코드가 다른 영역 침범 가능
- 실행 중 이동 불가: 시작 시점에만 주소 고정
- 유지 보수가 어려움

결국, 하드웨어 지원이 필요하다.

---

## ✅ 요약

- 운영체제와 하드웨어는 Base/Bound 레지스터를 통해 **간단한 형태의 가상 주소 → 물리 주소 변환(Address Translation)** 을 구현한다.
- 이를 통해 **메모리 보호**, **독립적 주소 공간**, **효율적 재배치** 가 가능해진다.
- 하지만 이 방식은 **내부 단편화**, **유연성 부족** 등의 한계가 있고, 이를 개선하기 위해 세그멘테이션과 페이징이 등장한다.

---

> 📚 출처: 『운영체제: 아주 쉬운 세 가지 이야기 (OSTEP)』
