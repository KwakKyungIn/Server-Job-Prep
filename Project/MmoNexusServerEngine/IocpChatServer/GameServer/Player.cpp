#include "pch.h"
#include "Player.h"
#include "PlayerSession.h"
#include "GameRoom.h"
#include "DataManager.h"
#include "PersistenceService.h"

// 플레이어 생성자. 부모 클래스인 Creature를 먼저 초기화해준다
// OBJECT_TYPE_PLAYER 타입을 넘겨줘야 나중에 캐스팅할 때 안 헷갈림
Player::Player() : Creature(Protocol::OBJECT_TYPE_PLAYER)
{
	// Protobuf 객체 초기화
	// 여기서 ID랑 이름을 대충이라도 넣어놔야 디버깅할 때 편함
	_playerInfo.set_playerid(0);
	_playerInfo.set_name("Uninitialized");
	_playerInfo.set_type(Protocol::PLAYER_NONE);

	// 이 부분이 제일 중요함
	// PlayerInfo는 Player가 들고 있지만, 위치 정보나 스탯 정보는 부모인 Creature도 알아야 함
	// 그래서 _playerInfo 내부의 mutable 포인터를 꺼내서 부모의 포인터 변수랑 연결해주는 작업
	// 이거 안 해주면 Creature 쪽에서 이동 처리할 때 nullptr 접근해서 서버 터짐
	_posInfo = _playerInfo.mutable_posinfo();
	_statInfo = _playerInfo.mutable_statinfo();

	// 초기 좌표 설정. 맵 한가운데로 떨어뜨림
	_posInfo->set_state(Protocol::MOVE_IDLE);
	_posInfo->set_x(50.0f);
	_posInfo->set_y(0.0f);
	_posInfo->set_z(50.0f);
	_posInfo->set_yaw(0.0f);
}

Player::~Player()
{
	// 플레이어가 메모리에서 해제될 때 호출됨
	// 예전엔 여기서 로그 찍었는데 로그 양이 너무 많아서 주석 처리함
}

// 플레이어가 피격당했을 때 호출되는 함수
// 단순히 HP만 깎는 게 아니라 DB(Redis) 동기화까지 고려해야 함
void Player::OnDamaged(std::shared_ptr<Creature> attacker, int32 damage)
{
	// 1. 일단 부모 클래스한테 넘겨서 HP 깎고 사망 처리 로직을 태움
	Creature::OnDamaged(attacker, damage);

	auto st = GetStatInfo();
	if (!st) return;

	const uint64 pid = GetPlayerId();

	// 2. 여기서 Redis 캐시에 바로 업데이트를 때림
	// 매번 피격될 때마다 DB에 쿼리 날리면 DB가 죽어버리니까
	// 일단 Redis(메모리)에만 반영하고 Dirty Flag를 켜둠
	// 실제 DB 저장은 나중에 배치로 돌거나 로그아웃할 때 처리됨 (Write-Back 패턴 적용)
	Persistence::PersistenceService::I().UpdatePlayerCore(
		pid,
		st->level(),
		st->hp(),
		st->totalexp(),
		true
	);
}

void Player::OnDead(std::shared_ptr<Creature> attacker)
{
	// 1. 부모 쪽 사망 처리 (상태 변경 등)
	Creature::OnDead(attacker);

	// 2. 플레이어 전용 사망 로직
	// 나중엔 여기에 경험치 드랍이나 부활 UI 패킷 보내는 거 추가해야 함
	printf(" [Player Dead] %s has been slain!\n", _playerInfo.name().c_str());

	// 변신 상태였다면 해제하거나 버프 다 지우는 로직도 필요할 듯
}

// 로그인 성공 후 DB에서 가져온 데이터를 플레이어 객체에 덮어씌우는 함수
void Player::Init(const Protocol::PlayerInfo& info)
{
	// DB에서 가져온 따끈따끈한 데이터를 내 로컬 변수에 깊은 복사
	_playerInfo.CopyFrom(info);

	// CopyFrom을 하면 내부 데이터 주소가 바뀔 수도 있어서 포인터를 다시 연결해줘야 함
	// 이거 실수로 빼먹으면 이동 동기화가 안 되는 버그 생김
	_posInfo = _playerInfo.mutable_posinfo();
	_statInfo = _playerInfo.mutable_statinfo();

	// 만약 DB에 위치 정보가 없으면(신규 유저) 기본 위치로 세팅
	if (_playerInfo.has_posinfo() == false)
	{
		_posInfo->set_state(Protocol::MOVE_IDLE);
		_posInfo->set_x(50.0f);
		_posInfo->set_y(0.0f);
		_posInfo->set_z(50.0f);
		_posInfo->set_yaw(0.0f);
	}

	// 이동 검증용 변수들 초기화
	// 룸 스레드에서만 접근해야 하므로 여기서 미리 세팅해둠
	ResetMoveStamp_ActorOnly();
	_lastAcceptedPos.CopyFrom(*_posInfo);
}

// 스탯 재계산 함수
// 레벨업하거나 장비 갈아끼면 무조건 이거 호출해줘야 함
void Player::RefreshStats()
{
	// 1. 현재 스탯 정보 포인터 가져오기
	Protocol::StatInfo* stat = GetStatInfo();
	if (stat == nullptr) return;

	// 2. 데이터 시트(DataManager)에서 내 레벨에 맞는 기본 스탯 가져오기
	const Protocol::StatTemplateInfo* baseStat = DataManager::Instance()->GetStatTemplate(stat->level());
	if (baseStat == nullptr)
	{
		// 데이터 없으면 큰일남. 일단 리턴
		return;
	}

	// 3. 일단 기본 스탯으로 덮어씌움 (초기화)
	stat->set_maxhp(baseStat->maxhp());
	stat->set_attack(baseStat->attack());
	stat->set_defense(baseStat->defense());
	stat->set_speed(baseStat->speed());

	// 4. 장착한 아이템들의 스탯 보너스를 전부 더함
	// 아이템 개수가 많아지면 여기서 루프 도는 게 성능 부담될 수도 있는데
	// 지금은 아이템이 별로 없어서 괜찮을 듯
	for (const auto& item : _items)
	{
		if (item.isequipped())
		{
			const Protocol::ItemTemplateInfo* itemData = DataManager::Instance()->GetItemTemplate(item.templateid());
			if (itemData)
			{
				stat->set_maxhp(stat->maxhp() + itemData->hpbonus());
				stat->set_attack(stat->attack() + itemData->attackbonus());
				stat->set_defense(stat->defense() + itemData->defensebonus());
			}
		}
	}

	// 5. HP 보정
	// 장비 벗어서 최대 체력이 깎였는데 현재 체력이 더 높으면 안 되니까 깎아줌
	if (stat->hp() > stat->maxhp())
		stat->set_hp(stat->maxhp());
}

void Player::Update()
{
	// 게임 루프에서 매 프레임(혹은 틱)마다 호출되는 곳
	// 쿨타임 계산이나 도트 데미지 처리 같은 거 여기서 하면 됨
}