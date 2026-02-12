## **Section 6: 신뢰성 사례 & 마무리** (2페이지)

**Page 36 - 안정성 도구 + 트러블슈팅**
```text
운영 안정화 도구
- DeadLockProfiler
- ASSERT_CRASH
- StompAllocator

사례 1) 투사체 중복 히트
- 원인: hitTargets 갱신 타이밍
- 해결: 판정 직후 갱신

사례 2) 거래 중 아이템 중복
- 원인: 거래 중 인벤 조작 허용
- 해결: 거래 상태머신에서 인벤 조작 차단
```

**Page 37 - 역량 요약 & 참고 링크**
```text
핵심 역량
1) 서버 권위/정합성 중심 설계
2) IOCP + JobQueue 기반 동시성 제어
3) 운영 관측/부하 검증까지 포함한 개발

참고 링크
- GitHub: github.com/yourname
- 문서: 아키텍처/모니터링/부하테스트 보고서
- 선택: 동작 영상(보조 자료)
```
