# Page 7 - 이동 검증 파이프라인

## 페이지 목표
- 코드 실행 순서 그대로 검증 단계를 보여준다.
- "어디서 drop되고, 어디서 보정되는지"를 시각적으로 명확히 전달한다.

## 권장 문서 배치
- 좌측(70%): Mermaid 흐름도 (검증 단계 + 분기)
- 우측(30%): Fail-Fast 조건표 3개
- 하단: 함수 경로 한 줄

## 본문 문안 (복붙용)
### 파이프라인 단계
1. `C_MOVE` 수신 후 RoomActor 큐에 enqueue (동일 룸 직렬 처리)
2. NaN/Inf/Out-of-bounds 입력 즉시 거부
3. `IsSeqNewer`로 역순/중복 패킷 드롭
4. `ComputeDtSec`로 서버 기준 dt 안정화
5. `CheckSpeed2D`로 최대 허용 거리 초과 시 clamp
6. `ValidateMove`로 NavMesh 충돌/슬라이딩 보정
7. 서버 좌표 commit + Zone/AOI 갱신
8. 확정 좌표만 `S_MOVE`로 주변 전파
9. 마지막 move stamp 갱신

### Fail-Fast 조건표 (우측)
- 역순 seq: 과거 패킷 드롭
- 과속 이동: 거리 clamp
- 비가용 지형: NavMesh 불가 시 거부/보정

## 코드 경로 (하단 표기용)
- `GameServer/ClientPacketHandler.GamePlay.cpp:8`
- `GameServer/GameRoom.Move.cpp:45`
- `GameServer/GameRoom.Move.cpp:75`
- `GameServer/GameRoom.Move.cpp:90`
- `GameServer/GameRoom.Move.cpp:102`
- `GameServer/GameRoom.Move.cpp:117`
- `GameServer/GameRoom.Move.cpp:155`
- `GameServer/GameRoom.Move.cpp:168`

## 제출본 캡션 예시
- "입력 안정성→순서/시간→속도→지형 검증을 순차 적용해, 비정상 이동 요청을 drop/clamp하고 확정 좌표만 공유 상태로 반영한다."
