# 🆚 C++ 포인터 vs 배열

C++에서는 배열과 포인터가 매우 비슷하게 동작하지만,  
**정확히는 다르며**, 내부 구조나 동작 방식에서 **중요한 차이점**이 존재합니다.

---

## 📌 배열 이름은 포인터처럼 사용된다?

```cpp
int arr[3] = {10, 20, 30};
int* p = arr;           // 암묵적 변환: &arr[0]

std::cout << arr[1] << std::endl;  // 20
std::cout << *(p + 1) << std::endl; // 20
```

- 배열 이름은 첫 번째 요소의 **주소로 변환(decay)**됨
- `arr[i]`와 `*(arr + i)`는 동일하게 작동
- 하지만 `arr = ...`처럼 **배열 이름은 재할당 불가**

---

## 💡 예제: sizeof 차이

```cpp
int arr[5] = {1,2,3,4,5};
int* ptr = arr;

std::cout << sizeof(arr) << std::endl;  // 20 (5 * 4바이트)
std::cout << sizeof(ptr) << std::endl;  // 8 (포인터 크기)
```

- `sizeof(arr)`는 **배열 전체 크기**
- `sizeof(ptr)`는 **포인터 자체 크기 (x64 기준 8바이트)**

---

## 💡 예제: 함수 인자로 전달될 때

```cpp
void func1(int arr[5]) { std::cout << sizeof(arr) << std::endl; }
void func2(int* ptr)   { std::cout << sizeof(ptr) << std::endl; }

int main() {
    int arr[5];
    func1(arr);  // 출력: 8
    func2(arr);  // 출력: 8
}
```

- 배열은 함수 인자로 전달되면 **무조건 포인터로 decay**됨
- 따라서 배열 크기를 알고 싶다면 `std::size()` 또는 템플릿 사용

---

## ⚖️ 배열 vs 포인터 차이 요약

| 항목 | 배열 | 포인터 |
|------|------|--------|
| 선언 | `int arr[3];` | `int* ptr;` |
| 메모리 소유 | 자체적으로 공간 가짐 | 가리키기만 함 |
| 크기 (`sizeof`) | 전체 배열 크기 | 포인터 크기 |
| 주소 | `&arr[0]` | `ptr` 자체가 주소 |
| 변경 가능 여부 | 불가능 (`arr++` 오류) | 가능 (`ptr++` 가능) |
| 함수 인자 전달 시 | 포인터로 decay됨 | 그대로 전달 |

---

## ⚠️ 주의사항

- 배열은 **복사 불가능** (`arr2 = arr1` 오류 발생)
- 배열 이름은 **주소처럼 쓰이지만 상수포인터처럼 동작**
- 포인터는 재할당/연산 가능하지만, 배열은 불가능

---

## ✅ 정리

- 배열과 포인터는 유사한 문법이 많지만, 구조적으로는 완전히 다름
- 배열은 고정 크기 메모리 덩어리, 포인터는 참조 주소만 가짐
- 배열 이름은 포인터처럼 행동하지만, 상수처럼 불변
