# Page 6 - 문제 정의: 서버 권위 이동이 필요한 이유

## 페이지 목표
- 클라이언트 신뢰 모델이 왜 바로 취약점으로 연결되는지 1페이지에서 납득시킨다.
- "입력은 클라이언트, 상태 확정은 서버"라는 원칙을 명확히 박는다.

## 권장 문서 배치
- 좌측(45%): 위협 시나리오 3종
- 우측(40%): 서버 권위 검증 원칙
- 하단(15%): 코드 근거(파일/함수 경로)

## 본문 문안 (복붙용)
### 1) 클라이언트 신뢰 시 발생 문제
- 속도핵/텔레포트: 비정상 delta를 그대로 반영하면 짧은 시간에 과도한 거리 이동이 가능
- 벽 통과: 충돌 지형을 무시한 좌표를 반영하면 불가능 지점 진입 허용
- 패킷 역전: 과거 seq 패킷이 최신 상태를 덮어써 위치 롤백/순간이동 발생

### 2) 서버 권위 설계 원칙
- 서버가 `seq`, `dt`, `speed`, `navmesh`를 순서대로 검증
- 비정상 요청은 drop 또는 보정(clamp/slide)
- 검증 완료 좌표만 최종 커밋하고 `S_MOVE`로 브로드캐스트

### 3) 문서 핵심 메시지
- "클라이언트는 제안하고, 서버가 확정한다."

## 코드 근거 (하단 표기용)
- 패킷 진입/룸 직렬화: `GameServer/ClientPacketHandler.GamePlay.cpp:8`
- 이동 검증 메인: `GameServer/GameRoom.Move.cpp:29`
- 시퀀스/시간/속도 유틸: `GameServer/MoveValidationUtils.h:19`, `GameServer/MoveValidationUtils.h:28`, `GameServer/MoveValidationUtils.h:57`
- NavMesh 검증 호출: `GameServer/GameRoom.Move.cpp:117`, `GameServer/GameMap.cpp:53`

## 제출본 캡션 예시
- "이동 패킷은 입력으로만 취급하고, 서버가 순서/시간/속도/지형 검증을 통과한 좌표만 최종 커밋한다."
