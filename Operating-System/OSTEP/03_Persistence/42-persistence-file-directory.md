# 📁 막간: 파일과 디렉터리 (OSTEP Chapter 42)

운영체제는 CPU와 메모리뿐 아니라 **영속 저장 장치(persistent storage)** 또한 가상화하여 사용자에게 추상화된 파일 및 디렉터리 인터페이스를 제공한다. 이 장에서는 파일 시스템 인터페이스, 파일/디렉터리 구조, 링크, 마운트 등 Unix 기반 시스템의 핵심 개념들을 다룬다.

---

## ❓ 핵심 질문

> 운영체제는 **영속 저장 장치**를 어떻게 관리하고, 사용자와 응용 프로그램에 어떤 **인터페이스(API)** 를 제공하는가?

---

## 🔍 상세 개념 정리

### 42.1 파일과 디렉터리

- **파일(file)**: 읽고 쓸 수 있는 바이트의 연속. 아이노드 번호로 식별됨.
- **디렉터리(directory)**: "이름 ↔ 아이노드 번호"의 쌍을 가진 목록.
- **디렉터리 트리 구조**: 루트 디렉터리(/)부터 시작하는 계층적 구조.
- 절대 경로 예시: `/foo/bar.txt`, `/bar/foo/bar.txt`

### 디렉터리 계층 구조 예시
```
/
├── home
│   └── user
│       └── file.txt
└── var
    └── log
```


### 42.2 파일 시스템 인터페이스

- 파일 이름이 중요하다: 시스템 내 대부분의 자원이 이름으로 식별됨.
- 모든 파일은 하나의 디렉터리 트리에 포함된다.

### 주요 시스템 콜
| 기능            | 시스템 콜              |
|-----------------|------------------------|
| 파일 열기       | open()                 |
| 파일 닫기       | close()                |
| 데이터 읽기     | read()                 |
| 데이터 쓰기     | write()                |
| 파일 삭제       | unlink()               |
| 디렉터리 생성   | mkdir()                |
| 디렉터리 삭제   | rmdir()                |


### 42.3 파일의 생성

\`\`\`c
int fd = open("foo", O_CREAT | O_WRONLY | O_TRUNC);
\`\`\`

- open()은 파일을 생성하거나 열고, **파일 디스크립터(fd)** 를 반환.
- creat()는 open()의 축약형. (역사적으로는 오타 포함)
- fd는 capability 개념으로, 해당 파일에 접근할 권한을 의미함.


```c
int fd = open("/tmp/data", O_WRONLY | O_CREAT | O_TRUNC, 0666);
```

### 접근 권한
- **0666:** 사용자, 그룹, 기타 모두 읽기/쓰기 허용
- 권한은 생성 시 설정하거나, `chmod()` 로 변경 가능

---

### 42.4 파일의 읽기와 쓰기

- \`read(fd, buf, size)\`, \`write(fd, buf, size)\` 로 데이터 읽기/쓰기 수행.
- strace를 이용해 시스템 콜 흐름을 추적 가능

\`\`\`bash
prompt> echo hello > foo
prompt> strace cat foo
open("foo", O_RDONLY|O_LARGEFILE) = 3
read(3, "hello\n", 4096) = 6
write(1, "hello\n", 6) = 6
\`\`\`

- fd 0,1,2는 표준입력, 표준출력, 표준에러
- cat은 open → read → write → close 순서로 작동

### 42.5 비 순차적 읽기와 쓰기

- 파일을 처음부터 끝까지 읽는 **순차적 접근** 외에, 임의 위치를 지정하는 **비순차적 접근**도 필요함.
- `lseek(fd, offset, whence)` 시스템 콜 사용:
  - `SEEK_SET`: 파일 시작부터 offset만큼 이동
  - `SEEK_CUR`: 현재 위치에서 offset만큼 이동
  - `SEEK_END`: 파일 끝에서 offset만큼 이동
- lseek은 디스크 탐색과는 관계 없음. 커널 내 메모리 변수만 갱신.

### 42.6 fsync()를 이용한 즉시 기록

- `write()`는 쓰기 요청을 메모리에만 저장할 수 있어 데이터 유실 위험 존재.
- `fsync(fd)`를 호출하면 해당 파일의 모든 변경 내용을 **즉시 디스크에 기록**.
- 디렉터리 자체도 fsync() 해야 안전한 저장 보장.

\`\`\`c
int fd = open("foo", O_CREAT | O_WRONLY | O_TRUNC);
write(fd, buffer, size);
fsync(fd);
close(fd);
\`\`\`

- 데이터뿐 아니라 **디렉터리 메타데이터도 fsync()** 해야 완전한 보장 가능.

### 42.7 파일 이름 변경

- `rename(old, new)` 시스템 콜을 사용.
- 원자적 동작 보장: 크래시가 발생해도 중간 상태는 없음.
- 안전한 저장을 위해 파일을 새 이름으로 저장한 뒤 rename으로 덮어씀.

\`\`\`c
int fd = open("foo.txt.tmp", O_WRONLY|O_CREAT|O_TRUNC);
write(fd, buffer, size);
fsync(fd);
close(fd);
rename("foo.txt.tmp", "foo.txt");
\`\`\`

### 42.8 파일 정보 추출

- `stat(path, &statbuf)`, `fstat(fd, &statbuf)` 사용
- 메타데이터: 파일 크기, 소유자, 권한, 접근/수정/변경 시간 등

\`\`\`c
struct stat {
  ino_t st_ino;
  off_t st_size;
  nlink_t st_nlink;
  time_t st_atime, st_mtime, st_ctime;
  ...
};
\`\`\`

- `stat file` 명령으로 확인 가능

### 42.9 파일 삭제

- `unlink(path)` 시스템 콜 사용
- 파일 이름을 디렉터리에서 제거, **아이노드의 참조 횟수** 감소
- 참조 수가 0이면 실제 데이터 해제
- `rm` 명령어는 내부적으로 unlink() 호출

\`\`\`bash
prompt> echo hello > file
prompt> rm file
prompt> strace rm file
unlink("file") = 0
\`\`\`


### 42.10 디렉터리 생성

- 디렉터리는 메타데이터로 직접 쓸 수 없음.
- `mkdir(path, mode)` 시스템 콜 사용
- 새 디렉터리는 기본적으로 `.` (자기 자신), `..` (부모 디렉터리) 항목 포함

```bash
prompt> mkdir foo
prompt> ls -a foo
.  ..
```

### 42.11 디렉터리 읽기

- `opendir()`, `readdir()`, `closedir()`를 통해 디렉터리 항목을 열람 가능

```c
DIR *dp = opendir(".");
struct dirent *d;
while ((d = readdir(dp)) != NULL)
    printf("%d %s\n", (int) d->d_ino, d->d_name);
closedir(dp);
```

- `struct dirent`는 파일 이름과 아이노드 번호 포함

### 42.12 디렉터리 삭제하기

- `rmdir(path)` 시스템 콜 사용
- **비어 있는 디렉터리만** 삭제 가능
  - `.`과 `..` 외에 다른 항목이 있으면 실패

### 42.13 하드 링크 (Hard Link)

- `link(oldpath, newpath)` 사용하여 **새로운 이름**으로 파일 접근 가능
- 아이노드 번호 공유 (내용은 공유, 이름만 다름)
- 참조 수가 0이 되면 실제로 삭제됨

```bash
prompt> echo hello > file
prompt> ln file file2
prompt> ls -i file file2
67158084 file
67158084 file2
```

- 참조 횟수는 `stat()`로 확인 가능

### 42.14 심볼릭 링크 (Symbolic Link)

- `ln -s target linkname` 으로 생성
- 별도의 파일로 **경로명 문자열을 저장**
- 다른 파일 시스템, 디렉터리에도 링크 가능

```bash
prompt> echo hello > file
prompt> ln -s file file2
prompt> stat file2
... symbolic link ...
```

- `ls -l` 출력에서 `l`로 표시되며, 연결된 경로명도 보임
- Dangling reference 문제: 원본이 삭제되면 링크는 끊긴 채 남음

```bash
prompt> rm file
prompt> cat file2
cat: file2: No such file or directory
```
---

### 42.15 파일 시스템 생성과 마운트

- 여러 개의 파일 시스템(디스크 파티션)을 하나의 디렉터리 트리로 구성
- `mkfs` (make filesystem): 빈 파일 시스템 생성
- `mount` 명령어 (또는 `mount()` 시스템 콜):

```bash
prompt> mount -t ext3 /dev/sda1 /home/users
```

- 마운트 후 `/home/users` 아래에서 새 파일 시스템에 접근 가능

```bash
prompt> ls /home/users
a  b
prompt> ls /home/users/a
foo
```

- `mount` 명령어로 현재 마운트된 파일 시스템 확인

```bash
/dev/sda1 on / type ext3 (rw)
/dev/sda5 on /tmp type ext3 (rw)
```

> 📌 유닉스는 여러 파일 시스템을 **단일 디렉터리 구조**로 통합 (윈도우의 드라이브(C:, D:)와 대조적)

---

### 42.16 요약

- 유닉스 파일 시스템은 이름 기반 인터페이스를 제공하며, 기본 시스템 콜로는 다음이 있음:
  - 파일: `open()`, `read()`, `write()`, `close()`, `unlink()`, `lseek()`, `fsync()`
  - 디렉터리: `mkdir()`, `opendir()`, `readdir()`, `rmdir()`
  - 링크: `link()`, `unlink()`, `rename()`, `symlink()`
  - 정보 추출: `stat()`, `fstat()`
- 파일 시스템은 아이노드 기반 구조로 되어 있으며, 하드링크와 심볼릭 링크를 통해 다중 경로 접근 가능
- 여러 파일 시스템은 마운트를 통해 하나의 계층 구조로 통합됨

---


> 📚 출처: 『운영체제: 아주 쉬운 세 가지 이야기 (OSTEP)』


