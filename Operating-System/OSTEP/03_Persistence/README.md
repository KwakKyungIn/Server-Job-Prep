# 💾 영속성 (Persistence)

> 이 문서는 OSTEP의 **영속성 단원**을 기반으로, 운영체제가 **디스크 I/O**, **파일 시스템**, **저장 장치 추상화**를 어떻게 구현하는지를 정리한 문서입니다.  
> 주요 주제는 **디스크 구조**, **파일 시스템 구현 방식**, **저널링**, **캐싱**, **신뢰성 문제 해결 기법**입니다.

---

## 📂 정리될 예정인 목차

| 번호 | 제목 | 설명 | 링크 (예정) |
|------|------|------|--------------|
| 35   | I/O 장치 소개 | 디스크와 장치 드라이버 기초 | *(작성 예정)* |
| 36   | 디스크 스케줄링 | 디스크 접근 순서 최적화 알고리즘 | *(작성 예정)* |
| 37   | RAID | 신뢰성과 성능을 높이기 위한 디스크 배열 방식 | *(작성 예정)* |
| 38   | 파일 시스템 인터페이스 | 파일 생성, 삭제, 열기, 읽기 등의 API | *(작성 예정)* |
| 39   | 파일 시스템 구현 | 디스크에 파일을 저장하는 자료구조 구조 | *(작성 예정)* |
| 40   | 크래시 일관성 | 시스템 장애 후 데이터 보호 방법 | *(작성 예정)* |
| 41   | 저널링 | 메타데이터 무결성을 위한 복구 전략 | *(작성 예정)* |
| 42   | 캐시와 쓰기 전략 | 버퍼 캐시, write-through 등 전략들 | *(작성 예정)* |
| 43   | 파일 시스템 정리 | 주요 개념 및 설계 전략 요약 | *(작성 예정)* |

---

## ✍️ 참고 사항

- 파일 시스템은 서버 프로그래밍에서 직접적으로 드러나진 않지만,
  **데이터 저장, 무결성, 고가용성**과 직결되는 주제로 매우 중요합니다.
- 실무에서 Redis, MySQL, RocksDB 등으로 이어지는 기반 개념이므로 반드시 숙지할 것.
