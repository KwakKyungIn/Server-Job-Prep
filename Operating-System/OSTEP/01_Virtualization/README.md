# 🧠 가상화 (Virtualization)

> 이 문서는 OSTEP의 **가상화 단원**을 기반으로 운영체제의 CPU, 메모리, 주소 공간 가상화 개념을 정리한 것입니다.  
> 각 장은 **핵심 질문 → 상세 개념 → 요약** 순서로 구성되어 있습니다.

---

## 📂 정리된 목차

| 번호 | 제목 | 설명 | 링크 |
|------|------|------|------|
| 05   | 운영체제 개요 | 운영체제가 왜 필요한가? 무엇을 가상화하는가? | [05-overview.md](./05-overview.md) |
| 07   | 프로세스 | 실행 중인 프로그램의 구조와 상태 | [07-process.md](./07-process.md) |
| 08   | 프로세스 API | fork, exec, wait 등 프로세스 제어를 위한 시스템 콜 | [08-process-api.md](./08-process-api.md) |
| 09   | 제한적 직접 실행 | 사용자 프로그램을 직접 실행하면서도 OS가 제어권을 유지하는 방법 | [09-limited-execution.md](./09-limited-execution.md) |
| 10   | 스케줄링 개요 | 다양한 CPU 스케줄링 알고리즘 소개 | [10-scheduling-overview.md](./10-scheduling-overview.md) |
| 11   | MLFQ | 멀티 레벨 피드백 큐 스케줄러 설계 및 적용 방식 | [11-mlfq.md](./11-mlfq.md) |
| 12   | 추첨 스케줄링 | 비례 기반 스케줄링(Lottery, Stride) 기법 소개 | [12-lottery.md](./12-lottery.md) |
| 13   | 멀티프로세서 스케줄링 | 다중 CPU 환경에서의 스케줄링 전략 | [13-multiprocessor.md](./13-multiprocessor.md) |
| 16   | 가상 메모리 소개 | 가상 메모리 개념 및 메모리 주소 공간 구조 | [16-vm-intro.md](./16-vm-intro.md) |
| 17   | 메모리 API | malloc, free 등 동적 메모리 할당 API의 동작 방식 | [17-vm-api.md](./17-vm-api.md) |

---

## ✍️ 참고 사항

- 각 파일은 OSTEP의 챕터별 순서대로 정리되어 있으며,  
  각 문서는 **핵심 질문 → 상세 개념 및 예시 → 총 요약** 흐름으로 구성되어 있습니다.
- 면접용 질문 정리는 별도 문서로 분리 예정입니다.
