# 게임 기능 목록 (Feature List)

| 현재 구현 완료 기능 | 부분 구현 | 계획 |
|---|---|---|
| 로그인 토큰 검증·게임 입장 게이트<br>채널/맵 이동 핸드셰이크<br>서버 권위 이동 검증·NavMesh 슬라이딩<br>AOI 스냅샷/Spawn·Despawn 동기화<br>투사체 전투/피격 처리<br>스킬 쿨타임 서버 검증<br>파티 생성·초대·채팅·상태 스냅샷<br>파티 기반 인스턴스 던전 + 강제 귀환/강퇴 처리<br>거래 상태머신 + 골드/아이템 + DB 원자 커밋<br>인벤/장비/퀵슬롯/드래그앤드롭<br>로컬 채팅<br>Redis 기반 자동 저장 | 로그인 비밀번호 검증 미사용<br>스킬 타입 확장(원형/부채꼴 등) 미완<br>플레이어 부활/몬스터 리젠/드랍 테이블 미완<br>DB 로딩 실패 처리 미완<br>하트비트 타임아웃/지연 측정 미구현 | 계정 생성/가입 플로우<br>채팅 로그/모니터링 고도화 |

## 1) 계정/로그인/세션
### 로그인 인증 및 토큰 발급 (상태: 부분 구현)
**유저 관점**
- 아이디/비밀번호로 로그인하면 성공 시 서버 리스트와 접속 토큰을 받는다.
- 발급된 토큰으로 게임 서버에 접속하며, 토큰이 유효하지 않으면 접속이 끊긴다.

**서버 내부 흐름**
1. LoginServer가 `C_LOGIN`을 받고 `userId`만 검사한다.
2. `S2S_REQ_LOGIN`을 DBAgent로 비동기 전송한다.
3. DBAgent가 `Players` 테이블에서 이름으로 조회해 성공 여부와 `playerId`를 반환한다.
4. LoginServer가 토큰을 생성하고 Redis에 TTL 300초로 저장한다.
5. `S_LOGIN`에 토큰과 서버 리스트를 담아 클라이언트에 응답한다.

**검증/예외 케이스**
- `userId` 공백, DB 세션 끊김 시 실패 처리.
- 비밀번호는 현재 검증 로직에서 사용되지 않음.
- 유저 미존재 시 계정 생성 TODO만 주석으로 존재.

**근거**
- `LoginServer/ClientPacketHandler.cpp` - `ClientPacketHandler::Handle_C_LOGIN`
- `DBAgent/DBAgentPacketHandler.cpp` - `Handle_S2S_REQ_LOGIN` (SQL SELECT)
- `LoginServer/S2SPacketHandler.cpp` - 토큰 생성/Redis 저장/`S_LOGIN` 전송
- `ServerCore/RedisManager.h` - `Set/Get`
- `Common/Protobuf/bin/Protocol.proto` - `C_LOGIN`, `S_LOGIN`

### 게임 접속/세션 바인딩 + 로비 데이터 로딩 게이트 (상태: 완료)
**유저 관점**
- 로그인 토큰으로 게임에 접속하면 캐릭터/인벤/퀵슬롯이 동기화된 뒤 월드에 등장한다.
- 로딩이 끝나기 전까지는 로비에서 대기하며, 완료 시 자동 입장된다.

**서버 내부 흐름**
1. 클라이언트가 `C_ENTER_GAME`을 전송한다.
2. GameServer가 Redis에서 토큰을 조회해 `playerId`를 확정한다.
3. 세션에 `playerId`를 바인딩하고 Pending Enter(채널/맵/인스턴스)를 저장한다.
4. LobbyRoom이 Player 객체를 준비하고 DB 로딩 대기 상태로 둔다.
5. DBAgent에서 스탯/아이템/퀵슬롯 데이터를 받아 Redis 프라임 + 클라이언트 동기화를 수행한다.
6. 모든 로딩이 끝나면 GameRoom으로 입장시키고 `S_ENTER_GAME` 및 AOI 스냅샷을 전송한다.

**검증/예외 케이스**
- 토큰이 Redis에 없으면 즉시 연결 종료.
- 잘못된 맵/던전 ID는 기본 월드맵으로 보정.
- DB 로딩 실패 시 TODO 처리만 존재(실제 실패 응답 미구현).

**근거**
- `GameServer/ClientPacketHandler.EnterGame.cpp` - `Handle_C_ENTER_GAME`
- `GameServer/LobbyRoom.cpp` - `EnterGame`, `OnStatLoaded`, `OnItemsLoaded`, `OnQuickSlotsLoaded`, `TryEnterWorldIfReady`
- `GameServer/S2SPacketHandler.cpp` - `Handle_S2S_RES_LOAD_PLAYER_DATA`, `Handle_S2S_RES_ITEMS_LOAD`, `Handle_S2S_RES_QUICKSLOT_LOAD`
- `Common/Protobuf/bin/Protocol.proto` - `C_ENTER_GAME`, `S_ENTER_GAME`, `S_ITEM_LIST`, `S_QUICKSLOT_LIST`

### 세션 관리 및 맵 변경 FSM (상태: 완료)
**유저 관점**
- 맵 이동 중에는 입력이 제한되고, 이동 완료 후 정상적으로 게임이 이어진다.
- 접속 종료 시 캐릭터가 안전하게 퇴장되고 데이터가 저장된다.

**서버 내부 흐름**
1. `GameSessionManager`가 `sessionId ↔ playerId` 매핑을 유지한다.
2. `PlayerSession::TryBeginMapChange`로 맵 이동 상태를 전이하고 토큰을 보관한다.
3. `C_MAP_CHANGE_ACK` 수신 시 토큰을 검증하고 룸 전환을 수행한다.
4. 전환 완료 후 `EndMapChange`로 상태를 정상화한다.
5. 연결 종료 시 Dirty 마킹 + AutoCommit Flush, 룸에서 안전하게 제거한다.

**검증/예외 케이스**
- 이동 중 중복 요청은 무시.
- 토큰 불일치/현재 룸 없음 → 이동 취소.
- 연결 종료 시 인스턴스 멤버십 정리 수행.

**근거**
- `GameServer/PlayerSession.h` / `GameServer/PlayerSession.cpp` - 맵 변경 상태/FSM, `OnDisconnected`
- `GameServer/GameSessionManager.cpp` - 세션/플레이어 바인딩
- `GameServer/GameRoom.MapChange.cpp` - `TransferMapChangeById`

### 계정 생성/비밀번호 검증 (상태: 계획/스텁)
**유저 관점**
- 현재는 가입/비밀번호 검증 흐름이 제공되지 않는다.
- 로그인이 실패하면 단순 실패 응답만 받는다.

**서버 내부 흐름(현재 상태)**
1. `C_LOGIN`에 `password` 필드는 있으나 실제 검증에 사용되지 않는다.
2. DBAgent는 `Players` 테이블에서 이름만 조회한다.
3. 유저 미존재 시 `CreateAccount` TODO 주석만 존재한다.
4. 계정 생성용 S2S 패킷/핸들러가 없다.
5. DB INSERT 로직이 구현되어 있지 않다.

**검증/예외 케이스**
- 비밀번호 불일치 검증 없음.
- 신규 가입 불가.

**근거**
- `Common/Protobuf/bin/Protocol.proto` - `C_LOGIN` (password 필드)
- `LoginServer/ClientPacketHandler.cpp` - `reqPkt.set_password` 주석 처리
- `DBAgent/DBAgentPacketHandler.cpp` - `// TODO: CreateAccount`

## 2) 월드/룸/인스턴스/채널 구조
### 채널·로비·게임룸 구조 (상태: 완료)
**유저 관점**
- 채널을 선택하면 해당 채널의 로비/월드에 입장한다.
- 맵 크기, 스폰 위치, 시야 반경은 맵 설정값을 따른다.

**서버 내부 흐름**
1. `RoomManager`가 `RoomKey(channelId, mapId, instanceId)`로 룸을 관리한다.
2. 채널별 `LobbyRoom`을 생성해 접속자를 임시 수용한다.
3. `GameRoom::Init`이 `MapConfig`(크기/셀/네비메시/AOI)를 로딩한다.
4. 로딩이 끝난 플레이어를 로비에서 게임룸으로 이동시킨다.
5. `RoomManager::UpdateAll`이 각 룸의 Update를 주기적으로 수행한다.

**검증/예외 케이스**
- 잘못된 맵 ID는 기본 맵으로 보정.
- `Maps.json`이 없으면 하드코딩 기본값으로 초기화.

**근거**
- `GameServer/RoomManager.h` / `GameServer/RoomManager.cpp`
- `GameServer/LobbyRoom.cpp` - `EnterGame`, `TryEnterWorldIfReady`
- `GameServer/GameRoom.LifeTime.cpp` - `GameRoom::Init`
- `GameServer/DataManager.cpp` / `GameServer/Maps.json`

### 맵/채널 이동 핸드셰이크 (상태: 완료)
**유저 관점**
- 포털이나 채널 이동을 요청하면 로딩 후 새 위치로 이동한다.
- 이동 중 중복 입력은 무시된다.

**서버 내부 흐름**
1. `C_MAP_CHANGE_REQ` 또는 `C_CHANNEL_CHANGE_REQ`를 수신한다.
2. 대상 맵/채널 유효성 검사 후 맵 변경 토큰을 생성한다.
3. 세션 상태를 MapChanging으로 전이하고 `S_MAP_CHANGE_BEGIN`을 전송한다.
4. 클라이언트가 `C_MAP_CHANGE_ACK`을 보내면 토큰을 검증한다.
5. `TransferMapChangeById`로 룸을 이동시키고 `S_MAP_CHANGE_END`를 전송한다.

**검증/예외 케이스**
- 던전 인스턴스에서는 채널 이동을 차단.
- 맵 이동 중 추가 요청은 무시.
- 토큰 불일치 시 이동 취소.

**근거**
- `GameServer/ClientPacketHandler.MapChange.cpp`
- `GameServer/ClientPacketHandler.MapChangeUtil.cpp`
- `GameServer/GameRoom.MapChange.cpp`
- `Common/Protobuf/bin/Protocol.proto` - `C_MAP_CHANGE_REQ/ACK`, `S_MAP_CHANGE_BEGIN/END`, `C_CHANNEL_CHANGE_REQ`

### 인스턴스 던전(파티 기반) (상태: 완료)
**유저 관점**
- 파티가 던전에 입장하면 모두 같은 인스턴스로 이동한다.
- 던전 종료 시 원래 월드 위치로 복귀한다.

**서버 내부 흐름**
1. `C_DUNGEON_ENTER_REQ` 수신 후 던전 맵 ID를 검증한다.
2. PartyActor가 파티 상태를 확인하고 ENTERING 트랜지션을 건다.
3. InstanceActor가 `instanceId`를 생성/조회한다.
4. 파티 메타에 인스턴스 정보를 저장하고 전원에게 맵 이동을 시작한다.
5. `C_DUNGEON_EXIT_REQ` 시 인스턴스를 닫고 복귀 맵으로 이동시킨다.

**검증/예외 케이스**
- 파티 미가입/이미 던전 진행 중이면 실패.
- 맵 이동 중에는 요청 차단.
- 강퇴/해산 시 강제 귀환 처리.

**근거**
- `GameServer/ClientPacketHandler.Dungeon.cpp`
- `GameServer/InstanceActor.h` / `GameServer/InstanceManagerCore.cpp`
- `GameServer/PartyManagerCore.h` / `GameServer/PartyActor.cpp`
- `Common/Protobuf/bin/Protocol.proto` - `C_DUNGEON_ENTER_REQ`, `S_DUNGEON_ENTER_RES`, `C_DUNGEON_EXIT_REQ`, `S_DUNGEON_EXIT_RES`

### 인스턴스 룸 자동 정리 (상태: 완료)
**유저 관점**
- 인스턴스가 비면 일정 시간 뒤 자동 종료된다.
- 정상적인 월드 룸은 유지된다.

**서버 내부 흐름**
1. 인스턴스 종료 시 룸을 Closing 상태로 마킹한다.
2. `RoomManager::UpdateAll`에서 `PurgeInstanceRooms`가 정리 후보를 수집한다.
3. `GameRoom::ShouldPurge`가 인스턴스/빈룸/유예시간 조건을 검사한다.
4. 조건을 만족하면 RoomManager가 룸을 제거한다.
5. InstanceActor는 만료 인스턴스도 주기적으로 정리한다.

**검증/예외 케이스**
- 플레이어가 남아있는 룸은 정리하지 않음.
- Closing 상태가 아니면 정리 대상 제외.

**근거**
- `GameServer/GameRoom.LifeTime.cpp` - `ShouldPurge`
- `GameServer/RoomManager.cpp` - `PurgeInstanceRooms`
- `GameServer/InstanceActor.h` - `TickTimeout`

## 3) 이동/동기화/검증(서버 권위, 보정/클램프/슬라이딩 등)
### 서버 권위 이동 검증(시퀀스/시간/속도/클램프/슬라이딩) (상태: 완료)
**유저 관점**
- 이동이 부자연스럽거나 과속이면 서버가 보정한다.
- 벽을 뚫지 못하고 장애물에 닿으면 미끄러지듯 이동한다.

**서버 내부 흐름**
1. `C_MOVE` 수신 후 좌표 NaN/범위 초과를 검증한다.
2. 시퀀스/타임스탬프를 비교해 역순 패킷을 드롭한다.
3. `ComputeDtSec`로 dt를 계산하고 허용 범위로 클램프한다.
4. 속도 기반 최대 이동 거리 초과 시 위치를 클램프한다.
5. NavMesh `ValidateMove`로 슬라이딩 보정 후 위치를 확정한다.

**검증/예외 케이스**
- 좌표가 비정상/맵 밖이면 무시.
- 시퀀스 되감기 패킷 드롭.
- NavMesh 위가 아니면 이동 무시.

**근거**
- `GameServer/GameRoom.Move.cpp` - `HandleMove`
- `GameServer/MoveValidationUtils.h` - `IsSeqNewer`, `ComputeDtSec`, `CheckSpeed2D`
- `GameServer/GameMap.cpp` / `GameServer/NavSystem.cpp` - `ValidateMove`
- `Common/Protobuf/bin/Protocol.proto` - `C_MOVE`, `S_MOVE`

### AOI 스냅샷/가시성 동기화 (상태: 완료)
**유저 관점**
- 주변 플레이어/몬스터/투사체만 보이고, 멀어지면 자동으로 사라진다.
- 맵 입장 시 대량 스폰 정보가 스냅샷으로 전달된다.

**서버 내부 흐름**
1. `SpatialGrid`로 주변 Zone 후보를 수집한다.
2. 거리 및 Connectivity(벽/층) 필터링을 거쳐 가시 목록을 결정한다.
3. 스폰/디스폰 리스트를 배치로 나눠 `S_SPAWN`/`S_DESPAWN` 전송한다.
4. 스냅샷 시작/끝 플래그로 대량 로딩 구간을 표시한다.
5. Visible set을 갱신해 이후 이동/투사체 동기화에 사용한다.

**검증/예외 케이스**
- Connectivity ID 불일치 시 가시성 제외.
- 스냅샷 모드에서 빈 스냅샷도 begin/end 패킷 전송.

**근거**
- `GameServer/GameRoom.AOI.v2.cpp` - `UpdateAOI`, `SendSpawnBatchedToMe`, `SendDespawnBatchedToMe`
- `GameServer/SpatialGrid.h/.cpp`
- `Common/Protobuf/bin/Protocol.proto` - `S_SPAWN`, `S_DESPAWN`

## 4) 전투/스킬/투사체/피격/쿨타임
### 즉발 스킬 판정 및 피해 처리 (상태: 부분 구현)
**유저 관점**
- 기본 공격 등 즉발 스킬이 맞으면 HP가 감소한다.
- 범위형 스킬은 일부만 판정된다.

**서버 내부 흐름**
1. 클라이언트의 `C_SKILL`을 수신한다.
2. `DataManager`에서 `SkillTemplateInfo`를 조회한다.
3. `BattleSystem::ResolveSkill`이 주변 타겟을 수집하고 사거리 판정을 수행한다.
4. `Creature::OnDamaged`로 HP를 감소시킨다.
5. `S_SKILL`과 `S_CHANGE_HP`를 브로드캐스트한다.

**검증/예외 케이스**
- 스킬 데이터가 없으면 무시.
- `SKILL_AUTO`만 판정 로직 구현됨(원형/부채꼴 미구현).
- 쿨타임은 `CanUseSkill/StartSkillCooldown`으로 서버 권위 적용.

**근거**
- `GameServer/GameRoom.Combat.cpp` - `HandleSkill`
- `GameServer/BattleSystem.cpp` - `ResolveSkill`
- `GameServer/Creature.cpp` - `OnDamaged`
- `Common/Protobuf/bin/Enum.proto` - `SkillType`

### 투사체 스킬 시스템 (상태: 완료)
**유저 관점**
- 화살/파이어볼 같은 투사체가 이동하며 벽이나 적에 맞으면 피해를 준다.
- 관통/다단히트 여부가 스킬 데이터에 따라 달라진다.

**서버 내부 흐름**
1. `HandleSkill`에서 투사체 스킬을 감지해 `Projectile`을 생성한다.
2. 투사체는 속도/수명/사거리/히트 반경/관통 여부를 가진다.
3. `UpdateProjectiles`가 매 tick 위치를 갱신한다.
4. NavMesh 레이캐스트로 벽 충돌을 검사하고 소멸 여부를 판단한다.
5. 선분-원 충돌 판정으로 타격을 계산하고 `S_CHANGE_HP`를 전파한다.

**검증/예외 케이스**
- 스킬 데이터가 없으면 즉시 소멸.
- 소유자 사라짐 시 투사체 제거.
- 벽 충돌 시 위치 보정 후 소멸.

**근거**
- `GameServer/GameRoom.Combat.cpp` - 투사체 생성
- `GameServer/Projectile.cpp`
- `GameServer/GameRoom.Projectile.cpp`
- `Common/Protobuf/bin/Struct.proto` - `ProjectileInfo`

### 쿨타임/사망 처리 (상태: 부분 구현)
**유저 관점**
- 쿨타임은 클라이언트 UI에 표시되며 서버에서도 검증된다.
- 사망 상태는 전파되지만 자동 부활/리젠 흐름은 제공되지 않는다.

**서버 내부 흐름**
1. `Creature::CanUseSkill`로 쿨타임 만료를 검사한다.
2. `GameRoom::HandleSkill`에서 검증 후 `StartSkillCooldown`을 적용한다.
3. `S_SKILL`에 `cooldownMs`를 포함해 클라이언트 UI에 전달한다.
4. `Player::OnDead`가 상태를 `ACTION_DEAD`로 전파한다.
5. 포션 사용 시 HP 회복과 함께 사망 상태 해제 전파가 가능하다.

**검증/예외 케이스**
- 쿨타임 중 스킬 요청은 무시된다.
- 부활/리젠 스폰 로직 및 전용 패킷 흐름은 미구현이다.

**근거**
- `GameServer/Creature.cpp` - `CanUseSkill`, `UseSkill`
- `GameServer/GameRoom.Combat.cpp` - `S_SKILL` 전송
- `GameServer/Player.cpp` - `OnDead`
- `Common/Protobuf/bin/Struct.proto` - `SkillTemplateInfo`

## 5) 몬스터 AI(FSM/네비/타겟팅 등)
### 몬스터 FSM AI (상태: 완료)
**유저 관점**
- 몬스터가 주변 유저를 감지해 추격하고 공격한다.
- 멀어지면 스폰 위치로 복귀한다.

**서버 내부 흐름**
1. `GameRoom::Update`가 매 tick `Monster::Update`를 호출한다.
2. FSM 상태(Idle/Chase/Attack/Return)로 로직을 분기한다.
3. Idle에서 근처 플레이어를 탐색해 Chase로 전이한다.
4. Chase는 사거리 진입 시 Attack으로 전이한다.
5. Attack은 주기적으로 `HandleSkill`을 호출한다.

**검증/예외 케이스**
- 대상이 죽었거나 방을 나가면 타겟 해제.
- 리시 범위(Leash) 초과 시 Return 전이.

**근거**
- `GameServer/GameRoom.Monster.cpp` - `Update`
- `GameServer/Monster.h/.cpp` - FSM 상태/전이

### NavMesh 기반 추적/경로/LOS (상태: 완료)
**유저 관점**
- 몬스터가 장애물을 피해 이동하며 끼임이 발생하면 경로를 재계산한다.
- 직선 시야가 확보되면 곧장 추격한다.

**서버 내부 흐름**
1. `RebuildPathTo`가 `FindPathWaypoints`로 경로를 구한다.
2. `FollowPath`가 웨이포인트를 따라 이동한다.
3. 일정 시간마다 또는 끼임 감지 시 재경로를 수행한다.
4. Line of Sight가 확보되면 직선 이동으로 최적화한다.
5. 이동은 NavMesh `ValidateMove`로 보정된다.

**검증/예외 케이스**
- 경로 계산 실패 시 직선 이동으로 폴백.
- 끼임 감지(`stuckAccumMs`) 시 재경로.

**근거**
- `GameServer/Monster.cpp` - `RebuildPathTo`, `FollowPath`, `UpdateChase`
- `GameServer/GameMap.cpp` - `FindPathWaypoints`, `HasLineOfSight`
- `GameServer/NavSystem.cpp`

### 스폰/드랍/리젠 (상태: 부분 구현)
**유저 관점**
- 기본 몬스터가 스폰되고 처치 시 아이템이 자동 지급된다.
- 리젠/스폰 테이블은 확인되지 않는다.

**서버 내부 흐름**
1. `GameRoom::Init`에서 테스트 몬스터를 1마리 스폰한다.
2. `EnterMonster`가 AOI/Zone에 등록하고 스폰 패킷을 전송한다.
3. 사망 시 `HandleMonsterDead`가 경험치/드랍을 처리한다.
4. 몬스터는 `LeaveMonster`로 디스폰된다.
5. 리젠 타이머/스폰 테이블 로직은 미발견.

**검증/예외 케이스**
- 드랍 아이템 템플릿이 없으면 스킵.
- 인벤이 가득 차면 드랍 획득 실패(알림 없음).

**근거**
- `GameServer/GameRoom.LifeTime.cpp` - 초기 스폰
- `GameServer/GameRoom.Monster.cpp` - `EnterMonster`, `LeaveMonster`
- `GameServer/GameRoom.Items.cpp` - `HandleMonsterDead`

## 6) 파티(생성/초대/탈퇴/던전 연동/서로 다른 룸 참조 가능 여부 포함)
### 파티 생성/초대/수락/거절/탈퇴/강퇴/해산 (상태: 완료)
**유저 관점**
- 파티를 만들고 다른 유저를 초대해 함께 플레이할 수 있다.
- 파티장 권한/탈퇴/강퇴/해산이 즉시 반영된다.

**서버 내부 흐름**
1. `C_PARTY_*` 요청을 수신한다.
2. PartyActor가 JobQueue로 요청을 직렬화한다.
3. PartyManagerCore가 파티 상태/멤버/버전을 갱신한다.
4. `S_PARTY_RESULT`로 성공/실패를 응답한다.
5. `S_PARTY_INFO_NTF`로 전체 멤버에게 최신 정보를 브로드캐스트한다.

**검증/예외 케이스**
- 대상 오프라인/이미 파티/리더 권한 없음/자기 자신 초대 등은 실패.
- 맵 이동 중 요청은 무시된다.

**근거**
- `GameServer/ClientPacketHandler.Party.cpp`
- `GameServer/PartyActor.cpp`
- `GameServer/PartyManagerCore.h/.cpp`
- `Common/Protobuf/bin/Protocol.proto` - `C_PARTY_CREATE_REQ`, `C_PARTY_INVITE_REQ`, `S_PARTY_RESULT`, `S_PARTY_INFO_NTF`

### 파티 정보/버전 동기화 (상태: 완료)
**유저 관점**
- 파티 UI가 최신 멤버 목록/리더 정보를 유지한다.
- 맵 이동 후에도 파티 정보가 자동 갱신된다.

**서버 내부 흐름**
1. 파티 변경 시 버전을 증가시킨다.
2. `MakePartyInfoNtf`로 스냅샷을 구성한다.
3. `BroadcastPartyInfo`가 멤버 세션에 전파한다.
4. 맵 이동 완료 시 `S_PARTY_INFO_NTF`를 재전송한다.
5. 클라이언트는 버전을 기준으로 UI를 업데이트한다.

**검증/예외 케이스**
- 세션이 없으면 전송 스킵.
- 파티 ID가 0이면 빈 파티 정보 전송.

**근거**
- `GameServer/ClientPacketHandler.Party.cpp` - `BroadcastPartyInfo`, `MakePartyInfoNtf`
- `GameServer/GameRoom.EnterLeave.cpp` - 맵 이동 후 파티 정보 재전송

### 파티 채팅(서로 다른 룸 참조) (상태: 완료)
**유저 관점**
- 파티원은 서로 다른 맵/채널에 있어도 채팅을 주고받는다.
- 파티에 속하지 않으면 파티 채팅이 전달되지 않는다.

**서버 내부 흐름**
1. `C_PARTY_CHAT_REQ` 수신.
2. PartyActor가 `partyId`와 멤버 목록을 조회한다.
3. GameSessionManager로 각 멤버 세션을 찾는다.
4. 각 세션에 `S_PARTY_CHAT_NTF`를 전송한다.
5. 룸/채널과 무관하게 전파된다.

**검증/예외 케이스**
- 파티 미소속이면 무시.
- 대상 세션이 없으면 해당 멤버 전송 스킵.

**근거**
- `GameServer/ClientPacketHandler.Party.cpp` - `Handle_C_PARTY_CHAT_REQ`
- `GameServer/GameSessionManager.cpp`
- `Common/Protobuf/bin/Protocol.proto` - `C_PARTY_CHAT_REQ`, `S_PARTY_CHAT_NTF`

### 파티 상태 스냅샷(HP/레벨/맵/위치) (상태: 완료)
**유저 관점**
- 파티 UI에서 멤버의 레벨/HP/위치를 확인할 수 있다.
- 멤버가 다른 룸에 있어도 상태가 수집된다.

**서버 내부 흐름**
1. 요청자가 `C_PARTY_STATUS_REQ`를 전송한다.
2. PartyActor가 파티 스냅샷을 가져와 멤버 ID 목록을 만든다.
3. 요청자 세션이 각 멤버 세션으로 팬아웃 요청을 보낸다.
4. 각 멤버 세션은 자신의 GameRoom에서 상태를 읽어 응답한다.
5. Collector가 결과를 모아 `S_PARTY_STATUS_NTF`를 전송한다.

**검증/예외 케이스**
- 맵 이동 중/로비 상태의 멤버는 스킵.
- 파티 없음/멤버 0명인 경우 빈 스냅샷 전송.

**근거**
- `GameServer/ClientPacketHandler.Party.cpp` - `Handle_C_PARTY_STATUS_REQ`
- `Common/Protobuf/bin/Protocol.proto` - `PartyMemberStatus`, `S_PARTY_STATUS_NTF`

### 던전 연동(파티 인스턴스 메타) (상태: 완료)
**유저 관점**
- 파티 던전 입장/퇴장 시 파티 상태가 함께 관리된다.
- 강제 귀환/강퇴 연동까지 함께 처리된다.

**서버 내부 흐름**
1. PartyManagerCore가 `instanceId`, `dungeonState`를 저장한다.
2. 던전 입장 시 ENTERING/IN_DUNGEON 상태로 전이한다.
3. 던전 종료 시 인스턴스를 닫고 상태를 NONE으로 되돌린다.
4. 파티 탈퇴/강퇴/해산 시 인스턴스 멤버를 제거한다.
5. 강퇴/해산/종료 시 ForceReturnToWorld로 원래 맵으로 복귀시킨다.

**검증/예외 케이스**
- 던전 상태 전이 중 중복 요청은 실패.
- 인스턴스 닫기 실패 시 롤백.

**근거**
- `GameServer/PartyManagerCore.h/.cpp` - `DungeonState`
- `GameServer/PartyActor.cpp` - 인스턴스 정리/강제 귀환
- `GameServer/ClientPacketHandler.Dungeon.cpp`

## 7) 교환(거래 상태, 아이템/골드, Ready/Confirm 등)
### 1:1 거래 상태 머신 (상태: 완료)
**유저 관점**
- 상대에게 거래를 요청하고, 수락/준비/확정 단계로 교환한다.
- 어느 한쪽이 취소하면 거래가 종료된다.

**서버 내부 흐름**
1. `C_TRADE_REQ` 수신 → TradeSession 생성 및 `S_TRADE_INVITE` 전송.
2. 상대의 `C_TRADE_INVITE_RESP` 수락 시 `S_TRADE_START` 전송.
3. `C_TRADE_OFFER_SET`으로 올린 아이템을 갱신하고 양쪽에 공유한다.
4. `C_TRADE_READY`가 양측 완료되면 `S_TRADE_LOCKED`로 잠금 상태 전환.
5. `C_TRADE_CONFIRM`이 양측 완료되면 커밋 단계로 진입한다.

**검증/예외 케이스**
- 이미 거래 중이면 실패.
- 상태가 맞지 않으면 요청 무시/실패 응답.

**근거**
- `GameServer/ClientPacketHandler.Trade.cpp`
- `GameServer/GameRoom.Trade.cpp` - `TradeState`, `HandleTrade*`
- `Common/Protobuf/bin/Protocol.proto` - Trade 메시지들

### 거래 검증/타임아웃/취소 (상태: 완료)
**유저 관점**
- 너무 멀거나 조건이 맞지 않으면 거래가 실패한다.
- 응답이 없으면 일정 시간 후 자동 취소된다.

**서버 내부 흐름**
1. 대상 존재/자기 자신/거리/맵 이동 여부를 검사한다.
2. 거래 중이면 중복 요청을 차단한다.
3. 아이템 UID/수량/장착 여부를 검증한다.
4. `UpdateTrades_ActorOnly`가 타임아웃을 감지해 취소한다.
5. 맵 이동/접속 종료 시 거래를 자동 취소한다.

**검증/예외 케이스**
- 거리 제한(too far) 실패.
- 커밋 중인 거래는 외부 취소 불가.

**근거**
- `GameServer/GameRoom.Trade.cpp` - `HandleTradeReqById`, `UpdateTrades_ActorOnly`, `CancelTrade_ActorOnly`
- `GameServer/GameRoom.EnterLeave.cpp` - 접속 종료 시 거래 취소
- `GameServer/ClientPacketHandler.MapChange.cpp` - 맵 이동 중 거래 금지

### 원자적 커밋(Phase1+DB 트랜잭션) (상태: 완료)
**유저 관점**
- 거래 확정 시 아이템 복사/증발 없이 안전하게 교환된다.

**서버 내부 흐름**
1. `BuildTradeCommitPlan`이 메모리 상에서 교환 결과를 시뮬레이션한다.
2. `S2S_REQ_TRADE_COMMIT`으로 DBAgent에 최종 스냅샷을 전송한다.
3. DBAgent가 `BEGIN TRAN`으로 A/B 아이템 삭제+UPSERT를 한 번에 수행한다.
4. `S2S_RES_TRADE_COMMIT` 성공 시 메모리/Redis를 갱신한다.
5. `S_TRADE_RESULT`로 결과를 양측에 알린다.

**검증/예외 케이스**
- 인벤 공간 부족 시 Phase1에서 실패.
- DB 실패 시 커밋 취소 및 상태 롤백.

**근거**
- `GameServer/GameRoom.Trade.cpp` - `BuildTradeCommitPlan_ActorOnly`, `StartTradeCommitPhase2_ActorOnly`
- `DBAgent/DBAgentPacketHandler.cpp` - `Handle_S2S_REQ_TRADE_COMMIT`
- `Common/Protobuf/bin/Protocol_S2S.proto` - `S2S_REQ/RES_TRADE_COMMIT`

### 골드 거래 (상태: 완료)
**유저 관점**
- 거래에 골드를 올리고 확정할 수 있다.
- 확정 후 양쪽 골드가 즉시 갱신된다.

**서버 내부 흐름**
1. `C_TRADE_GOLD_SET`으로 골드 제안 금액을 전송한다.
2. 서버가 잔액/음수/오버플로우를 검증한다.
3. `S_TRADE_OFFER_UPDATE`에 골드 정보를 포함해 양쪽에 동기화한다.
4. Phase1에서 아이템+골드를 포함한 최종 스냅샷을 계산한다.
5. `S2S_REQ_TRADE_COMMIT`에 `finalGoldA/B`를 담아 DB 업데이트 후 `S_GOLD_UPDATE`를 전송한다.

**검증/예외 케이스**
- 보유 골드보다 큰 금액은 실패.
- 음수/오버플로우 금액 차단.

**근거**
- `Common/Protobuf/bin/Protocol.proto` - `C_TRADE_GOLD_SET`, `S_TRADE_OFFER_UPDATE`, `S_GOLD_UPDATE`
- `GameServer/GameRoom.Trade.cpp` - `HandleTradeGoldSetById`, `BuildTradeCommitPlan_ActorOnly`, `OnTradeCommitResult_ActorOnly`
- `DBAgent/DBAgentPacketHandler.cpp` - `Handle_S2S_REQ_TRADE_COMMIT`

## 8) 인벤토리/장비/퀵슬롯
### 인벤토리 로딩/동기화 (상태: 완료)
**유저 관점**
- 게임 입장 시 인벤토리가 로딩되고 화면에 표시된다.
- 아이템 변화가 즉시 반영된다.

**서버 내부 흐름**
1. `C_ENTER_GAME` 처리 중 `S2S_REQ_ITEMS_LOAD`를 전송한다.
2. DBAgent가 `ITEMS` 테이블에서 아이템을 읽어온다.
3. LobbyRoom이 아이템을 Player에 반영하고 Redis에 프라임한다.
4. `S_ITEM_LIST`를 클라이언트에 전송한다.
5. 이후 변경 시 `S_CHANGE_ITEM`/`S_REMOVE_ITEM`를 전송한다.

**검증/예외 케이스**
- DB 로딩 실패 시 로비 대기/진입 실패 처리 TODO.
- 잘못된 슬롯 인덱스는 로딩 단계에서 필터링.

**근거**
- `GameServer/ClientPacketHandler.EnterGame.cpp` - `S2S_REQ_ITEMS_LOAD`
- `DBAgent/DBAgentPacketHandler.cpp` - `Handle_S2S_REQ_ITEMS_LOAD`
- `GameServer/LobbyRoom.cpp` - `OnItemsLoaded`
- `GameServer/GameRoom.Items.cpp` - `S_CHANGE_ITEM`, `S_REMOVE_ITEM`
- `Common/Protobuf/bin/Protocol.proto` - `S_ITEM_LIST`

### 소모품 사용(포션) + 스탯 저장 (상태: 완료)
**유저 관점**
- 포션을 사용하면 HP가 회복되고 인벤 수량이 감소한다.
- HP/스탯 변경이 즉시 반영된다.

**서버 내부 흐름**
1. `C_USE_ITEM` 수신 후 아이템 UID/타입을 검증한다.
2. 소모품만 사용 가능하도록 필터링한다.
3. HP를 회복하고 `StatInfo`를 갱신한다.
4. Redis/Dirty에 스탯 및 인벤 변경을 기록한다.
5. `S_CHANGE_ITEM`/`S_REMOVE_ITEM` 및 `S_CHANGE_STAT`을 전송한다.

**검증/예외 케이스**
- 거래 중에는 사용 불가.
- 풀 HP면 사용하지 않음.

**근거**
- `GameServer/GameRoom.Items.cpp` - `HandleUseItem`
- `Common/Protobuf/bin/Protocol.proto` - `C_USE_ITEM`, `S_CHANGE_ITEM`, `S_REMOVE_ITEM`, `S_CHANGE_STAT`

### 장비 착/해제 + 스탯 재계산 (상태: 완료)
**유저 관점**
- 무기/방어구/투구 장착 시 공격/방어/HP가 갱신된다.
- 같은 부위 장비는 자동으로 교체된다.

**서버 내부 흐름**
1. `C_EQUIP_ITEM` 수신 후 대상 아이템을 찾는다.
2. 슬롯 정책으로 장착 가능 부위를 판별한다.
3. 같은 부위 장비가 있으면 자동 해제한다.
4. 인벤 정보와 Redis/DB를 업데이트한다.
5. `Player::RefreshStats`로 스탯을 재계산하고 `S_EQUIP_ITEM`/`S_CHANGE_STAT`를 전송한다.

**검증/예외 케이스**
- 거래 중 장비 변경 불가.
- 장착 중인 아이템 이동/교환 차단.

**근거**
- `GameServer/GameRoom.Items.cpp` - `HandleEquipItemById`
- `GameServer/Player.cpp` - `RefreshStats`
- `Common/Protobuf/bin/Struct.proto` - `ItemTemplateInfo`

### 인벤토리 드래그&드롭(이동/병합/스왑) (상태: 완료)
**유저 관점**
- 슬롯 이동, 같은 아이템 합치기, 서로 교환이 가능하다.

**서버 내부 흐름**
1. `C_INV_DRAG_DROP` 수신 후 슬롯/UID 일치 여부를 검증한다.
2. 대상 슬롯이 비어 있으면 이동 처리한다.
3. 같은 아이템 + 스택 가능이면 병합한다.
4. 다른 아이템이면 슬롯 스왑을 수행한다.
5. 변경 내용을 DB/Redis에 반영하고 `S_CHANGE_ITEM`/`S_REMOVE_ITEM` 전송한다.

**검증/예외 케이스**
- 거래 중 인벤 조작 불가.
- 장착 중 아이템은 이동/스왑 불가.

**근거**
- `GameServer/GameRoom.Items.cpp` - `HandleInvDragDrop`
- `Common/Protobuf/bin/Protocol.proto` - `C_INV_DRAG_DROP`

### 퀵슬롯 저장/중복 방지 (상태: 완료)
**유저 관점**
- 퀵슬롯에 아이템/스킬을 등록하면 UI가 즉시 갱신된다.
- 같은 아이템을 여러 슬롯에 중복 등록하면 기존 슬롯이 자동 비워진다.

**서버 내부 흐름**
1. `C_SET_QUICKSLOT` 수신 후 인덱스/타입을 검증한다.
2. Redis 스냅샷을 읽어 중복 아이템 등록을 해제한다.
3. Redis에 새 퀵슬롯 값을 기록하고 Dirty 마킹한다.
4. `S_SET_QUICKSLOT`로 변경 결과를 전송한다.
5. 로그인 시 `S_QUICKSLOT_LIST`로 초기 스냅샷을 전송한다.

**검증/예외 케이스**
- 맵 이동 중에는 변경 불가.
- 잘못된 인덱스는 무시.

**근거**
- `GameServer/ClientPacketHandler.Misc.cpp` - `Handle_C_SET_QUICKSLOT`
- `GameServer/LobbyRoom.cpp` - `OnQuickSlotsLoaded`
- `DBAgent/DBAgentPacketHandler.cpp` - `Handle_S2S_REQ_QUICKSLOT_LOAD/ SAVE`
- `Common/Protobuf/bin/Protocol.proto` - `S_QUICKSLOT_LIST`, `C_SET_QUICKSLOT`, `S_SET_QUICKSLOT`

### 몬스터 처치 보상(경험치/드랍) (상태: 부분 구현)
**유저 관점**
- 몬스터 처치 시 경험치가 오르고 아이템이 자동으로 인벤에 들어온다.
- 현재는 테스트 값 기반으로 동작한다.

**서버 내부 흐름**
1. 몬스터 사망 시 `HandleMonsterDead`가 호출된다.
2. 경험치를 추가하고 레벨업을 처리한다.
3. 스탯/인벤 변경을 Redis/DB에 기록한다.
4. 고정 템플릿 아이템을 오토루트 방식으로 지급한다.
5. 드랍 테이블/랜덤 로직은 미구현이다.

**검증/예외 케이스**
- 인벤이 가득 차면 드랍 획득 실패(알림 없음).
- 아이템 템플릿이 없으면 드랍 생략.

**근거**
- `GameServer/GameRoom.Items.cpp` - `HandleMonsterDead`, `AddExpAndLevelUp`
- `GameServer/DataManager.cpp` - `StatTemplateInfo`, `ItemTemplateInfo`

## 9) 채팅
### 로컬(맵) 채팅 (상태: 완료)
**유저 관점**
- 같은 맵/룸의 유저에게 채팅이 브로드캐스트된다.
- 메시지는 즉시 전송된다.

**서버 내부 흐름**
1. `C_CHAT_REQ` 수신.
2. `HandleChatById`가 발신자 정보와 메시지를 조립한다.
3. `BroadcastChat`이 룸 내 모든 플레이어에게 전송한다.
4. 별도 저장/필터링 로직은 없다.
5. 파티/월드 채팅과는 분리되어 동작한다.

**검증/예외 케이스**
- 맵 이동 중 요청은 무시.

**근거**
- `GameServer/ClientPacketHandler.GamePlay.cpp` - `Handle_C_CHAT_REQ`
- `GameServer/GameRoom.Chat.cpp`
- `Common/Protobuf/bin/Protocol.proto` - `C_CHAT_REQ`, `S_CHAT_NTF`

### 파티 채팅 (상태: 완료)
**유저 관점**
- 파티원끼리 전용 채팅을 주고받는다.
- 서로 다른 맵/채널에서도 동작한다.

**서버 내부 흐름**
1. `C_PARTY_CHAT_REQ` 수신.
2. PartyActor가 멤버 목록을 조회한다.
3. GameSessionManager로 각 멤버 세션을 찾는다.
4. `S_PARTY_CHAT_NTF`를 각 세션에 전송한다.
5. 룸/채널과 무관하게 전파된다.

**검증/예외 케이스**
- 파티 미가입 시 무시.
- 오프라인 멤버는 전송 스킵.

**근거**
- `GameServer/ClientPacketHandler.Party.cpp` - `Handle_C_PARTY_CHAT_REQ`
- `Common/Protobuf/bin/Protocol.proto` - `S_PARTY_CHAT_NTF`

## 10) 운영/디버깅/로그/관리툴
### Redis Write-Back 저장 + 자동 커밋 (상태: 완료)
**유저 관점**
- 접속 종료나 주기적인 저장으로 캐릭터 데이터가 보존된다.
- 서버 장애 시 손실을 최소화한다.

**서버 내부 흐름**
1. 런타임 변경을 Redis 해시와 Dirty Set에 기록한다.
2. AutoCommitService가 주기적으로 Dirty PID를 수집한다.
3. 스냅샷을 만들어 `S2S_REQ_SAVE_*`로 DB에 저장 요청한다.
4. DBAgent 응답 성공 시 Dirty 플래그를 제거한다.
5. 접속 종료 시 `RequestFlushNow`로 즉시 저장을 요청한다.

**검증/예외 케이스**
- Redis 연결 실패 시 저장 로직 작동 불가.
- 인플라이트 중복 저장은 차단.

**근거**
- `GameServer/PersistenceService.h/.cpp`
- `GameServer/AutoCommitService.cpp`
- `GameServer/PlayerSession.cpp` - `RequestFlushNow`
- `Common/Protobuf/bin/Protocol_S2S.proto` - `S2S_REQ_SAVE_PLAYER_CORE/INVENTORY/QUICKSLOT`

### 접속 종료 처리/인스턴스 정리 (상태: 완료)
**유저 관점**
- 접속 종료 시 캐릭터가 월드에서 사라지고 던전 상태가 정리된다.

**서버 내부 흐름**
1. `OnDisconnected`에서 세션 바인딩 해제 및 Dirty 마킹을 수행한다.
2. 현재 룸에서 플레이어를 제거한다.
3. InstanceActor가 오프라인 멤버를 인스턴스에서 제거한다.
4. 인스턴스가 비면 Closing 처리 후 Purge 대상으로 전환한다.
5. 맵 변경 상태를 취소한다.

**검증/예외 케이스**
- 룸이 없으면 제거 스킵.
- 인스턴스 정보가 없으면 정리 생략.

**근거**
- `GameServer/PlayerSession.cpp` - `OnDisconnected`
- `GameServer/GameRoom.EnterLeave.cpp` - `LeaveById`
- `GameServer/InstanceManagerCore.cpp` - `OnMemberOffline`
- `GameServer/RoomManager.cpp` - `PurgeInstanceRooms`

### 하트비트/Ping (상태: 부분 구현)
**유저 관점**
- 연결 유지용 하트비트가 존재하지만 상세 상태 모니터링은 없다.

**서버 내부 흐름**
1. 클라이언트가 `C_HEART_BEAT_REQ`를 전송한다.
2. 서버 핸들러는 별도 로직 없이 성공 처리한다.
3. `PlayerSession::Ping`이 `S_HEART_BEAT_RES`를 전송한다.
4. 타임아웃/지연 측정 로직은 없다.
5. 모니터링 대시보드 연동은 없다.

**검증/예외 케이스**
- 하트비트 미수신 시 강제 종료 로직 미구현.

**근거**
- `GameServer/ClientPacketHandler.Misc.cpp` - `Handle_C_HEART_BEAT_REQ`
- `GameServer/PlayerSession.cpp` - `Ping`
- `Common/Protobuf/bin/Protocol.proto` - `C_HEART_BEAT_REQ`, `S_HEART_BEAT_RES`

### 테스트/운영 도구 (상태: 부분 구현)
**유저 관점**
- 콘솔 명령으로 파티 기능을 테스트할 수 있다.
- 패킷 생성 도구를 통해 핸들러/패킷 코드를 생성한다.

**서버 내부 흐름**
1. GameServer 콘솔 스레드가 `/p_create` 등 테스트 명령을 처리한다.
2. PacketGenerator가 프로토콜 기반 핸들러/패킷 코드를 생성한다.
3. 운영용 모니터링 UI는 별도 미구현이다.

**근거**
- `GameServer/GameServer.cpp` - `ConsoleThread`
- `Tools/PacketGenerator/PacketGenerator.py`

## 데모 시나리오 5개(영상/면접에서 보여줄 흐름)
1. 로그인 → 토큰 발급 → `C_ENTER_GAME` → 인벤/퀵슬롯 로딩 → `S_ENTER_GAME`/AOI 스냅샷.
2. 이동 검증 데모: 정상 이동 vs 과속/벽 통과 시 서버 클램프/슬라이딩 로그 확인.
3. 파티 생성/초대/상태 스냅샷 → 던전 입장 → 인스턴스 생성 → 던전 종료 후 복귀.
4. 전투 데모: 몬스터 AI 추격 → 투사체 스킬 적중 → `S_CHANGE_HP` → 경험치/드랍 반영.
5. 1:1 거래 데모: 초대 → 오퍼 → Ready/Lock → Confirm → DB 커밋 → 인벤 변경 확인.

## TODO / 미완 정리
- 로그인 비밀번호 검증 및 계정 생성/가입 플로우
- DB 로딩 실패 처리(로비 게이트)
- 스킬 타입 확장(원형/부채꼴 등) 및 범위 판정 고도화 (완료)
- 플레이어 부활/리젠 흐름(리스폰 패킷/스폰) (절반 완료. 애니메이팅만 동기화 하게끔)
- 몬스터 리젠/스폰 테이블 + 드랍 테이블/랜덤 로직
- 하트비트 타임아웃/지연 측정 + 모니터링
- 채팅 로그/모니터링 고도화
- 운영용 모니터링 UI
