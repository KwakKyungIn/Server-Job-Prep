# ⚡ 페이징: 더 빠른 주소 변환 (TLB) (OSTEP Chapter 22)

페이징은 메모리 보호와 가상 주소 공간 분리에 매우 효과적이지만, 성능 저하라는 문제를 발생시킨다.  
그 이유는 **페이지 테이블을 매번 메모리에서 조회해야 하기 때문**이다. 이 성능 문제를 해결하는 핵심 장치가 바로 **TLB(Translation Lookaside Buffer, 변환 색인 버퍼)** 다.

---

## ❓ 핵심 질문

> 주소 변환 시 페이지 테이블을 매번 읽는 성능 저하를 어떻게 줄일 수 있을까?  
> 하드웨어와 운영체제는 어떤 역할을 분담해야 할까?

---

## 🔍 상세 개념 정리

### 22.1 TLB의 개념과 동작

#### ✅ TLB란?
- **CPU 내의 MMU(Memory Management Unit)** 에 포함된 하드웨어 캐시
- 최근에 변환한 **가상 페이지 번호(VPN) → 물리 프레임 번호(PFN)** 를 저장
- 주소 변환 시, 먼저 TLB를 조회
- → **TLB에 있으면 빠르게 변환 (TLB Hit), 없으면 페이지 테이블 조회 (TLB Miss)**

#### 📈 변환 흐름 (TLB Hit 기준)
1. 가상 주소에서 VPN 추출
2. TLB 조회 → 변환 정보 찾음
3. 물리 주소 계산 후 접근

#### 🛑 TLB Miss 발생 시
1. 하드웨어 또는 운영체제가 페이지 테이블 조회
2. 변환 정보 찾음
3. TLB에 변환 정보 추가
4. 명령어 재실행

---

### 22.2 예제: 배열 접근

#### 🧑‍💻 예제 코드
```c
int sum = 0;
for (int i = 0; i < 10; i++) {
    sum += a[i];
}
```

#### 📦 페이지 배치
| VPN | 오프셋 |
|-----|-------|
| 06  | 04 ~ 15 | → a[0] ~ a[2]
| 07  | 00 ~ 15 | → a[3] ~ a[6]
| 08  | 00 ~ 15 | → a[7] ~ a[9]

#### 🔍 TLB 동작 예시
- a[0]: TLB Miss → 변환 후 저장
- a[1], a[2]: 같은 페이지 → TLB Hit
- a[3]: 새 페이지 → TLB Miss
- a[4] ~ a[6]: 같은 페이지 → TLB Hit
- a[7]: 새 페이지 → TLB Miss
- a[8], a[9]: 같은 페이지 → TLB Hit

→ **총 3번의 TLB Miss**, 히트율 70%

#### ✨ 지역성의 효과
- 공간 지역성(spatial locality): 같은 페이지 내에서 여러 변수 접근 → TLB Hit 증가
- 시간 지역성(temporal locality): 반복문에서 같은 주소 반복 접근 시 Hit 증가

---

### 22.3 TLB Miss 처리 방법

#### 🛡️ 하드웨어 관리 방식 (ex. x86)
- 페이지 테이블 위치와 형식을 하드웨어가 인식
- TLB Miss 발생 시, 하드웨어가 직접 페이지 테이블 조회 → TLB 갱신 후 명령어 재실행

#### 🔧 소프트웨어 관리 방식 (ex. MIPS, SPARC)
- 하드웨어가 예외(Exception) 발생 → 운영체제가 페이지 테이블 조회 및 TLB 갱신
- 운영체제가 더 유연하게 페이지 테이블 구조를 변경 가능

---

### 22.4 TLB 구조

| 필드 | 역할 |
|------|------|
| VPN | 가상 페이지 번호 |
| PFN | 물리 프레임 번호 |
| Valid | 항목 유효 여부 |
| Protect | 접근 권한 |
| ASID | 프로세스 구분용 ID |

#### ✅ 완전 연관(fully associative)
- 모든 항목을 병렬 검색하여 매칭 → 빠름

---

### 22.5 문맥 교환과 TLB 문제

#### ⚠️ 문제
- 다른 프로세스로 전환 시 TLB 내용이 **다른 프로세스의 변환 정보**일 수도 있음

#### 🔧 해결책
1. **TLB 비우기(Flush):** 문맥 교환 시 모든 valid 비트 초기화
2. **ASID(Address Space Identifier):**
   - 프로세스별로 변환 정보를 구분
   - 운영체제는 현재 프로세스의 ASID를 레지스터에 설정

---

### 22.6 캐시 교체 정책

| 정책 | 설명 |
|------|------|
| LRU | 가장 오래된 항목 교체 |
| Random | 무작위로 교체 (간단하고 예외 상황 회피에 강함) |

→ 일반 캐시와 동일하게 지역성을 최대한 활용하는 방식으로 설계

---

### 22.7 실제 TLB 사례: MIPS R4000

#### 📑 구성
| 필드 | 설명 |
|------|------|
| VPN  | 19비트 |
| PFN  | 24비트 |
| Valid, Dirty, Global, ASID 등 |

- 64개 항목 (그 중 일부는 운영체제용으로 예약)
- 명령어를 통해 소프트웨어에서 직접 갱신 (privileged instruction)

---

## ✅ 요약

TLB는 페이징 성능 문제를 해결하기 위한 핵심 하드웨어 캐시다.  
대부분의 주소 변환이 TLB에서 처리되므로, 성능 저하 없이 가상 메모리를 구현할 수 있다.

| 문제 | 해결책 |
|------|--------|
| 변환 지연 | TLB 캐싱 |
| 문맥 교환 시 충돌 | TLB Flush or ASID 사용 |
| 캐시 공간 한계 | 교체 정책 (LRU, Random 등) |

---

> 📚 출처: 『운영체제: 아주 쉬운 세 가지 이야기 (OSTEP)』
