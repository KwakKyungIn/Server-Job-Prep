# 📚 STL 알고리즘 정리 - 핵심 함수 중심

> 이 문서는 C++ STL에서 자주 사용되는 알고리즘 함수들을 정리한 문서입니다.  
> 게임 서버 개발을 포함한 실무에서 매우 자주 등장하며, 성능과 정확성에 직결되는 기본기입니다.  
> `algorithm` 헤더에 포함된 표준 함수들을 대상으로 하며, 조건과 함께 쓰는 예제도 포함합니다.

---

## 🔍 1. `std::find`

- **설명**: 주어진 범위에서 특정 값을 처음으로 찾음
- **반환**: 찾은 위치의 반복자, 없으면 `end()`
- **헤더**: `<algorithm>`

### 📌 예제

```cpp
#include <algorithm>
#include <vector>
#include <iostream>

int main() {
    std::vector<int> v = {1, 3, 5, 7, 9};

    auto it = std::find(v.begin(), v.end(), 5);
    if (it != v.end()) {
        std::cout << "찾음: " << *it << '\n';
    }
}
```

---

## 🔢 2. `std::count`

- **설명**: 특정 값이 몇 번 등장하는지 개수를 셈
- **반환**: 정수형 개수

### 📌 예제

```cpp
int cnt = std::count(v.begin(), v.end(), 3);
std::cout << "3의 개수: " << cnt << '\n';
```

---

## ✅ 3. `std::all_of`

- **설명**: 모든 원소가 조건을 만족하는지 확인
- **반환**: `true` 또는 `false`

### 📌 예제

```cpp
bool allOdd = std::all_of(v.begin(), v.end(), [](int x) {
    return x % 2 == 1;
});
std::cout << (allOdd ? "모든 수가 홀수" : "짝수 있음") << '\n';
```

---

## 🔎 4. `std::any_of`

- **설명**: 하나라도 조건을 만족하면 `true`
- **반환**: `true` 또는 `false`

### 📌 예제

```cpp
bool hasEven = std::any_of(v.begin(), v.end(), [](int x) {
    return x % 2 == 0;
});
std::cout << (hasEven ? "짝수 있음" : "전부 홀수") << '\n';
```

---

## ❌ 5. `std::none_of`

- **설명**: 모든 원소가 조건을 만족하지 않아야 `true`
- **반환**: `true` 또는 `false`

### 📌 예제

```cpp
bool noneNegative = std::none_of(v.begin(), v.end(), [](int x) {
    return x < 0;
});
std::cout << (noneNegative ? "음수 없음" : "음수 있음") << '\n';
```

---

## 🔁 6. `std::for_each`

- **설명**: 각 원소에 대해 함수 실행
- **반환**: 없음 (부수효과 목적)

### 📌 예제

```cpp
std::for_each(v.begin(), v.end(), [](int x) {
    std::cout << x << ' ';
});
```

---

## 🧹 7. `std::remove` (매우 중요)

- **설명**: **삭제가 아님**! 삭제할 값들을 뒤로 보내고, 유효 범위를 줄여주는 **재배치 알고리즘**
- **반환**: 제거 대상 이후의 반복자 (새로운 끝)

> 실제 삭제하려면 `erase`와 반드시 함께 써야 한다  
> ⇒ **Erase-Remove Idiom**

### 📌 잘못된 예시 (자주 나오는 실수)

```cpp
for (auto it = v.begin(); it != v.end(); ++it) {
    if (*it == 3) {
        v.erase(it); // ❌ 오류 발생
    }
}
```

> 위 코드는 반복자를 무효화시키며 `undefined behavior`를 일으킴

### ✅ 올바른 사용법 (Erase-Remove Idiom)

```cpp
v.erase(std::remove(v.begin(), v.end(), 3), v.end());
```

- 조건이 있는 경우 → `std::remove_if`

```cpp
v.erase(std::remove_if(v.begin(), v.end(), [](int x) {
    return x < 0; // 음수를 제거
}), v.end());
```

---

## 🧠 정리 요약

| 알고리즘 | 목적 | 조건 사용 | 결과 |
|----------|------|-----------|------|
| `find` | 값 찾기 | ❌ | 반복자 반환 |
| `count` | 값 개수 세기 | ❌ | 정수 반환 |
| `all_of` | 전부 조건 만족 | ✅ | bool |
| `any_of` | 하나라도 조건 만족 | ✅ | bool |
| `none_of` | 아무것도 조건 불만족 | ✅ | bool |
| `for_each` | 모든 원소에 함수 적용 | ✅ | void |
| `remove` | 값 재배치 | ❌ | 반복자 반환, `erase`와 함께 사용 |

---

## ✅ 면접 팁

- `remove`는 **삭제 아님** → 반드시 `erase`와 함께 써야 함
- `for_each`는 반복문보다 깔끔한 코드 작성에 유용
- `all_of`, `any_of`, `none_of`는 조건 검사를 명확히 표현할 수 있어 **가독성 향상**
- `remove_if`는 조건 기반 삭제에 최적

