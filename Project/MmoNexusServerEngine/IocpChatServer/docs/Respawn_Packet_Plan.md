# Respawn Packet 설계안 (기획서)

## 목적
- 사망 상태 해제(부활)를 **아이템/HP 회복과 분리된 별도 패킷**으로 처리한다.
- 클라이언트는 임의로 부활 상태를 바꾸지 못하고, **서버 승인** 후에만 부활한다.
- 전투/이동/스킬 로직의 권위를 서버로 고정한다.

---

## 패킷 설계(초안)

### C_RESPAWN_REQ
클라이언트 → 서버  
부활을 요청한다.

- `uint32 client_time_ms` (선택)  
  - 재전송/지연 분석용
- `uint32 revive_type` (선택)  
  - 0=기본/마을 부활, 1=현장 부활, 2=아이템 부활 등

### S_RESPAWN_RES
서버 → 클라이언트  
부활 결과를 요청자에게 회신한다.

- `bool success`
- `int32 reason`  
  - 0=OK  
  - 1=NOT_DEAD  
  - 2=INVALID_STATE  
  - 3=COOLDOWN  
  - 4=NO_ITEM  
  - 5=IN_DUNGEON_RESTRICT  
  - 6=IN_TRADE  
  - 7=IN_MAP_CHANGE  
  - 8=INTERNAL
- `int32 respawn_map_id`
- `int64 respawn_instance_id`
- `PositionInfo respawn_pos`

### S_RESPAWN_NTF (옵션)
서버 → 주변 플레이어  
부활 상태 변경을 브로드캐스트한다.

- `uint64 objectId`
- `PositionInfo posInfo` (actionState=IDLE, state=IDLE)

> 비고: 현재는 S_MOVE로 actionState를 전파 중이라,
> `S_RESPAWN_NTF`는 선택사항. 도입 시 S_MOVE 대체 여부 검토 필요.

---

## 서버 처리 플로우

1) **유효성 검증**
- 플레이어가 실제로 사망 상태인지 확인  
  - `hp <= 0` 또는 `actionState == DEAD`
- 맵 전환 중인지 / 거래 중인지 / 던전 제한 중인지 확인

2) **부활 위치 결정**
- 기본: 저장된 귀환 위치 또는 기본 월드 스폰
- 던전 내 제한 정책이 있다면 강제로 월드로 복귀

3) **상태 갱신**
- `hp = maxHp`
- `actionState = IDLE`
- `posInfo` 업데이트
- 이동 검증/쿨타임 상태 초기화 필요 시 처리

4) **전파**
- 본인: `S_RESPAWN_RES`  
  - 성공 시 respawn 위치 포함
- 주변: `S_RESPAWN_NTF` 또는 `S_MOVE`로 actionState/pos 전파

---

## 클라이언트 처리 플로우

1) 사망 시 UI에서 “부활” 버튼 활성화  
2) 버튼 클릭 → `C_RESPAWN_REQ`
3) `S_RESPAWN_RES` 수신
   - 성공: 위치 워프 + 애니메이션 리셋 + 입력 해제
   - 실패: 이유 메시지 표시
4) 주변 플레이어는 `S_RESPAWN_NTF` 혹은 `S_MOVE`로 애니 복귀

---

## 정책/제약 사항 (추천)

- **쿨타임**: 사망 후 N초는 부활 불가
- **아이템 소모**: 현장 부활은 아이템 필요
- **던전/인스턴스 제약**: 특정 던전은 부활 불가 혹은 강제 귀환
- **전투 중 제약**: PvP/PvE 규칙 반영

---

## 테스트 시나리오

- 사망 상태에서 C_RESPAWN_REQ → 성공/실패 분기 확인  
- 사망 상태가 아닌데 요청 → 실패 처리  
- 던전 제한, 거래 중, 맵 변경 중 요청 → 실패 처리  
- 부활 직후 주변 플레이어의 애니/상태 반영 확인

---

## 구현 체크리스트 (추후 작업)

- Protocol.proto / PacketGenerator 갱신
- 서버: 핸들러/상태 머신 추가
- 클라: UI + 응답 처리 + 애니 리셋
- 문서화 및 테스트 로그 추가
