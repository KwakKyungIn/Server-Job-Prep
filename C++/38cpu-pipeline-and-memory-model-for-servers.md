# ⚙️ 서버 프로그래머를 위한 CPU 파이프라인 & 메모리 모델 총정리

> 고성능 서버(특히 IOCP/에픽 폴링 기반, 멀티스레드)에서 **성능·안전성**을 좌우하는 두 축:  
> 1) **CPU 파이프라인/캐시/분기예측** 등 마이크로아키텍처 이해  
> 2) **C++ 메모리 모델**과 `std::atomic`/동기화 원리  
> 이 문서는 면접/실무에서 바로 써먹을 수 있도록 **원리 → 영향 → 실전 레시피** 순으로 정리했습니다.

---

## 1) CPU 파이프라인: 왜 알아야 하나?

### 1.1 파이프라인 기본
- 현대 CPU는 명령을 **단계별(페치 → 디코드 → 실행 → 메모리 → 커밋)** 로 겹쳐 수행해 **IPC(Instructions Per Cycle)** 를 끌어올립니다.
- **슈퍼스칼라**: 한 사이클에 여러 명령을 동시에 처리.
- **Out-of-Order(OOO)**: 의존성이 없는 명령은 순서 바꿔 먼저 실행하여 **버블(유휴)** 을 줄임.
- **분기 예측(Branch Prediction)**: 분기 결과를 미리 예측해 파이프라인을 채움. 오예측 시 **파이프라인 플러시(수~수십 사이클 손실)**.

**서버 코드 영향**
- **핫패스**에서 **예측 불가능한 분기**(랜덤/해시 기반 조건) → 잦은 미스프리딕트 → 레이턴시 스파이크.
- **분기 수를 줄이거나, 패턴을 예측 가능**하게 만들면 이득. C++20의 `[[likely]]`, `[[unlikely]]` 힌트를 합리적으로 사용.

### 1.2 메모리 계층 & 비용 (대략적 체감치)
- **레지스터**: ~0.3ns
- **L1 캐시**: ~1ns
- **L2 캐시**: ~3–4ns
- **L3 캐시(LLC)**: ~10ns+
- **DRAM**: ~80–120ns
- **디스크/네트워크**: μs–ms
> 결론: **캐시 적중**이 성능을 좌우. **데이터 지역성(locality)** 이 곧 성능.

### 1.3 캐시 라인, 프리패처, TLB
- **캐시 라인**: 보통 64바이트. 이 단위로 메모리를 가져옴.  
  → **False Sharing(거짓 공유)**: 서로 다른 스레드가 **같은 캐시 라인**의 서로 다른 변수를 갱신하면 캐시 일관성 트래픽 폭증.
- **프리패처**: 순차 접근 패턴을 감지해서 미리 당겨옴 → **선형 순회**가 유리.
- **TLB**: 가상주소→물리주소 변환 캐시. **랜덤 접근**이 많으면 TLB 미스 증가.

### 1.4 멀티소켓/NUMA
- 코어들이 **소켓별 메모리 노드**를 가짐. “원격 노드” 접근은 더 느림.
- 실전 요령
  - 스레드 **어피니티**와 **메모리 배치(첫 할당 스레드 노드)** 를 맞춰 **NUMA 로컬리티** 유지.
  - 데이터 구조를 **노드/샤드별**로 분리(sharding)해 원격 접근 최소화.

---

## 2) 캐시 일관성과 코어 간 상호작용

### 2.1 MESI 프로토콜(개념)
- 캐시 라인 상태: **Modified / Exclusive / Shared / Invalid**.
- 한 코어가 쓰기 → 다른 코어의 같은 라인은 **Invalid**로.  
  → 스핀락으로 같은 라인 계속 두드리면 **버스 트래픽 폭증**.

### 2.2 False Sharing 피하기
- 서로 다른 스레드가 갱신하는 카운터/플래그는 **분리·패딩**:
  ```cpp
  struct alignas(64) PaddedCounter { std::atomic<uint64_t> x; char pad[64 - sizeof(std::atomic<uint64_t>)]; };
  ```
- 또는 **배열을 스레드별로 샤딩** 후 마지막에 합산(reduction).

---

## 3) C++ 메모리 모델 핵심

### 3.1 기본 정의
- **데이터 레이스(Data Race)**: 동시 접근(최소 한쪽이 쓰기) + 적절한 동기화 없음 → **UB(Undefined Behavior)**.
- **Happens-Before**: A가 B보다 먼저 관찰(메모리 가시성 포함)되어야 함을 보장하는 **순서 관계**.
- `std::atomic` 연산과 동기화 프리미티브(뮤텍스, CV)는 적절히 사용 시 **Happens-Before**를 형성합니다.

### 3.2 원자 연산의 메모리 순서
- `memory_order_seq_cst` (가장 강함, 기본값): **전역적 총순서** + acquire/release 보장.
- `memory_order_release` / `memory_order_acquire`:  
  - **Release**: 이전 쓰기들을 **이 변수**를 통해 **공개**  
  - **Acquire**: 대응하는 release가 공개한 쓰기들을 **가져옴**
- `memory_order_acq_rel`: read-modify-write에 혼합.
- `memory_order_relaxed`: 순서 제약 없음(동기화 X) — **카운터/통계**처럼 독립적 값엔 유용.

> `memory_order_consume`는 사실상 비권장(대부분 acquire로 취급).

### 3.3 동기화 도구와 관계
- **뮤텍스/락**: 잠금 해제(unlock)는 암묵적 **release**, 잠금(lock)은 **acquire**.
- **조건 변수**: `notify_*`는 release와 결합, `wait`는 acquire로 깨어남(락과 함께 사용 시).

### 3.4 `volatile` 오해 금지
- C++에서 `volatile`은 **메모리 질서/스레드 간 동기화**를 보장하지 않음(하드웨어 레지스터용).  
  → 동기화엔 **`std::atomic`/락/펜스**를 사용.

---

## 4) 실전 레시피 (서버 코어 패턴)

### 4.1 퍼블리시/구독 (데이터 + 플래그)
```cpp
struct Data { int a; int b; };
std::atomic<bool> ready{false};
Data payload;

void producer() {
    payload.a = 10;
    payload.b = 20;           // 1) 일반 쓰기
    ready.store(true, std::memory_order_release); // 2) release로 공개
}

void consumer() {
    while (!ready.load(std::memory_order_acquire)) { /* spin or yield */ }
    // 여기 도달하면 producer의 payload.a/b가 보장됨
    use(payload);
}
```
- **원칙**: “데이터를 먼저 쓰고 → release 플래그 설정” / “플래그를 acquire로 확인 후 → 데이터 읽기”.

### 4.2 SPSC 링버퍼 (단일 생산자/소비자)
```cpp
template<typename T, size_t N>
struct SpscRing {
    T buf[N];
    std::atomic<size_t> head{0}; // 소비자만 증가
    std::atomic<size_t> tail{0}; // 생산자만 증가

    bool push(const T& v) {
        size_t t = tail.load(std::memory_order_relaxed);
        size_t h = head.load(std::memory_order_acquire); // 소비자의 head 관찰
        if (((t + 1) % N) == h) return false;            // full
        buf[t] = v;                                      // 데이터 쓰기
        tail.store((t + 1) % N, std::memory_order_release); // 공개
        return true;
    }
    bool pop(T& out) {
        size_t h = head.load(std::memory_order_relaxed);
        size_t t = tail.load(std::memory_order_acquire); // 생산자의 tail 관찰
        if (h == t) return false;                        // empty
        out = buf[h];                                    // 데이터 읽기
        head.store((h + 1) % N, std::memory_order_release);
        return true;
    }
};
```
- 생산자는 `tail`을, 소비자는 `head`를 **독점적으로** 갱신 → 충돌 최소화.
- 인덱스 관찰 시 `acquire`, 공개 시 `release`.

### 4.3 Double-Checked Locking (DCLP) — 올바른 형태
```cpp
std::atomic<Foo*> inst{nullptr};
std::mutex m;

Foo* get() {
    Foo* p = inst.load(std::memory_order_acquire);
    if (!p) {
        std::lock_guard<std::mutex> g(m);
        p = inst.load(std::memory_order_relaxed);
        if (!p) {
            p = new Foo();
            inst.store(p, std::memory_order_release);
        }
    }
    return p;
}
```
- 포인터 **publish** 시 `release`, 읽을 때 `acquire`.

### 4.4 통계 카운터(낮은 비용)
```cpp
std::atomic<uint64_t> req_count{0};
void on_request() {
    req_count.fetch_add(1, std::memory_order_relaxed); // 순서 불필요 → relaxed
}
```
- 독립적 통계는 **relaxed**로 충분. 다만 **false sharing** 주의(패딩/샤딩).

---

## 5) 스핀락/백오프/파이프라인

### 5.1 스핀락의 주의점
- 뜨거운 라인을 계속 갱신 → **MESI Invalidations** 남발 → 전체 성능 악화.
- 단기 임계구역 + 충돌 낮은 상황에만. 그렇지 않으면 **뮤텍스/세마포어/이벤트**로 전환.

### 5.2 백오프 & `pause`
```cpp
for (int k=0; !try_lock(); ++k) {
    if (k < 16) _mm_pause();           // x86: 파이프라인 친화적 스핀
    else std::this_thread::yield();    // 장기 충돌: 양보
}
```
- `pause`는 하이퍼스레딩 환경에서 **파이프라인 스톨 완화**와 **전력 절감**에 도움.

---

## 6) 메모리 모델에서 흔한 함정

- **원자와 비원자를 섞어** 같은 변수를 접근: 데이터 레이스/UB.
- **너무 강한 순서**(`seq_cst`)를 과도하게 사용: 불필요한 배리어 비용.  
  ↳ 먼저 **올바름**을 확보, 그다음 필요한 곳에만 낮은 오더로 최적화.
- **`volatile`로 동기화하려는 시도**: 잘못된 사용.
- **원자 64비트 정렬 문제**(특히 32비트 타겟): 정렬 보장 확인.
- **메모리 해제와 재사용**: Lock-free 구조에서 **ABA 문제**/메모리 재생성 위험 → **Hazard Pointer / Epoch Reclamation** 고려.

---

## 7) 서버 프로그래머 관점 체크리스트

- [ ] 핫패스 분기 예측 가능? 불가하면 분기 수를 줄였는가?
- [ ] 데이터 구조가 **선형 접근**/캐시 친화적인가?
- [ ] 스레드-로컬/샤딩으로 **경합과 거짓 공유**를 줄였는가?
- [ ] NUMA 환경에서 **스레드와 메모리 배치**가 일치하는가?
- [ ] 동기화는 **뮤텍스/조건변수** 등 고수준 API 우선, 원자/펜스는 필요한 곳에만.
- [ ] 퍼블리시/구독은 **release → acquire** 패턴을 지켰는가?
- [ ] 스핀은 짧게, **백오프**와 **양보** 전략 포함했는가?
- [ ] TSan/UBSan/ASan 등 **정적·동적 분석 도구**로 검증했는가?

---

## 8) 작은 벤치 지침

- **초당 요청 수(QPS)** 와 **p99/999 지연시간**을 함께 본다.  
- 프로파일러/perf/ETW로 **캐시 미스/분기 미스**를 측정해 **핫스팟**을 찾아라.
- 미세 최적화 전, **알고리즘/데이터 구조**를 먼저 바꿔라.

---

## 부록 A — 메모리 오더 요약표

| 오더                | 의미                                                         | 주용도                                      |
|---------------------|--------------------------------------------------------------|---------------------------------------------|
| `relaxed`           | 원자성만 보장, 순서/가시성 X                                 | 독립 카운터, 프로파일 통계                  |
| `acquire`           | 이후 읽기/쓰기가 앞서한 쓰기를 볼 수 있도록 **가져옴**       | 플래그/포인터 읽기                          |
| `release`           | 이전 쓰기를 **공개**                                         | 플래그/포인터 쓰기(퍼블리시)                |
| `acq_rel`           | RMW에 사용(읽으면서 쓰기)                                   | fetch_add 등 RMW에 양방향 제약              |
| `seq_cst`           | 가장 강함(전역 총순서), 간단하지만 비용↑                      | 단순히 올바름이 최우선일 때                 |

---

## 마무리

- **성능**: 파이프라인/캐시/분기예측/NUMA의 **물리 법칙**을 코드로 존중하라.  
- **정확성**: C++ 메모리 모델의 **Happens-Before** 규칙에 맞춰 `atomic`/락을 배치하라.  
- **전략**: 먼저 **정확성 → 측정 → 병목 제거 → 미세 튜닝** 순서를 지켜라.

