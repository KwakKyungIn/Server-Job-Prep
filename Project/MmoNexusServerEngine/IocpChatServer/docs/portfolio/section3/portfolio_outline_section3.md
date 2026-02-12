## **Section 3: 서버 엔진 설계** (6페이지)

**Page 22 - Actor + JobQueue 직렬화 모델**
- Mermaid 다이어그램 필요
```text
핵심 아이디어
- 액터 상태를 단일 실행 경로로 직렬화
- 세션은 PostRoom()으로 액터에 작업 위임

효과
- 데이터 레이스 감소
- 액터 로직 복잡도 제어
```

**Page 23 - Hybrid 스레드 모델**
- Mermaid 다이어그램 필요
```text
IOCP Dispatch 스레드 + 로직 워커 분리
- 네트워크 I/O와 게임 로직 경합 분리
- GlobalQueue가 JobQueue 실행 분배
```

**Page 24 - IOCP 네트워크 레이어**
- Mermaid 다이어그램 필요
```text
Windows IOCP + Overlapped 소켓
- Accept/Recv/Send 비동기 처리
- Session 기반 Dispatch
- 송신 백로그 임계치로 폭주 완화
```

**Page 25 - 패킷 파이프라인**
- Mermaid 다이어그램 필요
```text
PacketHeader: size/id/crc/seq
  ↓
핸들러 테이블 디스패치
  ↓
Protobuf 직렬화

부가 도구
- PacketGenerator(Python+Jinja2)
```

**Page 26 - 로비 데이터 로딩 게이트**
```text
입장 게이트
- Stat 로딩
- Items 로딩
- QuickSlot 로딩
모두 완료 시 월드 입장 허용

실패 시
- 입장 실패 응답 + 세션 정리
```

**Page 27 - 세션 생명주기 관리**
```text
sessionId ↔ playerId 바인딩
- MapChanging FSM 관리
- C_MAP_CHANGE_ACK 토큰 검증
- Disconnect 시 Dirty 마킹/Flush/인스턴스 정리
```

---

