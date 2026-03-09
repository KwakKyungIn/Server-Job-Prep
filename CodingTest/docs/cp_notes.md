# 코테 메모 (자주 까먹는 포인트 모음)

> 목표: 시험장에서 “생각 안 나서 시간 날리는” 포인트만 빠르게 꺼내 쓰기.

---

## 1) `string` 관련

### 1-0. `+=` / `push_back()` / `+` 차이
- `s += "abc"`: 문자열 덩어리 붙이기 가능.
- `s.push_back('a')`: **한 글자(char)** 만 가능.
- `s + "abc"`: **새 문자열을 만들어 반환** → 반복하면 비용 커질 수 있음  
  → **가능하면 `+=`로 누적**.

### 1-1. `begin()` / `end()`는 이터레이터
- `s.begin()`, `s.end()`는 **iterator 반환**  
- `end()`는 “마지막 원소 다음”을 가리킴.

### 1-2. `insert`, `erase` 자주 쓰는 형태
```cpp
// insert(pos, str) : pos 위치에 str 삽입
s.insert(pos, "ABC");

// insert(iterator, char) 도 가능
s.insert(s.begin() + pos, 'X');

// erase(pos, count) : pos부터 count개 삭제
s.erase(pos, count);

// erase(iterator) / erase(range) 도 가능
s.erase(s.begin() + pos);
s.erase(s.begin() + l, s.begin() + r); // [l, r) 삭제
```

### 1-3. `find`는 위치 반환, 실패면 `string::npos`
```cpp
size_t p = s.find("abc");
if (p == string::npos) {
    // 못 찾음
} else {
    // p가 시작 인덱스
}
```

### 1-4. `substr`
```cpp
string a = "abcdef";
string b = a.substr(2);      // "cdef" (2부터 끝까지)
string c = a.substr(2, 3);   // "cde"  (2부터 3글자)
```

### 1-5. ASCII 암기
- `'A'` = 65, `'a'` = 97  
- 실전에서는 그냥 **문자 리터럴로 비교**하는 게 실수 적음:
```cpp
if ('A' <= ch && ch <= 'Z') { /* ... */ }
if ('a' <= ch && ch <= 'z') { /* ... */ }
```

### 1-6. `string::reverse()`는 없다 → `<algorithm>`의 `reverse`
- 반환값 없음(`void`)
- 원본이 바뀜(in-place)
```cpp
#include <algorithm>

reverse(s.begin(), s.end());  // s 자체가 뒤집힘
```

### 1-7. `split()` (구분자가 문자열인 버전)
- 가능하면 통채로 암기
```cpp
#include <bits/stdc++.h>
using namespace std;

vector<string> split(const string& input, const string& delimiter) {
    vector<string> result;
    size_t start = 0;
    size_t end = input.find(delimiter);

    while (end != string::npos) {
        result.push_back(input.substr(start, end - start));
        start = end + delimiter.size();
        end = input.find(delimiter, start);
    }

    result.push_back(input.substr(start));
    return result;
}
```

### 1-8. “문자열이 숫자인지” 체크
- `atoi(s.c_str())`는 **실패하면 0**을 반환하는데,
  - `"0"`도 0이라서 **구분이 불가능**함(주의).

실전에서 안전한 패턴(정수인지 확인 후 변환):
```cpp
bool isInteger(const string& s) {
    if (s.empty()) return false;
    size_t i = 0;
    if (s[0] == '-' || s[0] == '+') i = 1;
    if (i == s.size()) return false;
    for (; i < s.size(); i++) if (!isdigit((unsigned char)s[i])) return false;
    return true;
}

// 사용
if (isInteger(s)) {
    long long x = stoll(s);
}
```

---

## 2) 재귀 (순열/조합)

### 2-1. 기저 사례(Base case)를 맨 위에
- “언제 끝나는지”가 명확해야 스택 폭발/무한 재귀 방지.

### 2-2. 순열: `next_permutation` / `prev_permutation`
```cpp
#include <bits/stdc++.h>
using namespace std;

void printV(const vector<int>& v) {
    for (int x : v) cout << x << ' ';
    cout << '\n';
}

int main() {
    int a[3] = {1, 2, 3};
    vector<int> v(a, a + 3);

    // 오름차순부터 모든 순열
    sort(v.begin(), v.end());
    do {
        printV(v);
    } while (next_permutation(v.begin(), v.end()));

    cout << "-------------\n";

    // 내림차순부터 역방향 순열
    sort(v.begin(), v.end(), greater<int>());
    do {
        printV(v);
    } while (prev_permutation(v.begin(), v.end()));

    return 0;
}
```

### 2-3. 조합: 재귀 버전(인덱스 기반)
```cpp
#include <bits/stdc++.h>
using namespace std;

int n = 5, k = 3;
int a[5] = {1, 2, 3, 4, 5};

void print(const vector<int>& b) {
    for (int x : b) cout << x << ' ';
    cout << '\n';
}

void combi(int start, vector<int>& b) {
    if ((int)b.size() == k) {
        print(b);
        return;
    }
    for (int i = start + 1; i < n; i++) {
        b.push_back(a[i]);     // 값 넣기
        combi(i, b);           // 다음은 i 이후에서 선택
        b.pop_back();
    }
}

int main() {
    vector<int> b;
    combi(-1, b);
    return 0;
}
```

### 2-4. (참고) 조합: 중첩 for문 형태(작은 k에서만)
```cpp
// k=3 같은 고정일 때만. k가 바뀌면 재귀/비트마스크가 낫다.
for (int i = 0; i < n; i++) {
    for (int j = i + 1; j < n; j++) {
        for (int t = j + 1; t < n; t++) {
            cout << i << ' ' << j << ' ' << t << '\n';
        }
    }
}
```

---

## 3) 정수론 / 모듈러

### 3-0. 합동 성질(중요)
- `a ≡ b (mod n)`이고 `b ≡ c (mod n)`이면 `a ≡ c (mod n)`.

### 3-1. 모듈러 연산 성질(시험장 필수)
- `[(a mod n) + (b mod n)] mod n = (a + b) mod n`
- `[(a mod n) - (b mod n)] mod n = (a - b) mod n`
- `[(a mod n) * (b mod n)] mod n = (a * b) mod n`

> 음수 나올 수 있는 뺄셈은 `((x % MOD) + MOD) % MOD` 패턴으로 정리.

---

## 4) 누적합(Prefix Sum)

### 4-1. 1-index로 만드는 게 편함
- `psum[i] = psum[i-1] + a[i]`
- 구간합 `[l..r] = psum[r] - psum[l-1]`

```cpp
#include <bits/stdc++.h>
using namespace std;
using ll = long long;

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    int n, m;
    cin >> n >> m;

    vector<ll> a(n + 1), psum(n + 1);
    for (int i = 1; i <= n; i++) {
        cin >> a[i];
        psum[i] = psum[i - 1] + a[i];
    }

    while (m--) {
        int l, r;
        cin >> l >> r;
        cout << psum[r] - psum[l - 1] << '\n';
    }
    return 0;
}
```

---

## 5) 그래프 구현 & 탐색(기본)

### 5-0. 그래프 표현 3종
- **인접 행렬**: 조밀 그래프 / N이 작을 때  
- **인접 리스트(vector)**: 보통 대부분 문제에서 이게 정배  
- **격자(map/grid)**: 문제 입력이 2D로 주어지면 그대로 씀

---

## 6) 방향 벡터(격자 이동)

### 6-1. 4방향
```cpp
const int dy[4] = {-1, 0, 1, 0};
const int dx[4] = {0, 1, 0, -1};

for (int dir = 0; dir < 4; dir++) {
    int ny = y + dy[dir];
    int nx = x + dx[dir];
}
```

### 6-2. 8방향
```cpp
const int dy[8] = {-1, -1, 0, 1, 1, 1, 0, -1};
const int dx[8] = { 0,  1, 1, 1, 0,-1,-1, -1};
```

---

## 7) Grid DFS (2D DFS) 템플릿

```cpp
#include <bits/stdc++.h>
using namespace std;

int n;
vector<vector<int>> a;
vector<vector<int>> visited;

const int dy[4] = {-1, 0, 1, 0};
const int dx[4] = {0, 1, 0, -1};

void dfs(int y, int x) {
    visited[y][x] = 1;

    for (int dir = 0; dir < 4; dir++) {
        int ny = y + dy[dir];
        int nx = x + dx[dir];

        if (ny < 0 || ny >= n || nx < 0 || nx >= n) continue;
        if (a[ny][nx] == 0) continue;       // 갈 수 없는 칸 조건(문제에 맞게)
        if (visited[ny][nx]) continue;

        dfs(ny, nx);
    }
}

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    cin >> n;
    a.assign(n, vector<int>(n));
    visited.assign(n, vector<int>(n, 0));

    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) cin >> a[i][j];
    }

    // 예: (0,0)부터 탐색
    if (a[0][0] != 0) dfs(0, 0);

    return 0;
}
```

---

## 8) DFS (인접 리스트) 템플릿

### 8-1. 수도 코드
```
DFS(u):
  visited[u] = true
  for v in adj[u]:
    if !visited[v]:
      DFS(v)
```

### 8-2. C++ 구현
```cpp
#include <bits/stdc++.h>
using namespace std;

int n; // 정점 수
vector<vector<int>> adj;
vector<int> visited;

void dfs(int u) {
    visited[u] = 1;
    cout << u << '\n';

    for (int v : adj[u]) {
        if (!visited[v]) dfs(v);
    }
}

int main() {
    n = 6;
    adj.assign(n + 1, {});
    visited.assign(n + 1, 0);

    adj[1].push_back(2);
    adj[1].push_back(3);
    adj[2].push_back(4);
    adj[4].push_back(2);
    adj[2].push_back(5);

    dfs(1);
    return 0;
}
```

### 8-3. DFS 방문 처리 2가지 스타일(자주 헷갈림)
1) **가기 전에 체크**
```cpp
if (!visited[v]) dfs(v);
```

2) **일단 들어가고, 들어오자마자 컷**
```cpp
void dfs(int u){
    if (visited[u]) return;
    visited[u] = 1;
    for (int v: adj[u]) dfs(v);
}
```
- 둘 다 가능. 문제 로직/사이드이펙트(카운트, 출력 등) 기준으로 고르면 됨.

---

## 9) 빠른 체크리스트(시험장용)
- 문자열 누적은 `+=` (반복 `+`는 피하기)
- `find` 실패는 `string::npos`
- `reverse(begin, end)`는 원본 변경
- 재귀는 base case 먼저
- 조합/순열 템플릿은 그냥 손에 익혀두기
- 누적합은 1-index + `psum[r]-psum[l-1]`
- 격자 탐색은 방향벡터 + 범위체크 + 방문체크 순서 고정

---

# BFS 메모

## 1) BFS 수도코드 (방문만 체크)
```
BFS(G, start):
    start.visited = true
    q.push(start)

while q not empty:
    u = q.front(); q.pop()
    for v in Adj[u]:
    if v.visited == false:
    v.visited = true
    q.push(v)

```

## 2) BFS 수도코드 (거리까지 저장)
차이는 딱 한 줄: `visited[v] = visited[u] + 1
```
BFS(G, start):
visited[start] = 1
q.push(start)

while q not empty:
u = q.front(); q.pop()
for v in Adj[u]:
if visited[v] == 0:
visited[v] = visited[u] + 1
q.push(v)
````

## 3) 일반 그래프 BFS (인접 리스트)

```cpp
#include <bits/stdc++.h>
using namespace std;

vector<int> adj[100];
int visited[100]; // 0=미방문, 1부터 거리

void BFS(int start) {
    queue<int> q;
    visited[start] = 1;
    q.push(start);

    while (!q.empty()) {
        int here = q.front(); q.pop();

        for (int there : adj[here]) {
            if (visited[there]) continue;
            visited[there] = visited[here] + 1;
            q.push(there);
        }
    }
}

int main() {
    int nodeList[] = {10, 12, 14, 16, 18, 20, 22, 24};

    adj[10].push_back(12);
    adj[10].push_back(14);
    adj[10].push_back(16);
    adj[12].push_back(18);
    adj[12].push_back(20);
    adj[20].push_back(22);
    adj[20].push_back(24);

    BFS(10);

    for (int x : nodeList) cout << x << " : " << visited[x] << '\n';
    cout << "10 -> 24 최단거리(간선 수): " << visited[24] - 1 << '\n';
    return 0;
}
````

## 4) 격자(2D) BFS

```cpp
#include <bits/stdc++.h>
using namespace std;

const int dy[4] = {-1, 0, 1, 0};
const int dx[4] = {0, 1, 0, -1};

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    int n, m;
    cin >> n >> m;

    int sy, sx, ey, ex;
    cin >> sy >> sx;
    cin >> ey >> ex;

    vector<vector<int>> a(n, vector<int>(m));
    vector<vector<int>> visited(n, vector<int>(m, 0));

    for (int i = 0; i < n; i++)
        for (int j = 0; j < m; j++)
            cin >> a[i][j];

    queue<pair<int,int>> q;
    visited[sy][sx] = 1;
    q.push({sy, sx});

    while (!q.empty()) {
        auto [y, x] = q.front(); q.pop();

        for (int dir = 0; dir < 4; dir++) {
            int ny = y + dy[dir];
            int nx = x + dx[dir];

            if (ny < 0 || ny >= n || nx < 0 || nx >= m) continue;
            if (a[ny][nx] == 0) continue;       // 벽/막힘
            if (visited[ny][nx]) continue;      // 이미 방문

            visited[ny][nx] = visited[y][x] + 1;
            q.push({ny, nx});
        }
    }

    cout << visited[ey][ex] << '\n'; // 1부터 시작한 거리 값
    return 0;
}
```

## 5) 시험장 한 줄 요약

* BFS는 큐(queue)
* 시작점 visited[start]=1 넣고 push
* while에서 pop → 인접 방문 → visited 갱신 → push
* 거리 저장 핵심: `visited[next] = visited[cur] + 1`

