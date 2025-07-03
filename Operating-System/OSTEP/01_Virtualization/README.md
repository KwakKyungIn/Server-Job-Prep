# 🧠 가상화 (Virtualization)

> 이 문서는 OSTEP의 **가상화 단원**을 기반으로 운영체제의 CPU, 메모리, 주소 공간 가상화 개념을 정리한 것입니다.  
> 각 장은 **핵심 질문 → 상세 개념 → 요약** 순서로 구성되어 있습니다.

---

## 📂 정리된 목차

| Chapter | 제목 | 설명 | 링크 |
|---------|------|------|------|
| 05 | 운영체제 개요 | 운영체제가 왜 필요한가? 무엇을 어떻게 가상화하는가? | [05-overview.md](./05-overview.md) |
| 07 | 프로세스 | 프로세스의 구조와 상태, 메모리 계층 이해 | [07-process.md](./07-process.md) |
| 08 | 프로세스 API | fork, exec, wait 등 시스템 콜을 통한 프로세스 제어 | [08-process-api.md](./08-process-api.md) |
| 09 | 제한적 직접 실행 | 사용자 프로그램 실행과 운영체제의 제어권 확보 | [09-limited-execution.md](./09-limited-execution.md) |
| 10 | 스케줄링 개요 | FIFO, RR 등 CPU 스케줄링 기본 정책 | [10-scheduling-overview.md](./10-scheduling-overview.md) |
| 11 | MLFQ | 멀티 레벨 피드백 큐를 통한 동적 우선순위 스케줄링 | [11-mlfq.md](./11-mlfq.md) |
| 12 | 추첨 스케줄링 | 확률 기반 스케줄링 기법: Lottery, Stride | [12-lottery.md](./12-lottery.md) |
| 13 | 멀티프로세서 스케줄링 | 다중 CPU 환경에서의 스케줄링 전략 | [13-multiprocessor.md](./13-multiprocessor.md) |
| 16 | 가상 메모리 소개 | 주소 공간 가상화 개념 및 메모리 계층 구조 | [16-vm-intro.md](./16-vm-intro.md) |
| 17 | 메모리 API | malloc, free를 통한 동적 메모리 관리 | [17-vm-api.md](./17-vm-api.md) |
| 18 | 가상화 메커니즘 | 트랩, 커널 모드, 타이머를 통한 제어 메커니즘 | [18-vm-mechanism.md](./18-vm-mechanism.md) |
| 19 | 세그멘테이션 | 메모리 분할 및 보호를 위한 세그먼트 구조 | [19-vm-segmentation.md](./19-vm-segmentation.md) |
| 20 | 가용 공간 관리 | 메모리 할당 기법과 free space 문제 | [20-vm-freespace.md](./20-vm-freespace.md) |
| 21 | 페이징 기초 | 고정 크기 페이지 기반 메모리 가상화 | [21-vm-paging.md](./21-vm-paging.md) |
| 22 | TLB | 주소 변환 속도를 높이기 위한 Translation Lookaside Buffer | [22-vm-tlb.md](./22-vm-tlb.md) |
| 23 | 작은 페이지 테이블 | 다단계 페이지 테이블로 메모리 절약 | [23-vm-smalltables.md](./23-vm-smalltables.md) |
| 24 | 물리 메모리를 넘어서 | 스와핑과 디스크 기반 메모리 확장 | [24-vm-beyondphys.md](./24-vm-beyondphys.md) |
| 25 | 교체 알고리즘 | LRU, FIFO 등 페이지 교체 전략 | [25-vm-replacement.md](./25-vm-replacement.md) |
| 26 | VAX 사례 | 실제 시스템(VAX/VMS)의 가상 메모리 설계 사례 | [26-vm-vax.md](./26-vm-vax.md) |

---

## ✍️ 참고 사항

- 각 파일은 OSTEP의 챕터별 순서대로 정리되어 있으며,  
  각 문서는 **핵심 질문 → 상세 개념 및 예시 → 총 요약** 흐름으로 구성되어 있습니다.
- 면접용 질문 정리는 별도 문서로 분리 예정입니다.
