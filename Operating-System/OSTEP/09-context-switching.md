# 🧪 막간: 프로세스 API (OSTEP Chapter 8)

이 장에서는 운영체제가 제공하는 **프로세스 생성 및 제어를 위한 핵심 시스템 콜 API들**을 다룬다. 특히, Unix 계열 시스템의 `fork()`, `exec()`, `wait()` 시스템 콜을 통해 프로세스 생성, 실행, 종료 대기, 입출력 재지정 등 실제 동작 흐름을 이해하는 것이 목표다.

---

## ❓ 핵심 질문

> 운영체제는 **프로세스를 생성하고 제어**하기 위해 어떤 인터페이스(API)를 제공해야 할까? 그리고 왜 그렇게 설계되었을까?

---

## 🔍 상세 개념 정리

### 8.1 `fork()` 시스템 콜 – 복제의 마법

`fork()`는 현재 실행 중인 **프로세스를 복제**하여 **자식 프로세스**를 만든다. 복제된 자식은 부모와 거의 동일한 메모리, 코드, 레지스터, 파일 디스크립터 상태를 갖고 생성된다.

#### 📌 핵심 개념
- 부모와 자식은 **완전히 분리된 프로세스**
- `fork()` 호출 결과:
  - 부모는 자식의 PID를 반환받음
  - 자식은 `0`을 반환받음
- 이를 활용해 **분기 로직(if/else)** 으로 자식과 부모의 동작을 다르게 작성할 수 있음

#### 💻 예제 코드 (`p1.c`)
```c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main() {
    printf("hello world (pid:%d)\n", getpid());
    int rc = fork();

    if (rc < 0) {
        fprintf(stderr, "fork failed\n");
        exit(1);
    } else if (rc == 0) {
        printf("hello, I am child (pid:%d)\n", getpid());
    } else {
        printf("hello, I am parent of %d (pid:%d)\n", rc, getpid());
    }

    return 0;
}
```

#### 💡 실행 결과 (비결정성 존재)
```bash
hello world (pid:1234)
hello, I am child (pid:1235)
hello, I am parent of 1235 (pid:1234)
```

> 실행 순서는 항상 바뀔 수 있음 → **CPU 스케줄러가 어떤 프로세스를 먼저 실행하느냐에 따라 결정됨**

---

### 8.2 `wait()` 시스템 콜 – 자식이 끝날 때까지 기다리기

부모 프로세스가 `wait()`를 호출하면 **자식 프로세스가 종료될 때까지** 실행을 멈추고 기다린다. 자식이 종료되면 그 시점에서 `wait()`가 반환된다.

#### 💻 예제 코드 (`p2.c`)
```c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main() {
    printf("hello world (pid:%d)\n", getpid());
    int rc = fork();

    if (rc < 0) {
        fprintf(stderr, "fork failed\n");
        exit(1);
    } else if (rc == 0) {
        printf("hello, I am child (pid:%d)\n", getpid());
    } else {
        int wc = wait(NULL);
        printf("hello, I am parent of %d (wc:%d) (pid:%d)\n", rc, wc, getpid());
    }

    return 0;
}
```

#### 💡 실행 결과 (항상 동일)
```bash
hello world (pid:2000)
hello, I am child (pid:2001)
hello, I am parent of 2001 (wc:2001) (pid:2000)
```

> 자식이 끝날 때까지 기다리므로 **출력 순서가 항상 동일함**

---

### 8.3 `exec()` 시스템 콜 – 나를 다른 프로그램으로 바꾸기

`exec()` 계열 함수는 **현재 프로세스를 다른 프로그램으로 대체**한다. 즉, `exec()` 호출 이후에는 이전 코드가 더 이상 실행되지 않고, **다른 실행 파일이 그 자리를 차지하게 된다.**

#### 📌 동작 원리
- `exec()` 호출 시:
  - 현재 프로세스의 **코드, 데이터, 힙, 스택**이 모두 새로운 프로그램으로 덮어쓰기됨
  - **새로운 프로세스는 생성되지 않음**
- 성공 시 `exec()`는 반환되지 않음 (실패 시에만 반환)

#### 💻 예제 코드 (`p3.c`)
```c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>

int main() {
    printf("hello world (pid:%d)\n", getpid());
    int rc = fork();

    if (rc < 0) {
        fprintf(stderr, "fork failed\n");
        exit(1);
    } else if (rc == 0) {
        printf("hello, I am child (pid:%d)\n", getpid());
        char *args[3];
        args[0] = strdup("wc");
        args[1] = strdup("p3.c");
        args[2] = NULL;
        execvp(args[0], args);
        printf("this shouldn't print out\n");
    } else {
        int wc = wait(NULL);
        printf("hello, I am parent of %d (wc:%d) (pid:%d)\n", rc, wc, getpid());
    }

    return 0;
}
```

#### 💡 출력 예시
```bash
hello world (pid:1234)
hello, I am child (pid:1235)
29 107 1030 p3.c
hello, I am parent of 1235 (wc:1235) (pid:1234)
```

---

### 8.4 입출력 재지정 – `stdout`을 파일로 보내기

`fork()`와 `exec()` 사이에 **파일 디스크립터 조작**을 통해 **표준 출력(STDOUT)** 을 파일로 재지정할 수 있다.

#### 💻 예제 코드 (`p4.c`)
```c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/wait.h>

int main() {
    int rc = fork();
    if (rc == 0) {
        close(STDOUT_FILENO);
        open("./p4.output", O_CREAT | O_WRONLY | O_TRUNC, S_IRWXU);
        
        char *args[3];
        args[0] = strdup("wc");
        args[1] = strdup("p4.c");
        args[2] = NULL;
        execvp(args[0], args);
    } else {
        wait(NULL);
    }
    return 0;
}
```

#### 💡 실행 예시
```bash
./p4
cat p4.output
32 109 846 p4.c
```

> 표준 출력이 `p4.output` 파일로 재지정됨

---

### 8.5 여타 API들

| 함수 | 설명 |
|------|------|
| `kill(pid, SIGINT)` | 특정 프로세스에 시그널 보내기 |
| `waitpid(pid, ...)` | 특정 자식의 종료 기다리기 |
| `ps`, `top` | 실행 중인 프로세스 목록 보기 |
| `pipe()` | 프로세스 간 데이터 전달용 파이프 생성 |

---

## ✅ 요약

- `fork()`: 현재 프로세스를 복사하여 자식 프로세스 생성
- `wait()`: 자식이 종료될 때까지 대기
- `exec()`: 현재 프로세스를 다른 프로그램으로 교체
- 입출력 재지정: 실행 전 `STDOUT`을 닫고 새로운 파일을 열면 출력을 파일로 보낼 수 있음
- Unix의 프로세스 API는 **단순하지만 매우 강력**한 조합을 제공하며, 수많은 프로그래밍 응용의 기본이 된다

---

> 📚 출처: 『운영체제: 아주 쉬운 세 가지 이야기 (OSTEP)』
