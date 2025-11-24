# 💾 영속성 (Persistence)

> 이 문서는 OSTEP의 **영속성 단원**을 기반으로, 운영체제가 **데이터를 디스크에 안전하게 저장하고, 복구하며, 관리하는 전 과정을** 정리한 공간입니다.  
> 특히 서버 프로그래밍과 밀접한 관계가 있는 **저장 장치 인터페이스, 파일 시스템 구조, 캐시 전략, 크래시 복구 메커니즘** 등 실전에서 자주 마주치는 개념들을 깊이 있게 다룹니다.

---

## 🧠 왜 따로 정리했을까?

- 보통 OS 공부는 프로세스, 메모리, 스케줄링, 동기화까지만 끝내는 경우가 많습니다.  
  하지만 **서버 개발자**가 되기 위해서는 "데이터를 어떻게 안정적으로 저장할 것인가"에 대한 이해가 필수입니다.

- 특히 다음과 같은 실무/면접 상황에서 반드시 등장합니다:
  - "파일 시스템은 왜 block 단위로 관리하나요?"
  - "RAID 레벨 중 서버에서 자주 쓰는 조합은?"
  - "ext3는 왜 저널링을 쓰고, 크래시 상황에서 어떤 방식으로 복구하나요?"
  - "flush와 fsync의 차이는?"
  - "무결성을 보장하려면 어떤 체크 방식이 필요할까요?"

- 이 문서는 위 질문들에 대해, OSTEP 기반으로 탄탄히 정리한 **실전 면접 대비용 + 심화 학습용** 자료입니다.

---

## 🗂️ 문서 구성 방식

- 챕터별로 하나의 `.md` 파일로 정리되어 있으며,  
  각 파일은 다음과 같은 구성 흐름을 따릅니다:

  1. **핵심 질문** – 이 장에서 꼭 이해해야 할 본질적인 문제 제기  
  2. **상세 개념 정리** – OSTEP 책 내용을 기반으로 한 구조적 설명과 예시  
  3. **요약 정리 표** – 실전 대비를 위한 빠른 복습용 요약  

---

## 📂 정리된 목차

| 번호 | 제목 | 설명 | 링크 |
|------|------|------|------|
| 39 | I/O 장치 소개 | 저장 장치와 디바이스 드라이버의 기본 개념 소개 | [39-persistence-devices.md](./39-persistence-devices.md) |
| 40 | 디스크 스케줄링 | 디스크 구조와 접근 시간 최적화를 위한 스케줄링 알고리즘 | [40-persistence-disks.md](./40-persistence-disks.md) |
| 41 | RAID | 데이터 신뢰성과 성능 향상을 위한 RAID 구조 소개 | [41-persistence-raid.md](./41-persistence-raid.md) |
| 42 | 파일 및 디렉터리 인터페이스 | 파일 생성, 삭제, 열기, 경로 해석 등 시스템 콜 인터페이스 | [42-persistence-file-directory.md](./42-persistence-file-directory.md) |
| 43 | 파일 시스템 구현 | 파일 시스템 내부 구조와 인덱스 노드 기반 저장 방식 | [43-persistence-fs.md](./43-persistence-fs.md) |
| 44 | 지역성과 Fast File System | 지역성과 성능을 고려한 FFS의 설계 및 구현 전략 | [44-persistence-ffs.md](./44-persistence-ffs.md) |
| 45 | 크래시 일관성 | 시스템 장애 후 메타데이터 및 데이터 정합성 보장 기법 | [45-persistence-crash-consistency.md](./45-persistence-crash-consistency.md) |
| 46 | Log-structured File System | 쓰기 최적화 중심의 로그 기반 파일 시스템 구조 | [46-persistence-logstructured.md](./46-persistence-logstructured.md) |
| 47 | 데이터 무결성 | 오류 검출, 중복성, 체크섬 등을 통한 데이터 무결성 보장 | [47-persistence-integrity.md](./47-persistence-integrity.md) |
| 50 | 분산 시스템 개요 | 네트워크 환경에서 데이터 공유 및 일관성을 위한 분산 파일 시스템 기초 | [50-persistence-distributed-systems.md](./50-persistence-distributed-systems.md) |
| 51 | NFS (Sun의 네트워크 파일 시스템) | 무상태 서버 기반 NFS 구조 및 캐시 일관성 기법 분석 | [51-persistence-nfs-sun.md](./51-persistence-nfs-sun.md) |
| 52 | Andrew File System (AFS) | 콜백 기반 캐시 일관성과 전체 파일 캐싱 방식의 확장성 설계 | [52-persistence-afs.md](./52-persistence-afs.md) |

---

## 🧭 마지막으로

- 이 정리 문서는 단순한 정리 이상의 목적을 갖습니다.  
  실제로 서버 개발 중 발생하는 디스크 I/O 이슈, 데이터 유실, 파일 시스템 병목 같은 문제를 정확히 이해하고 **문제 해결력을 갖추기 위한 기반**입니다.
