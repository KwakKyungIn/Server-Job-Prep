# 🛡️ 락 기반의 병행 자료 구조 (OSTEP Chapter 32)

이 장에서는 병행성을 지원하는 자료 구조를 만들기 위해 **락을 어떻게 추가하고 사용해야 하는지**를 구체적인 예제와 함께 살펴본다.  
병행 자료 구조를 만드는 것은 단순히 락을 추가하는 것 이상으로, **성능과 정확성을 모두 고려해야 하는 어려운 문제**다.

---

## ❓ 핵심 질문

> 자료 구조에 락을 추가할 때 **어떤 방식으로 추가해야 정확성을 유지하면서도 성능을 유지할 수 있을까?**  
> 락을 너무 많이 사용하면 성능이 저하되는데, 어떻게 하면 병행성과 성능의 균형을 맞출 수 있을까?

---

## 🔍 상세 개념 정리

---

### 32.1 병행 카운터

#### 🛡️ 기본 카운터 구조
```c
typedef struct __counter_t {
    int value;
} counter_t;

void init(counter_t *c) { c->value = 0; }

void increment(counter_t *c) { c->value++; }

void decrement(counter_t *c) { c->value--; }

int get(counter_t *c) { return c->value; }
```

→ 이 구조는 동기화가 없어서 멀티쓰레드 환경에서는 잘못된 결과를 낸다.

#### 🔑 락 추가 버전
```c
typedef struct __counter_t {
    int value;
    pthread_mutex_t lock;
} counter_t;

void init(counter_t *c) {
    c->value = 0;
    pthread_mutex_init(&c->lock, NULL);
}

void increment(counter_t *c) {
    pthread_mutex_lock(&c->lock);
    c->value++;
    pthread_mutex_unlock(&c->lock);
}
```

#### ⚠️ 성능 문제
- 정확히 동작하지만 성능이 낮음
- 모든 쓰레드가 하나의 락을 두고 경쟁함 → 확장성 낮음

#### 🔬 성능 측정 결과
- 1개 쓰레드: 빠름 (~0.03초)
- 4개 쓰레드: 매우 느려짐 (~5초 이상)
- → 완벽한 확장성(perfect scaling) X

#### 🧠 확장 가능한 카운터: 엉성한 카운터(Sloppy Counter)

##### 구조
| 항목 | 설명 |
|-----|------|
| 전역 카운터 | 전체 값을 유지 |
| 지역 카운터 | CPU마다 별도로 관리 |
| 지역 락 | 각 CPU의 카운터 보호 |
| 전역 락 | 전역 카운터 보호 |
| 임계치 S | 지역 → 전역 전송 빈도 |

##### 기본 원리
- 각 CPU는 자신의 지역 카운터를 증가시킴 (지역 락으로 보호)
- S 값에 도달하면 전역 카운터에 반영하고 지역 카운터 초기화
- 정확한 값을 얻을 땐 모든 지역 락과 전역 락 획득 후 합산

##### 예시 흐름
| 시간 | L1 | L2 | L3 | L4 | G |
|-----|----|----|----|----|---|
| 0   | 0  | 0  | 0  | 0  | 0 |
| 6   | 5→0| 1  | 3  | 4  | 5 |
| 7   | 0  | 2  | 4  | 5→0|10 |

##### 성능
- S가 클수록 성능 향상
- S가 작으면 정확성 높음

---

### 32.2 병행 연결 리스트

#### 🛡️ 기본 연결 리스트
```c
typedef struct __node_t {
    int key;
    struct __node_t *next;
} node_t;

typedef struct __list_t {
    node_t *head;
    pthread_mutex_t lock;
} list_t;
```

#### 🔑 삽입 코드
```c
int List_Insert(list_t *L, int key) {
    pthread_mutex_lock(&L->lock);
    node_t *new = malloc(sizeof(node_t));
    if (new == NULL) {
        pthread_mutex_unlock(&L->lock);
        return -1;
    }
    new->key = key;
    new->next = L->head;
    L->head = new;
    pthread_mutex_unlock(&L->lock);
    return 0;
}
```

#### ⚠️ 문제점
- malloc() 실패 시 락 해제 누락 가능 → 오류 발생 가능성
- 전체 리스트에 하나의 락만 있어 확장성 낮음

#### 🔧 개선 방법
- **임계 영역만 락으로 보호**
- 실패 처리 로직에서 락 해제 누락 방지

#### ✅ 확장성 개선 기법: Hand-over-Hand Locking
- 각 노드마다 락을 두고 순서대로 획득/해제
- → 병행성 ↑, 하지만 오버헤드 큼

---

### 32.3 병행 큐

#### 🔍 Michael & Scott 큐
```c
typedef struct __queue_t {
    node_t *head;
    node_t *tail;
    pthread_mutex_t headLock;
    pthread_mutex_t tailLock;
} queue_t;
```

##### 특징
- head, tail 각각 별도 락
- 삽입: tailLock 사용
- 삭제: headLock 사용
- 삽입과 삭제가 독립적으로 병행 가능

#### ✅ 더미 노드 사용
- 초기 상태에서 head와 tail이 같은 노드를 가리킴
- 데이터 없는 상태 구분

---

### 32.4 병행 해시 테이블

#### 🛡️ 기본 구조
```c
typedef struct __hash_t {
    list_t lists[BUCKETS];
} hash_t;
```

#### ✅ 원리
- 각 버킷에 개별 리스트 존재
- 각 리스트에 락 존재 → 병행성 ↑
- 전체에 하나의 락을 쓰는 것보다 훨씬 확장성 좋음

---

### 32.5 요약

| 자료 구조 | 기본 방법 | 확장성 개선 |
|-----------|------------|-------------|
| 카운터 | 전역 락 | 지역 + 전역 카운터 |
| 리스트 | 전체 락 | 노드별 락 |
| 큐 | head/tail 락 분리 | Michael & Scott 큐 |
| 해시 테이블 | 전체 락 | 버킷별 락 |

#### ⚠️ 중요한 교훈
- 병행성이 높아진다고 무조건 성능이 좋아지는 것은 아니다
- 락 획득/해제 오버헤드가 병행성 이득보다 크면 의미 없음
- 성능 향상은 필요할 때만 추구하라 (미숙한 최적화 X)

---

## ✅ 요약

병행 자료 구조는 **정확성(올바르게 동작)** 과 **성능(확장성)** 을 모두 고려해야 한다.  
병행성을 높이려면 락을 잘게 쪼개야 하지만, 락 자체의 비용도 커지므로 신중해야 한다.

| 자료 구조 | 확장성 기술 |
|-----------|------------|
| 카운터 | 엉성한 카운터 (sloppy counter) |
| 리스트 | Hand-over-hand locking |
| 큐 | head/tail 락 분리 |
| 해시 테이블 | 버킷별 락 |

→ 시작은 단순한 하나의 락으로 하고, 성능이 문제일 때만 개선하라.  
→ “미숙한 최적화는 모든 악의 근원이다.” – Knuth

---

> 📚 출처: 『운영체제: 아주 쉬운 세 가지 이야기 (OSTEP)』
